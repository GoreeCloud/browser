// SPDX-License-Identifier: MPL-2.0
// Real Linux filesystem checks through a deliberately synthetic Gecko API shim.
// This does NOT run privileged Gecko IOUtils or prove Gecko profile security.
import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import os from 'node:os';
import { BookmarkStore } from '../bookmarks/store.mjs';
import { exportPortableBookmarks } from '../bookmarks/portable.mjs';
import { createGeckoProfileBookmarkDriver, GECKO_PROFILE_BOOKMARK_FILE } from '../bookmarks/gecko-profile-driver.mjs';

const bookmark=id=>({id,title:id,type:'bookmark',url:'https://example.org/'+id});
const data=id=>exportPortableBookmarks([bookmark(id)]);
const notFound=()=>Object.assign(new Error('No such profile file'),{name:'NotFoundError'});

async function fixture(t) {
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'goreecloud-profile-fs-'));
  await fs.chmod(dir,0o700);
  t.after(async()=>fs.rm(dir,{recursive:true,force:true}));
  const primary=path.join(dir,GECKO_PROFILE_BOOKMARK_FILE);
  const backup=primary+'.bak', temporary=primary+'.tmp';
  let interrupt=false,owner=true,tempCounter=0;
  const ioUtils={
    async getChildren(p) {return (await fs.readdir(p)).map(name=>path.join(p,name));},
    async createUniqueFile(parent,prefix,mode) {
      assert.equal(mode,0o600);
      const name=path.join(parent,prefix+(++tempCounter));
      const handle=await fs.open(name,'wx',mode);
      await handle.close();
      return name;
    },
    async exists(p) {
      try { await fs.lstat(p); return true; }
      catch(error) { if(error.code==='ENOENT')return false; throw error; }
    },
    async stat(p) {
      let st;
      try { st=await fs.lstat(p); }
      catch(error) { if(error.code==='ENOENT')throw notFound(); throw error; }
      return {type:st.isFile()?'regular':st.isDirectory()?'directory':'other',size:st.size,permissions:st.mode & 0o777};
    },
    readUTF8:p=>fs.readFile(p,'utf8'),
    async writeUTF8(p,text,options) {
      assert.equal(options.mode,'overwrite');
      assert.equal(options.flush,true);
      assert.ok(options.tmpPath.startsWith(primary+'.tmp-'));
      if(options.backupFile)await fs.rename(p,options.backupFile);
      const handle=await fs.open(options.tmpPath,'r+');
      try { await handle.truncate(0); await handle.writeFile(text,'utf8'); await handle.sync(); }
      finally { await handle.close(); }
      if(interrupt) {
        interrupt=false;
        throw Error('Simulated crash before atomic replace');
      }
      await fs.rename(options.tmpPath,p);
      return Buffer.byteLength(text);
    },
    setPermissions:(p,mode)=>fs.chmod(p,mode),
  };
  const assertProfileSecurity=async()=>{
    if(!owner)return false;
    try {
      const stat=await fs.lstat(dir);
      if(!stat.isDirectory()||stat.isSymbolicLink()||(stat.mode&0o077)!==0)return false;
      const pending=(await fs.readdir(dir)).filter(name=>name.startsWith(GECKO_PROFILE_BOOKMARK_FILE+'.tmp-')).map(name=>path.join(dir,name));
      for(const p of [primary,backup,temporary,...pending]) {
        let item;
        try { item=await fs.lstat(p); }
        catch(error) { if(error.code==='ENOENT')continue; throw error; }
        if(!item.isFile()||item.isSymbolicLink()||(item.mode&0o077)!==0)return false;
      }
      return true;
    } catch { return false; }
  };
  const driver=()=>createGeckoProfileBookmarkDriver({
    ioUtils,
    pathUtils:{profileDir:dir,join:path.join,isAbsolute:path.isAbsolute},
    assertProfileSecurity,developmentEnabled:true,
  });
  return {dir,primary,backup,temporary,driver,
    async pendingTemps() {return (await fs.readdir(dir)).filter(n=>n.startsWith(GECKO_PROFILE_BOOKMARK_FILE+'.tmp-')).map(n=>path.join(dir,n));},
    stopOwner:()=>{owner=false;},
    interruptNextWrite:()=>{interrupt=true;}};
}

test('real filesystem: offline reopen, backup and private file permissions',async t=>{
  const e=await fixture(t),store=new BookmarkStore(e.driver());
  await store.add({item:bookmark('first')});
  assert.equal((await fs.stat(e.primary)).mode&0o777,0o600);
  await store.add({item:bookmark('second')});
  assert.equal((await fs.stat(e.backup)).mode&0o777,0o600);
  assert.deepEqual(JSON.parse(await fs.readFile(e.backup,'utf8')).nodes.map(x=>x.id),['first']);
  const reopened=new BookmarkStore(e.driver());
  assert.deepEqual((await reopened.snapshot()).nodes.map(x=>x.id),['first','second']);
});

test('real filesystem: interrupted backup move preserves backup and temp evidence',async t=>{
  const e=await fixture(t),driver=e.driver();
  assert.equal(await driver.read(),null);
  await driver.write(data('safe'));
  e.interruptNextWrite();
  await assert.rejects(driver.write(data('interrupted')),/Simulated crash/);
  assert.equal(await e.driver().read().then(()=>true,()=>false),false);
  assert.deepEqual(JSON.parse(await fs.readFile(e.backup,'utf8')).nodes.map(x=>x.id),['safe']);
  const pending=await e.pendingTemps();
  assert.equal(pending.length,1);
  assert.deepEqual(JSON.parse(await fs.readFile(pending[0],'utf8')).nodes.map(x=>x.id),['interrupted']);
  const recovery=e.driver();
  await assert.rejects(recovery.read(),{code:'RECOVERY_REQUIRED'});
  await assert.rejects(recovery.write(data('new')),{code:'READ_REQUIRED'});
  assert.deepEqual(JSON.parse(await fs.readFile(pending[0],'utf8')).nodes.map(x=>x.id),['interrupted']);
});

test('real filesystem: symlink profile file fails closed without touching target',async t=>{
  const e=await fixture(t),outside=path.join(e.dir,'untouched.json');
  await fs.writeFile(outside,data('outside'),{mode:0o600});
  await fs.symlink(outside,e.primary);
  const driver=e.driver();
  await assert.rejects(driver.read(),{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.deepEqual(JSON.parse(await fs.readFile(outside,'utf8')).nodes.map(x=>x.id),['outside']);
});

test('real filesystem: world-readable profile file and revoked owner deny access',async t=>{
  const e=await fixture(t);
  await fs.writeFile(e.primary,data('original'),{mode:0o600});
  await fs.chmod(e.primary,0o644);
  await assert.rejects(e.driver().read(),{code:'PROFILE_SECURITY_UNVERIFIED'});
  await fs.chmod(e.primary,0o600);
  const d=e.driver();
  await d.read();
  e.stopOwner();
  await assert.rejects(d.write(data('unsafe')),{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.deepEqual(JSON.parse(await fs.readFile(e.primary,'utf8')).nodes.map(x=>x.id),['original']);
});

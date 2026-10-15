// SPDX-License-Identifier: MPL-2.0
// These are real, isolated Node filesystem checks with an IOUtils-shaped
// emulator. They are NOT Gecko xpcshell or proof of Gecko's file permissions.
import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, mkdir, lstat, readFile, writeFile, rename, chmod, access, rm, symlink, readdir, open } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join, isAbsolute } from 'node:path';
import { BookmarkStore } from '../bookmarks/store.mjs';
import { exportPortableBookmarks } from '../bookmarks/portable.mjs';
import { createGeckoProfileBookmarkDriver, GECKO_PROFILE_BOOKMARK_FILE } from '../bookmarks/gecko-profile-driver.mjs';

const bookmark = id => ({id, title:id, type:'bookmark', url:'https://example.org/'+id});
const portable = id => exportPortableBookmarks([bookmark(id)]);
const absent = error => error?.code === 'ENOENT';
const missing = () => Object.assign(new Error('Missing profile file'),{name:'NotFoundError'});

async function fixture(t) {
  const root=await mkdtemp(join(tmpdir(),'goreecloud-browser-profile-test-'));
  t.after(()=>rm(root,{recursive:true,force:true}));
  const profile=join(root,'profile');
  await mkdir(profile,{mode:0o700});
  const file=join(profile,GECKO_PROFILE_BOOKMARK_FILE);
  const pathUtils={profileDir:profile,join,isAbsolute};
  let tempCounter=0;
  const ioUtils={
    async getChildren(parent) {return (await readdir(parent)).map(name=>join(parent,name));},
    async createUniqueFile(parent,prefix,mode) {
      assert.equal(mode,0o600);
      const path=join(parent,prefix+(++tempCounter));
      const handle=await open(path,'wx',mode);
      await handle.close();
      return path;
    },
    async exists(path) {
      try { await access(path); return true; }
      catch(error) { if(absent(error)) return false; throw error; }
    },
    async stat(path) {
      let info;
      try {info=await lstat(path);} catch(error) {if(absent(error)) throw missing();throw error;}
      return {type:info.isFile()?'regular':info.isDirectory()?'directory':'other',size:info.size};
    },
    async readUTF8(path) {
      try {return await readFile(path,'utf8');}
      catch(error) {if(absent(error)) throw missing();throw error;}
    },
    async writeUTF8(path,text,options) {
      // Firefox 158 IOUtils.cpp passes 0666 to nsFileOutputStream::Init:
      // the process umask controls privacy until any later chmod. Mimic it.
      assert.equal(options.mode,'overwrite');
      assert.equal(options.flush,true);
      await writeFile(options.tmpPath,text,{encoding:'utf8',flag:'w',mode:0o666,flush:true});
      // Gecko's IOUtils::WriteSync MOVES the original file to backup first.
      if(options.backupFile) await rename(path,options.backupFile);
      await rename(options.tmpPath,path);
    },
    async setPermissions(path,mode) { await chmod(path,mode); },
  };
  // Harness-only guard: verifies the private directory and rejects symbolic
  // links at known paths. This does NOT implement a real Gecko profile lock.
  async function assertProfileSecurity() {
    const dir=await lstat(profile);
    if(!dir.isDirectory() || (dir.mode & 0o077)!==0) return false;
    const privateFiles=(await readdir(profile)).filter(name=>name.startsWith(GECKO_PROFILE_BOOKMARK_FILE+'.tmp-')).map(name=>join(profile,name));
    for(const path of [file,file+'.tmp',file+'.bak',...privateFiles]) {
      try { const st=await lstat(path); if(st.isSymbolicLink() || (st.mode & 0o077)!==0) return false; }
      catch(error) {if(!absent(error)) throw error;}
    }
    return true;
  }
  const driver=()=>createGeckoProfileBookmarkDriver({
    ioUtils,pathUtils,developmentEnabled:true,assertProfileSecurity,
  });
  return {root,profile,file,ioUtils,driver};
}

test('isolated filesystem persists a bookmark and preserves private backup',async t=>{
  const e=await fixture(t),store=new BookmarkStore(e.driver());
  await store.add({item:bookmark('first')});
  const first=await readFile(e.file,'utf8');
  await store.add({item:bookmark('second')});
  assert.equal(await readFile(e.file+'.bak','utf8'),first);
  assert.deepEqual((await new BookmarkStore(e.driver()).snapshot()).nodes.map(x=>x.id),
    ['first','second']);
  if(process.platform!=='win32') {
    assert.equal((await lstat(e.file)).mode & 0o777,0o600);
    assert.equal((await lstat(e.file+'.bak')).mode & 0o777,0o600);
  }
});

test('a durable write followed by lost acknowledgement reloads from disk',async t=>{
  const e=await fixture(t),store=new BookmarkStore(e.driver());
  await store.snapshot();
  const original=e.ioUtils.writeUTF8;
  let first=true;
  e.ioUtils.writeUTF8=async(...args)=>{
    await original(...args);
    if(first){first=false;throw Error('Synthetic acknowledgement lost');}
  };
  await assert.rejects(store.add({item:bookmark('committed')}),/acknowledgement lost/);
  assert.deepEqual((await store.snapshot()).nodes.map(x=>x.id),['committed']);
  await store.add({item:bookmark('next')});
  assert.deepEqual((await new BookmarkStore(e.driver()).snapshot()).nodes.map(x=>x.id),
    ['committed','next']);
});

test('orphan temporary data is preserved despite a valid primary file',async t=>{
  const e=await fixture(t),driver=e.driver();
  await writeFile(e.file,portable('older'),{mode:0o600});
  await writeFile(e.file+'.tmp',portable('interrupted'),{mode:0o600});
  await driver.read();
  await assert.rejects(driver.write(portable('new')),{code:'RECOVERY_REQUIRED'});
  assert.equal(await readFile(e.file,'utf8'),portable('older'));
  assert.equal(await readFile(e.file+'.tmp','utf8'),portable('interrupted'));
});

test('corrupted persistent data cannot authorize a replacement',async t=>{
  const e=await fixture(t),driver=e.driver();
  await writeFile(e.file,'{broken',{mode:0o600});
  await assert.rejects(driver.read());
  await assert.rejects(driver.write(portable('replacement')),{code:'READ_REQUIRED'});
  assert.equal(await readFile(e.file,'utf8'),'{broken');
});

test('an insecure profile directory refuses real filesystem reads and writes',async t=>{
  if(process.platform==='win32') {t.skip('POSIX permissions required');return;}
  const e=await fixture(t),driver=e.driver();
  await chmod(e.profile,0o755);
  await assert.rejects(driver.read(),{code:'PROFILE_SECURITY_UNVERIFIED'});
  await assert.rejects(driver.write(portable('nope')),{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(await e.ioUtils.exists(e.file),false);
});

test('a symlinked temporary artifact is rejected without touching its target',async t=>{
  if(process.platform==='win32') {t.skip('symlink privileges vary');return;}
  const e=await fixture(t),driver=e.driver();
  const sentinel=join(e.root,'sentinel');
  await writeFile(sentinel,'preserve');
  await symlink(sentinel,e.file+'.tmp');
  await assert.rejects(driver.read(),{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(await readFile(sentinel,'utf8'),'preserve');
});

test('backup-move interruption preserves the old data without blind recreation',async t=>{
  const e=await fixture(t),driver=e.driver();
  const old=portable('safe-old');
  await writeFile(e.file,old,{mode:0o600});
  await driver.read();
  e.ioUtils.writeUTF8=async(path,text,options)=>{
    assert.ok(options.backupFile);
    await rename(path,options.backupFile);
    throw Error('Synthetic failure after backup move');
  };
  await assert.rejects(driver.write(portable('not-committed')),
    /Synthetic failure after backup move/);
  assert.equal(await e.ioUtils.exists(e.file),false);
  assert.equal(await readFile(e.file+'.bak','utf8'),old);
  await assert.rejects(driver.read(),{code:'RECOVERY_REQUIRED'});
  await assert.rejects(driver.write(portable('blind-recreate')),{code:'READ_REQUIRED'});
  assert.equal(await readFile(e.file+'.bak','utf8'),old);
});

test('precreated temporary file remains private despite 0666 write mode',async t=>{
  if(process.platform==='win32') {t.skip('POSIX creation mode required');return;}
  const e=await fixture(t),driver=e.driver();
  await driver.read();
  let observed;
  e.ioUtils.writeUTF8=async(path,text,options)=>{
    await writeFile(options.tmpPath,text,{flag:'w',mode:0o666});
    observed=(await lstat(options.tmpPath)).mode & 0o777;
    throw Error('Synthetic crash before replacement');
  };
  await assert.rejects(driver.write(portable('pending')),
    /Synthetic crash before replacement/);
  assert.equal(observed,0o600);
  const privateTemps=(await readdir(e.profile)).filter(name=>name.startsWith(GECKO_PROFILE_BOOKMARK_FILE+'.tmp-'));
  assert.equal(privateTemps.length,1);
  assert.equal((await lstat(join(e.profile,privateTemps[0]))).mode & 0o777,0o600);
  assert.equal(await e.ioUtils.exists(e.file),false);
});

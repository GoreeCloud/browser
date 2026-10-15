// SPDX-License-Identifier: MPL-2.0
// Node filesystem shims only. NOT Gecko IOUtils, real profile locking or ACL proof.
import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import os from 'node:os';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE} from '../bookmarks/gecko-profile-driver.mjs';
import {createGeckoBookmarkRecoveryInspector} from '../bookmarks/gecko-recovery-inspector.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const data=id=>exportPortableBookmarks([{id,title:id,type:'bookmark',url:'https://example.org/'+id}]);
const missing=e=>{if(e?.code==='ENOENT')e.name='NotFoundError';throw e;};
async function fixture(t) {
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'goreecloud-recovery-contract-'));
  await fs.chmod(dir,0o700);
  t.after(()=>fs.rm(dir,{recursive:true,force:true}));
  const primary=path.join(dir,GECKO_PROFILE_BOOKMARK_FILE);
  let writes=0,allowed=true;
  const ioUtils={
    getChildren:async p=>(await fs.readdir(p)).map(name=>path.join(p,name)),
    async exists(p){try{await fs.lstat(p);return true;}catch(e){if(e.code==='ENOENT')return false;throw e;}},
    async stat(p){
      let st;try{st=await fs.lstat(p);}catch(e){return missing(e);}
      return {type:st.isFile()?'regular':'other',size:st.size,permissions:st.mode&0o777};
    },
    async readUTF8(p){try{return await fs.readFile(p,'utf8');}catch(e){return missing(e);}},
    async createUniqueFile(parent,prefix,mode){
      assert.equal(mode,0o600);
      const tmp=path.join(parent,prefix);
      const handle=await fs.open(tmp,'wx',mode);await handle.close();return tmp;
    },
    async writeUTF8(p,value,opts){
      writes++;
      await fs.writeFile(opts.tmpPath,value,{flag:'w'});
      if(opts.backupFile)await fs.rename(p,opts.backupFile);
      await fs.rename(opts.tmpPath,p);
    },
    setPermissions:(p,mode)=>fs.chmod(p,mode)
  };
  const options={ioUtils,pathUtils:{profileDir:dir,join:path.join,isAbsolute:path.isAbsolute},
    developmentEnabled:true,assertProfileSecurity:async()=>allowed};
  return {primary,ioUtils,driver:()=>createGeckoProfileBookmarkDriver(options),
    inspector:()=>createGeckoBookmarkRecoveryInspector(options),
    writes:()=>writes,revoke:()=>{allowed=false;}};
}
test('real isolated files: valid write, backup and read-only healthy preview',async t=>{
  const f=await fixture(t),driver=f.driver();
  assert.equal(await driver.read(),null);
  await driver.write(data('one'));
  await driver.write(data('two'));
  assert.equal(await fs.readFile(f.primary,'utf8'),data('two'));
  assert.equal(await fs.readFile(f.primary+'.bak','utf8'),data('one'));
  assert.equal((await fs.lstat(f.primary)).mode&0o777,0o600);
  const before=await fs.readFile(f.primary);
  assert.equal((await f.inspector().inspect()).status,'healthy');
  assert.deepEqual(await fs.readFile(f.primary),before);
});
test('real isolated files: Gecko collision orphan blocks write and stays intact',async t=>{
  const f=await fixture(t),orphan=f.primary.replace(/\.json$/,'.json-2.tmp-');
  await fs.writeFile(f.primary,data('one'),{mode:0o600});
  await fs.writeFile(orphan,'{unfinished',{mode:0o600});
  const original=await fs.readFile(orphan);
  const driver=f.driver();await driver.read();
  await assert.rejects(driver.write(data('two')),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.writes(),0);
  assert.deepEqual(await fs.readFile(orphan),original);
  assert.equal(await fs.readFile(f.primary,'utf8'),data('one'));
  const preview=await f.inspector().inspect();
  assert.equal(preview.requiresManualReview,true);
  assert.deepEqual(preview.privateTemporaries,['corrupt-or-unreadable']);
});
test('real isolated files: revocation prevents inspection and write',async t=>{
  const f=await fixture(t);
  await fs.writeFile(f.primary,data('one'),{mode:0o600});
  const driver=f.driver();await driver.read();f.revoke();
  await assert.rejects(driver.write(data('two')),{code:'PROFILE_SECURITY_UNVERIFIED'});
  await assert.rejects(f.inspector().inspect(),{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(await fs.readFile(f.primary,'utf8'),data('one'));
});

test('real isolated files: symlinked recovery artifact is never read as a bookmark file',async t=>{
  if(process.platform==='win32'){t.skip('POSIX symlink behavior');return;}
  const f=await fixture(t);
  const target=f.primary+'.sentinel';
  const link=f.primary+'.tmp-';
  await fs.writeFile(target,'private-sentinel',{mode:0o600});
  await fs.symlink(target,link);
  const before=await fs.readFile(target);
  const preview=await f.inspector().inspect();
  assert.equal(preview.requiresManualReview,true);
  assert.deepEqual(preview.privateTemporaries,['not-regular']);
  await assert.rejects(f.driver().read(),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.writes(),0);
  assert.deepEqual(await fs.readFile(target),before);
});

test('real isolated files: world-readable bookmark is not scanned as healthy',async t=>{
  if(process.platform==='win32'){t.skip('POSIX chmod behavior');return;}
  const f=await fixture(t),contents=data('private');
  await fs.writeFile(f.primary,contents,{mode:0o600});
  await fs.chmod(f.primary,0o644);
  let reads=0;
  const read=f.ioUtils.readUTF8;
  f.ioUtils.readUTF8=async p=>{reads++;return read(p);};
  const original=await fs.readFile(f.primary);
  const inspected=await f.inspector().inspect();
  assert.equal(inspected.status,'recovery-review-required');
  assert.equal(inspected.primary,'private-mode-unverified');
  assert.equal(reads,0);
  assert.deepEqual(await fs.readFile(f.primary),original);
  assert.equal((await fs.lstat(f.primary)).mode&0o777,0o644);
});

test('real isolated files: insecure backup and temp remain untouched',async t=>{
  if(process.platform==='win32'){t.skip('POSIX chmod behavior');return;}
  const f=await fixture(t),backup=f.primary+'.bak',temp=f.primary+'.tmp-';
  await fs.writeFile(f.primary,data('one'),{mode:0o600});
  await fs.writeFile(backup,data('backup'),{mode:0o600});
  await fs.writeFile(temp,'{unfinished',{mode:0o600});
  await fs.chmod(backup,0o666);
  await fs.chmod(temp,0o644);
  const originalBackup=await fs.readFile(backup),originalTemp=await fs.readFile(temp);
  const r=await f.inspector().inspect();
  assert.equal(r.primary,'valid');
  assert.equal(r.backup,'private-mode-unverified');
  assert.deepEqual(r.privateTemporaries,['private-mode-unverified']);
  assert.equal(r.requiresManualReview,true);
  assert.deepEqual(await fs.readFile(backup),originalBackup);
  assert.deepEqual(await fs.readFile(temp),originalTemp);
});

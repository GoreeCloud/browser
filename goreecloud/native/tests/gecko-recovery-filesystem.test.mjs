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

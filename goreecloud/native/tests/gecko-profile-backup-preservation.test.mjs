// SPDX-License-Identifier: MPL-2.0
// Node temporary-filesystem contracts, not native Gecko IOUtils acceptance.
import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE} from '../bookmarks/gecko-profile-driver.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const data=id=>exportPortableBookmarks([{id,type:'bookmark',title:id,url:'https://example.org/'+id}]);
const missing=e=>{if(e?.code==='ENOENT')e.name='NotFoundError';throw e;};
async function fixture(t) {
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'goreecloud-backup-preserve-'));
  await fs.chmod(dir,0o700);
  t.after(async()=>fs.rm(dir,{recursive:true,force:true}));
  const primary=path.join(dir,GECKO_PROFILE_BOOKMARK_FILE), backup=primary+'.bak';
  let commits=0,backupReads=0,authorized=true;
  const ioUtils={
    getChildren:async p=>(await fs.readdir(p)).map(n=>path.join(p,n)),
    async exists(p){try{await fs.lstat(p);return true;}catch(e){if(e.code==='ENOENT')return false;throw e;}},
    async stat(p){let st;try{st=await fs.lstat(p);}catch(e){return missing(e);}return {type:st.isFile()?'regular':'other',size:st.size,permissions:st.mode&0o777};},
    async readUTF8(p){if(p===backup)backupReads++;try{return await fs.readFile(p,'utf8')}catch(e){return missing(e)}},
    async createUniqueFile(p,prefix,mode){const dest=path.join(p,prefix),h=await fs.open(dest,'wx',mode);await h.close();return dest;},
    async writeUTF8(p,content,opts){commits++;await fs.writeFile(opts.tmpPath,content);if(opts.backupFile)await fs.rename(p,opts.backupFile);await fs.rename(opts.tmpPath,p);},
    setPermissions:(p,mode)=>fs.chmod(p,mode),
  };
  const driver=createGeckoProfileBookmarkDriver({ioUtils,developmentEnabled:true,
    pathUtils:{profileDir:dir,join:path.join,isAbsolute:path.isAbsolute},
    assertProfileSecurity:async()=>authorized});
  return {primary,backup,ioUtils,driver,commits:()=>commits,backupReads:()=>backupReads,revoke:()=>{authorized=false;}};
}
async function seed(f,backupContent,backupMode=0o600) {
  await fs.writeFile(f.primary,data('primary'),{mode:0o600});
  await fs.writeFile(f.backup,backupContent,{mode:0o600});
  await fs.chmod(f.backup,backupMode);
  assert.equal(await f.driver.read(),data('primary'));
}

test('valid private backup rotates normally on successive writes',async t=>{
  const f=await fixture(t);
  assert.equal(await f.driver.read(),null);
  await f.driver.write(data('one'));
  await f.driver.write(data('two'));
  await f.driver.write(data('three'));
  assert.equal(await fs.readFile(f.primary,'utf8'),data('three'));
  assert.equal(await fs.readFile(f.backup,'utf8'),data('two'));
  assert.equal(f.commits(),3);
  assert.ok(f.backupReads()>=1);
});

test('corrupt backup is retained byte-for-byte and blocks replacement',async t=>{
  const f=await fixture(t), damaged='{interrupted\u0000';
  await seed(f,damaged);
  await assert.rejects(f.driver.write(data('changed')),{code:'RECOVERY_REQUIRED'});
  assert.equal(await fs.readFile(f.primary,'utf8'),data('primary'));
  assert.equal(await fs.readFile(f.backup,'utf8'),damaged);
  assert.equal(f.commits(),0);
  await assert.rejects(f.driver.write(data('changed')),{code:'READ_REQUIRED'});
});

test('world-readable backup is not read or overwritten',async t=>{
  if(process.platform==='win32'){t.skip('POSIX chmod semantics');return;}
  const f=await fixture(t);await seed(f,data('old'),0o644);
  await assert.rejects(f.driver.write(data('changed')),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
  assert.equal(await fs.readFile(f.backup,'utf8'),data('old'));
  assert.equal((await fs.lstat(f.backup)).mode&0o777,0o644);
});

test('symlinked backup is refused without touching its target',async t=>{
  if(process.platform==='win32'){t.skip('POSIX symlink semantics');return;}
  const f=await fixture(t), target=path.join(path.dirname(f.primary),'sentinel.bin');
  await fs.writeFile(f.primary,data('primary'),{mode:0o600});
  await fs.writeFile(target,'preserved',{mode:0o600});
  await fs.symlink(target,f.backup);
  await f.driver.read();
  await assert.rejects(f.driver.write(data('changed')),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
  assert.equal(await fs.readFile(target,'utf8'),'preserved');
  assert.equal((await fs.lstat(f.backup)).isSymbolicLink(),true);
});

test('oversized private backup is not read or overwritten',async t=>{
  const f=await fixture(t);
  await seed(f,'x'.repeat(2_000_001));
  await assert.rejects(f.driver.write(data('changed')),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
  assert.equal((await fs.stat(f.backup)).size,2_000_001);
});

test('backup read errors do not reveal sensitive absolute paths',async t=>{
  const f=await fixture(t);await seed(f,data('old'));
  const read=f.ioUtils.readUTF8;
  f.ioUtils.readUTF8=async p=>{if(p===f.backup)throw Error('/private/owner/sensitive');return read(p)};
  await assert.rejects(f.driver.write(data('changed')),e=>
    e.code==='RECOVERY_REQUIRED'&&!e.message.includes('/private/'));
  assert.equal(f.commits(),0);
  assert.equal(await fs.readFile(f.backup,'utf8'),data('old'));
});

test('metadata failure blocks replacement without leaking paths',async t=>{
  const f=await fixture(t);await seed(f,data('old'));
  const stat=f.ioUtils.stat;
  f.ioUtils.stat=async p=>{if(p===f.backup)throw Error('/private/owner/metadata');return stat(p)};
  await assert.rejects(f.driver.write(data('changed')),e=>
    e.code==='RECOVERY_REQUIRED'&&!e.message.includes('/private/'));
  assert.equal(f.commits(),0);
});

test('owner revoked during backup stat does not read backup or mutate storage',async t=>{
  const f=await fixture(t);await seed(f,data('old'));
  const orig=f.ioUtils.stat;
  let release, started;
  const startedP=new Promise(r=>{started=r});const wait=new Promise(r=>{release=r});
  f.ioUtils.stat=async p=>{if(p===f.backup){started();await wait;}return orig(p)};
  const attempt=f.driver.write(data('changed'));
  await Promise.race([startedP,new Promise((_,reject)=>setTimeout(()=>reject(Error('Backup stat preflight not reached')),500))]);
  f.revoke();release();
  await assert.rejects(attempt,{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
});

test('fresh profile cannot initialize over backup-only recovery evidence',async t=>{
  const f=await fixture(t);
  await fs.writeFile(f.backup,data('old'),{mode:0o600});
  await assert.rejects(f.driver.read(),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.commits(),0);
  assert.equal(await fs.readFile(f.backup,'utf8'),data('old'));
});

test('backup directory is preserved without attempting replacement',async t=>{
  const f=await fixture(t);
  await fs.writeFile(f.primary,data('primary'),{mode:0o600});
  await fs.mkdir(f.backup,{mode:0o700});
  assert.equal(await f.driver.read(),data('primary'));
  await assert.rejects(f.driver.write(data('changed')),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
  assert.equal((await fs.lstat(f.backup)).isDirectory(),true);
  assert.equal(await fs.readFile(f.primary,'utf8'),data('primary'));
});

test('missing backup permission metadata fails closed before reading backup',async t=>{
  const f=await fixture(t);await seed(f,data('old'));
  const originalStat=f.ioUtils.stat;
  f.ioUtils.stat=async p=>p===f.backup
    ? {...await originalStat(p),permissions:undefined}
    : originalStat(p);
  await assert.rejects(f.driver.write(data('changed')),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
  assert.equal(await fs.readFile(f.backup,'utf8'),data('old'));
});

test('backup presence errors fail closed without leaking sensitive paths',async t=>{
  const f=await fixture(t);await seed(f,data('old'));
  const originalExists=f.ioUtils.exists;
  f.ioUtils.exists=async p=>{if(p===f.backup)throw Error('/secret/profile/recovery');
    return originalExists(p);};
  await assert.rejects(f.driver.write(data('changed')),e=>
    e.code==='RECOVERY_REQUIRED'&&!e.message.includes('/secret/'));
  assert.equal(f.backupReads(),0);
  assert.equal(f.commits(),0);
  assert.equal(await fs.readFile(f.backup,'utf8'),data('old'));
});

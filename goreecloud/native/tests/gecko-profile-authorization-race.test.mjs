// SPDX-License-Identifier: MPL-2.0
// Synthetic async race tests. Not a real Gecko profile lock or native runtime.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE}
  from '../bookmarks/gecko-profile-driver.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const dir='/isolated/bookmark-guard-recheck';
const file=dir+'/'+GECKO_PROFILE_BOOKMARK_FILE;
const document=exportPortableBookmarks([
  {id:'safe',type:'bookmark',title:'Safe',url:'https://example.org/safe'}
]);
function deferred() {
  let resolve;
  const promise=new Promise(r=>{resolve=r;});
  return {promise,resolve};
}
function fixture() {
  let authorized=true,reads=0,writes=0;
  let heldStat=null;
  const ioUtils={
    async stat(path) {
      assert.equal(path,file);
      const hold=heldStat;
      if(hold) {
        heldStat=null;
        hold.started.resolve();
        await hold.release.promise;
      }
      return {type:'regular',size:Buffer.byteLength(document),permissions:0o600};
    },
    async readUTF8(path) {
      assert.equal(path,file);
      reads++;
      return document;
    },
    async exists(path) {return path===file;},
    async getChildren() {return [file];},
    async createUniqueFile() {writes++;return file+'.tmp-';},
    async writeUTF8() {writes++;},
    async setPermissions() {},
  };
  const driver=createGeckoProfileBookmarkDriver({
    ioUtils,developmentEnabled:true,
    pathUtils:{profileDir:dir,join:(...p)=>p.join('/'),
      isAbsolute:p=>p.startsWith('/')},
    assertProfileSecurity:async()=>authorized,
  });
  return {
    driver,
    holdStat() {
      const next={started:deferred(),release:deferred()};
      heldStat=next;
      return next;
    },
    revoke(){authorized=false;},
    restore(){authorized=true;},
    reads(){return reads;},
    writes(){return writes;},
  };
}

test('revoked owner during async stat cannot read any private bookmark bytes',async()=>{
  const f=fixture(),hold=f.holdStat();
  const result=f.driver.read();
  await hold.started.promise;
  f.revoke();
  hold.release.resolve();
  await assert.rejects(result,{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(f.reads(),0);
  assert.equal(f.writes(),0);
  await assert.rejects(f.driver.write(document),{code:'PROFILE_SECURITY_UNVERIFIED'});
});

test('revocation during existing-file write preflight stat blocks re-read and mutation',async()=>{
  const f=fixture();
  assert.equal(await f.driver.read(),document);
  assert.equal(f.reads(),1);
  const hold=f.holdStat(),result=f.driver.write(document);
  await hold.started.promise;
  f.revoke();
  hold.release.resolve();
  await assert.rejects(result,{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(f.reads(),1,'No second read after authorization revoked');
  assert.equal(f.writes(),0,'No temp creation or commit');
  f.restore();
  await assert.rejects(f.driver.write(document),{code:'READ_REQUIRED'});
});

test('authorized existing bookmark reads succeed without the race',async()=>{
  const f=fixture();
  assert.equal(await f.driver.read(),document);
  assert.equal(f.reads(),1);
  assert.equal(f.writes(),0);
});

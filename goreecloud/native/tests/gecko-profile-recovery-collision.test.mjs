// SPDX-License-Identifier: MPL-2.0
// Source-level recovery scan regressions, not privileged Gecko runtime.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE} from '../bookmarks/gecko-profile-driver.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const profile='/test/browser-profile';
const file=profile+'/'+GECKO_PROFILE_BOOKMARK_FILE;
const data=exportPortableBookmarks([{id:'a',title:'A',type:'bookmark',url:'https://example.org/a'}]);

function fake() {
  const files=new Map();
  let writes=0;
  const ioUtils={
    async exists(p){return files.has(p);},
    async stat(p){
      if(!files.has(p))throw Object.assign(new Error('missing'),{name:'NotFoundError'});
      return {type:'regular',size:Buffer.byteLength(files.get(p)),permissions:0o600};
    },
    async readUTF8(p){return files.get(p);},
    async getChildren(){return [...files.keys()];},
    async createUniqueFile(){writes++;return file+'.tmp-';},
    async writeUTF8(){writes++;},
    async setPermissions(){}
  };
  const driver=()=>createGeckoProfileBookmarkDriver({ioUtils,developmentEnabled:true,
    assertProfileSecurity:async()=>true,
    pathUtils:{profileDir:profile,join:(...x)=>x.join('/'),isAbsolute:p=>p.startsWith('/')},
  });
  return {files,ioUtils,driver,writes:()=>writes};
}
const collision=n=>file.replace(/\.json$/,'.json-'+n+'.tmp-');

test('numeric Gecko collision artifact blocks new profile initialization',async()=>{
  const f=fake(),path=collision(1);f.files.set(path,data);
  await assert.rejects(f.driver().read(),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.files.get(path),data);
  assert.equal(f.writes(),0);
});
test('numeric collision artifact blocks write without modifying primary',async()=>{
  const f=fake();f.files.set(file,data);f.files.set(collision(9),'{pending');
  const d=f.driver();assert.equal(await d.read(),data);
  await assert.rejects(d.write(data),{code:'RECOVERY_REQUIRED'});
  assert.equal(f.files.get(file),data);
  assert.equal(f.files.get(collision(9)),'{pending');
  assert.equal(f.writes(),0);
});
test('recovery inventory refuses oversized listing without any write',async()=>{
  const f=fake();const d=f.driver();await d.read();
  f.ioUtils.getChildren=async()=>Array.from({length:25001},(_,i)=>profile+'/unrelated'+i);
  await assert.rejects(d.write(data),{code:'PROFILE_STORE_UNAVAILABLE'});
  assert.equal(f.writes(),0);
});
test('recovery inventory errors do not expose privileged filesystem paths',async()=>{
  const f=fake();
  f.ioUtils.getChildren=async()=>{throw Error('sensitive /real/profile/location');};
  await assert.rejects(f.driver().read(),err=>
    err.code==='PROFILE_STORE_UNAVAILABLE' && !err.message.includes('/real/profile'));
  assert.equal(f.writes(),0);
});

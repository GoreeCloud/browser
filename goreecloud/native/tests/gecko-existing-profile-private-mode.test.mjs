// SPDX-License-Identifier: MPL-2.0
// In-memory Gecko IOUtils contracts; not native Firefox/profile-lock evidence.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE}
  from '../bookmarks/gecko-profile-driver.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const dir='/isolated/driver-private-file';
const base=dir+'/'+GECKO_PROFILE_BOOKMARK_FILE;
const text=exportPortableBookmarks([{id:'safe',type:'bookmark',title:'Safe',url:'https://example.org/safe'}]);
function fixture(mode) {
  const files=new Map([[base,text]]);
  let reads=0,writes=0,fileMode=mode;
  const ioUtils={
    async stat(p) {
      if(!files.has(p))throw Object.assign(Error('not found'),{name:'NotFoundError'});
      return {type:'regular',size:Buffer.byteLength(files.get(p)),permissions:fileMode};
    },
    async readUTF8(p){reads++;return files.get(p);},
    async exists(p){return files.has(p);},
    async getChildren(){return [...files.keys()];},
    async createUniqueFile(){writes++;return base+'.tmp-';},
    async writeUTF8(){writes++;},
    async setPermissions(){},
  };
  const driver=createGeckoProfileBookmarkDriver({ioUtils,developmentEnabled:true,
    pathUtils:{profileDir:dir,join:(...parts)=>parts.join('/'),isAbsolute:p=>p.startsWith('/')},
    assertProfileSecurity:async()=>true});
  return {driver,files,setMode:next=>{fileMode=next;},reads:()=>reads,writes:()=>writes};
}

for(const [name,mode] of [
  ['group-readable',0o640],['world-readable',0o644],
  ['Windows-style generic',0o666],['missing',undefined],
  ['unrecognized high bits',0o1600],['invalid negative',-1],
]) {
  test('rejects '+name+' primary before any bookmark data read',async()=>{
    const f=fixture(mode);
    await assert.rejects(f.driver.read(),{code:'PRIVATE_PROFILE_UNVERIFIED'});
    assert.equal(f.reads(),0);
    assert.equal(f.writes(),0);
    assert.equal(f.files.get(base),text);
  });
}

test('previously private bookmark file cannot be replaced after chmod drift',async()=>{
  const f=fixture(0o600);
  assert.equal(await f.driver.read(),text);
  f.setMode(0o644);
  await assert.rejects(f.driver.write(text),{code:'PRIVATE_PROFILE_UNVERIFIED'});
  assert.equal(f.reads(),1);
  assert.equal(f.writes(),0);
  assert.equal(f.files.get(base),text);
  await assert.rejects(f.driver.write(text),{code:'READ_REQUIRED'});
});

test('properly private existing bookmarks can still be read',async()=>{
  const f=fixture(0o600);
  assert.equal(await f.driver.read(),text);
  assert.equal(f.reads(),1);
  assert.equal(f.writes(),0);
});

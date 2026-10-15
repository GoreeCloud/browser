// SPDX-License-Identifier: MPL-2.0
// Synthetic IOUtils permission contracts, not actual privileged Gecko tests.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE} from '../bookmarks/gecko-profile-driver.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const bookmark={id:'secret',type:'bookmark',title:'secret',url:'https://example.org/secret'};
const document=exportPortableBookmarks([bookmark]);

function fixture(mode) {
  const profile='/isolated/profile';
  const primary=profile+'/'+GECKO_PROFILE_BOOKMARK_FILE;
  const temp=primary+'.tmp-1';
  const paths=new Map();
  let attemptedWrites=0;
  const ioUtils={
    async exists(path){return paths.has(path);},
    async getChildren(parent){return [...paths.keys()].filter(path=>path.startsWith(parent+'/'));},
    async stat(path) {
      if(!paths.has(path)) throw Object.assign(new Error('absent'),{name:'NotFoundError'});
      return {type:'regular',size:paths.get(path).length,permissions:mode};
    },
    async readUTF8(path){return paths.get(path);},
    async createUniqueFile(_dir,_prefix,perms) {
      assert.equal(perms,0o600);
      paths.set(temp,'');
      return temp;
    },
    async writeUTF8(){attemptedWrites++;},
    async setPermissions(){},
  };
  const driver=createGeckoProfileBookmarkDriver({
    ioUtils,developmentEnabled:true,
    pathUtils:{profileDir:profile,isAbsolute:p=>p.startsWith('/'),join:(...p)=>p.join('/')},
    assertProfileSecurity:async()=>true,
  });
  return {driver,paths,temp,writes:()=>attemptedWrites};
}

for(const [description,mode] of [
  ['group-readable mode',0o640],
  ['world-readable mode',0o644],
  ['Windows-style permission report',0o666],
  ['missing permission metadata',undefined],
]) {
  test('refuses '+description+' before writing bookmark contents',async()=>{
    const e=fixture(mode);
    assert.equal(await e.driver.read(),null);
    await assert.rejects(e.driver.write(document),{code:'PRIVATE_TEMP_UNVERIFIED'});
    assert.equal(e.writes(),0);
    assert.equal(e.paths.get(e.temp),'');
    await assert.rejects(e.driver.write(document),{code:'READ_REQUIRED'});
  });
}

test('private 0600 mode allows a write',async()=>{
  const e=fixture(0o600);
  assert.equal(await e.driver.read(),null);
  await e.driver.write(document);
  assert.equal(e.writes(),1);
});

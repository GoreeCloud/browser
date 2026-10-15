// SPDX-License-Identifier: MPL-2.0
// Contract checks for untrusted directory inventories; not Gecko runtime.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createGeckoProfileBookmarkDriver,GECKO_PROFILE_BOOKMARK_FILE} from '../bookmarks/gecko-profile-driver.mjs';
import {createGeckoBookmarkRecoveryInspector} from '../bookmarks/gecko-recovery-inspector.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const dir='/test/locked-profile', primary=dir+'/'+GECKO_PROFILE_BOOKMARK_FILE;
const stored=exportPortableBookmarks([{id:'safe',type:'bookmark',title:'A',url:'https://example.org/'}]);

function fixture() {
  const files=new Map(), listing=[];
  let writes=0;
  const ioUtils={
    async stat(p) {
      if(!files.has(p)) throw Object.assign(Error('not found'),{name:'NotFoundError'});
      return {type:'regular',size:Buffer.byteLength(files.get(p)),permissions:0o600};
    },
    async readUTF8(p){return files.get(p);},
    async exists(p){return files.has(p);},
    async getChildren(){return listing;},
    async createUniqueFile(){writes++;return primary+'.tmp-';},
    async writeUTF8(){writes++;},
    async setPermissions(){},
  };
  const args={ioUtils,pathUtils:{profileDir:dir,
    join:(...parts)=>parts.join('/'),isAbsolute:p=>p.startsWith('/')},
    assertProfileSecurity:async()=>true,developmentEnabled:true};
  return {files,listing,driver:()=>createGeckoProfileBookmarkDriver(args),
    inspector:()=>createGeckoBookmarkRecoveryInspector(args),writes:()=>writes};
}

for(const invalid of ['/other-profile/unrelated','/test/locked-profile-other/file',
  '/test/locked-profile/nested/secret','/test/locked-profile/../outside',
  '/test/locked-profile/.','/test/locked-profile/..']) {
  test('refuses inconsistent inventory entry without revealing or writing: '+invalid,async()=>{
    const f=fixture();
    f.listing.push(invalid);
    await assert.rejects(f.driver().read(),{code:'PROFILE_STORE_UNAVAILABLE'});
    await assert.rejects(f.inspector().inspect(),{code:'UNVERIFIABLE_DIRECTORY'});
    assert.equal(f.writes(),0);
    assert.equal(f.files.size,0);
  });
}

test('driver blocks unsafe inventory after a previously valid read',async()=>{
  const f=fixture();
  f.files.set(primary,stored);
  const d=f.driver();
  assert.equal(await d.read(),stored);
  f.listing.push(dir+'/nested/escape');
  await assert.rejects(d.write(stored),{code:'PROFILE_STORE_UNAVAILABLE'});
  assert.equal(f.files.get(primary),stored);
  assert.equal(f.writes(),0);
  await assert.rejects(d.write(stored),{code:'READ_REQUIRED'});
});

test('direct children are accepted by the guard without automatic restoration',async()=>{
  const f=fixture();
  f.files.set(primary,stored);
  f.listing.push(primary);
  const preview=await f.inspector().inspect();
  assert.equal(preview.status,'healthy');
  assert.equal(preview.primary,'valid');
  assert.equal(f.files.get(primary),stored);
  assert.equal(f.writes(),0);
});

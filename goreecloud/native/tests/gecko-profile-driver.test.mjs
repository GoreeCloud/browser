// SPDX-License-Identifier: MPL-2.0
import test from 'node:test';
import assert from 'node:assert/strict';
import { BookmarkStore } from '../bookmarks/store.mjs';
import { exportPortableBookmarks } from '../bookmarks/portable.mjs';
import { createGeckoProfileBookmarkDriver, GECKO_PROFILE_BOOKMARK_FILE } from '../bookmarks/gecko-profile-driver.mjs';

const profile='/synthetic/profile', file=profile+'/'+GECKO_PROFILE_BOOKMARK_FILE;
const bookmark=id=>({id,title:id,type:'bookmark',url:'https://example.org/'+id});
const missing=()=>Object.assign(new Error('Missing file'),{name:'NotFoundError'});
function fakeGecko() {
  const files=new Map(), events=[];
  let failChmod=false,vanishOnRead=false;
  const pathUtils={profileDir:profile,isAbsolute:p=>p.startsWith('/'),join:(...parts)=>parts.join('/')};
  const ioUtils={
    async exists(p) {return files.has(p);},
    async stat(p) {if(!files.has(p))throw missing();return {type:'regular',size:new TextEncoder().encode(files.get(p)).byteLength};},
    async readUTF8(p) {if(vanishOnRead||!files.has(p))throw missing();return files.get(p);},
    async writeUTF8(p,text,opts) {
      events.push(['write',p,{...opts}]);
      if(files.has(p)&&opts.backupFile)files.set(opts.backupFile,files.get(p));
      files.set(p,text);
    },
    async setPermissions(p,mode,honorUmask) {
      events.push(['chmod',p,mode,honorUmask]);
      if(failChmod){failChmod=false;throw Error('chmod failed after commit');}
    },
  };
  return {files,events,pathUtils,ioUtils,
    failNextChmod(){failChmod=true;},failNextRead(){vanishOnRead=true;}};
}
const driverFor=env=>createGeckoProfileBookmarkDriver({
  ioUtils:env.ioUtils,pathUtils:env.pathUtils,developmentEnabled:true,
});

test('gated profile driver rejects disabled and invalid environments',()=>{
  const e=fakeGecko();
  assert.throws(()=>createGeckoProfileBookmarkDriver({ioUtils:e.ioUtils,pathUtils:e.pathUtils}),{code:'FEATURE_DISABLED'});
  assert.throws(()=>createGeckoProfileBookmarkDriver({ioUtils:{},pathUtils:e.pathUtils,developmentEnabled:true}),{code:'INVALID_PRIVILEGED_ADAPTER'});
});

test('missing profile file is a new local collection',async()=>{
  const e=fakeGecko();assert.equal(await driverFor(e).read(),null);assert.equal(e.files.size,0);
});

test('read-before-write and malformed input are rejected',async()=>{
  const e=fakeGecko(),d=driverFor(e);
  await assert.rejects(d.write(exportPortableBookmarks([bookmark('a')])),{code:'READ_REQUIRED'});
  await d.read();
  await assert.rejects(d.write('{not-json'));
  assert.equal(e.files.size,0);
});
test('save and reopen preserve local data with atomic write options',async()=>{
  const e=fakeGecko(),s=new BookmarkStore(driverFor(e));
  await s.add({item:bookmark('one')});
  assert.deepEqual(e.events.find(x=>x[0]==='write')[2],
    {tmpPath:file+'.tmp',flush:true,mode:'overwrite'});
  assert.ok(e.events.some(x=>x[0]==='chmod'&&x[1]===file&&x[2]===0o600));
  const reopened=new BookmarkStore(driverFor(e));
  assert.deepEqual((await reopened.snapshot()).nodes.map(x=>x.id),['one']);
});

test('replacements retain prior version in a private backup',async()=>{
  const e=fakeGecko(),s=new BookmarkStore(driverFor(e));
  await s.add({item:bookmark('first')});
  const original=e.files.get(file);
  await s.add({item:bookmark('second')});
  assert.equal(e.files.get(file+'.bak'),original);
  assert.equal(e.events.filter(x=>x[0]==='write')[1][2].backupFile,file+'.bak');
  assert.ok(e.events.some(x=>x[0]==='chmod'&&x[1]===file+'.bak'&&x[2]===0o600));
});
test('orphaned backup or temporary data blocks creating a new store',async()=>{
  for(const name of [file+'.bak',file+'.tmp']){
    const e=fakeGecko();
    e.files.set(name,exportPortableBookmarks([bookmark('old')]));
    const store=new BookmarkStore(driverFor(e));
    await assert.rejects(store.snapshot(),{code:'RECOVERY_REQUIRED'});
    await assert.rejects(store.add({item:bookmark('new')}),{code:'RECOVERY_REQUIRED'});
    assert.equal(e.files.has(file),false);
  }
});
test('corrupt data is never treated as an empty new profile',async()=>{
  const e=fakeGecko();
  e.files.set(file,'{corrupt');
  const s=new BookmarkStore(driverFor(e));
  await assert.rejects(s.snapshot());
  await assert.rejects(s.add({item:bookmark('new')}));
  assert.equal(e.files.get(file),'{corrupt');
});

test('oversized and non-regular profile data fail before mutation',async()=>{
  const e=fakeGecko();
  e.files.set(file,'x'.repeat(2_000_001));
  await assert.rejects(driverFor(e).read(),{code:'INVALID_PROFILE_FILE'});
  e.ioUtils.stat=async()=>({type:'directory',size:0});
  await assert.rejects(driverFor(e).read(),{code:'INVALID_PROFILE_FILE'});
  assert.equal(e.events.filter(x=>x[0]==='write').length,0);
});
test('disappearing profile file is a race, not a new profile',async()=>{
  const e=fakeGecko();
  e.files.set(file,exportPortableBookmarks([bookmark('a')]));
  e.failNextRead();
  await assert.rejects(driverFor(e).read(),{code:'PROFILE_STORE_CHANGED'});
});
test('permission error after durable commit forces cache reload',async()=>{
  const e=fakeGecko(),s=new BookmarkStore(driverFor(e));
  const initial=await s.snapshot();
  e.failNextChmod();
  await assert.rejects(s.add({item:bookmark('persisted')}),/chmod failed/);
  const after=await s.snapshot();
  assert.ok(after.revision>initial.revision);
  assert.deepEqual(after.nodes.map(x=>x.id),['persisted']);
  await s.add({item:bookmark('next')});
  assert.deepEqual((await s.snapshot()).nodes.map(x=>x.id),['persisted','next']);
});

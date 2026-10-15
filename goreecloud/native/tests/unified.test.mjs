// SPDX-License-Identifier: MPL-2.0
import test from 'node:test';
import assert from 'node:assert/strict';
import { BOOKMARK_LOCATIONS, UnifiedBookmarks, UnifiedBookmarkError, previewFirefoxLegacyBookmarks } from '../bookmarks/unified.mjs';
import { importPortableBookmarks, exportPortableBookmarks } from '../bookmarks/portable.mjs';

const bookmark = (id) => ({ id, title: id, url: 'https://example.org', type: 'bookmark' });
function fakeStore() {
  let nodes = [], writes = 0;
  const store = {
    snapshot: async () => ({ revision: writes, nodes: structuredClone(nodes) }),
    add: async ({ item }) => { nodes.push(item); writes++; return store.snapshot(); },
    rename: async ({ id, title }) => { nodes.find(x=>x.id===id).title = title; writes++; return store.snapshot(); },
    move: async () => { writes++; return store.snapshot(); },
    remove: async ({ id }) => { nodes = nodes.filter(x=>x.id!==id); writes++; return store.snapshot(); },
    exportJSON: async () => exportPortableBookmarks(nodes),
    importJSON: async ({ text, allowReplace }) => { if(!allowReplace) throw Error('Consent required'); nodes=importPortableBookmarks(text).nodes; writes++; return store.snapshot(); },
    get writes() { return writes; }
  };
  return store;
}

test('exactly two labelled bookmark locations in fixed order', () => {
  const b = new UnifiedBookmarks({ local: fakeStore() });
  assert.deepEqual(b.locations(), [
    { id: 'local', label: 'Local Bookmarks', available: true },
    { id: 'goreecloud', label: 'GoreeCloud Bookmarks', available: false },
  ]);
});
test('saving locally works with no account or cloud adapter', async () => {
  const b = new UnifiedBookmarks({local:fakeStore()});
  const result = await b.save({location:BOOKMARK_LOCATIONS.LOCAL,item:bookmark('a')});
  assert.equal(result.location, 'local');
  assert.equal((await b.snapshot('local')).nodes[0].id, 'a');
});
test('save cannot silently select a default or third location', async () => {
  const local = fakeStore(); const b = new UnifiedBookmarks({local});
  await assert.rejects(b.save({item:bookmark('a')}),{code:'INVALID_LOCATION'});
  await assert.rejects(b.save({location:'toolbar',item:bookmark('a')}),{code:'INVALID_LOCATION'});
  assert.equal(local.writes,0);
});
test('cloud save fails closed without authenticated cloud adapter', async () => {
  const local=fakeStore();const b=new UnifiedBookmarks({local});
  await assert.rejects(b.save({location:'goreecloud',item:bookmark('a')}),{code:'CLOUD_UNAVAILABLE'});
  assert.equal(local.writes,0);
});
test('cloud adapter present but signed-out stays unavailable', async () => {
  const local=fakeStore(),cloud=fakeStore();const b=new UnifiedBookmarks({local,cloud});
  await assert.rejects(b.save({location:'goreecloud',item:bookmark('a')}),{code:'CLOUD_UNAVAILABLE'});
  assert.equal(cloud.writes,0);
});
test('an enabled cloud selection routes exclusively to its own adapter', async () => {
  const local=fakeStore(),cloud=fakeStore();
  const b=new UnifiedBookmarks({local,cloud,hasCloudSession:()=>true});
  await b.save({location:'goreecloud',item:bookmark('b')});
  assert.deepEqual((await b.snapshot('local')).nodes,[]);
  assert.equal((await b.snapshot('goreecloud')).nodes[0].id,'b');
  assert.equal(b.locations()[1].available,true);
});
test('changing session state immediately removes cloud access', async () => {
  let signedIn=true; const cloud=fakeStore();
  const b=new UnifiedBookmarks({local:fakeStore(),cloud,hasCloudSession:()=>signedIn});
  await b.save({location:'goreecloud',item:bookmark('one')});
  signedIn=false;
  await assert.rejects(b.snapshot('goreecloud'),{code:'CLOUD_UNAVAILABLE'});
  assert.equal(b.locations()[1].available,false);
});
test('deletions and imports stay within explicitly selected store', async () => {
  const local=fakeStore(),cloud=fakeStore();
  const b=new UnifiedBookmarks({local,cloud,hasCloudSession:()=>true});
  await b.save({location:'local',item:bookmark('local')});
  await b.save({location:'goreecloud',item:bookmark('cloud')});
  await b.remove({location:'local',id:'local'});
  assert.deepEqual((await b.snapshot('local')).nodes,[]);
  assert.equal((await b.snapshot('goreecloud')).nodes[0].id,'cloud');
  await b.importJSON({location:'local',text:exportPortableBookmarks([bookmark('restored')]),allowReplace:true});
  assert.equal((await b.snapshot('local')).nodes[0].id,'restored');
});
test('legacy Firefox roots flatten into Local Bookmarks without extra top-level categories',()=>{
  const result=previewFirefoxLegacyBookmarks({
    toolbar:[bookmark('1'),{id:'f',title:'Research',type:'folder',children:[bookmark('2')]}],
    menu:[bookmark('3')],other:[bookmark('4')],mobile:[bookmark('5')],
  });
  assert.equal(result.location,'local');
  assert.equal(result.itemCount,6);
  assert.deepEqual(result.counts,{toolbar:3,menu:1,other:1,mobile:1});
  const imported=importPortableBookmarks(result.json);
  assert.equal(imported.nodes.length,5);
  assert.equal(imported.nodes[1].title,'Research');
  assert.equal(imported.nodes[1].children[0].url,'https://example.org');
  assert.equal(new Set(imported.nodes.map(x=>x.id)).size,5);
});
test('root migration does not write data or assign anything to cloud',()=>{
  const result=previewFirefoxLegacyBookmarks({toolbar:[bookmark('a')]});
  assert.equal(result.location,'local');
  assert.equal(result.itemCount,1);
  assert.equal(result.json.includes('GoreeCloud Bookmarks'),false);
});
test('duplicate source IDs across distinct legacy roots are safely namespaced',()=>{
  const imported=importPortableBookmarks(previewFirefoxLegacyBookmarks({menu:[bookmark('same')],toolbar:[bookmark('same')]}).json);
  assert.notEqual(imported.nodes[0].id,imported.nodes[1].id);
});
test('rejects malformed roots and URL schemes before migration',()=>{
  assert.throws(()=>previewFirefoxLegacyBookmarks({readingList:[]}),UnifiedBookmarkError);
  assert.throws(()=>previewFirefoxLegacyBookmarks({toolbar:[{...bookmark('a'),url:'javascript:alert(1)'}]}));
  assert.throws(()=>previewFirefoxLegacyBookmarks(null),UnifiedBookmarkError);
});
test('empty migration preview is valid',()=>{
  const preview=previewFirefoxLegacyBookmarks({});
  assert.equal(preview.itemCount,0);
  assert.deepEqual(importPortableBookmarks(preview.json).nodes,[]);
});
test('rejects structurally invalid stores and cloud session hooks',()=>{
  assert.throws(()=>new UnifiedBookmarks({local:{}}),UnifiedBookmarkError);
  assert.throws(()=>new UnifiedBookmarks({local:fakeStore(),cloud:{}}),UnifiedBookmarkError);
  assert.throws(()=>new UnifiedBookmarks({local:fakeStore(),hasCloudSession:true}),UnifiedBookmarkError);
});

test('cloud session exceptions fail closed without breaking local bookmarks',async()=>{
  const local=fakeStore(),cloud=fakeStore();
  const b=new UnifiedBookmarks({local,cloud,hasCloudSession:()=>{throw Error('session expired')}});
  assert.equal(b.locations()[0].available,true);
  assert.equal(b.locations()[1].available,false);
  await assert.rejects(b.save({location:'goreecloud',item:bookmark('bad')}),{code:'CLOUD_UNAVAILABLE'});
  await b.save({location:'local',item:bookmark('ok')});
  assert.equal(local.writes,1);
  assert.equal(cloud.writes,0);
});
test('adapter responses cannot spoof the selected bookmark location',async()=>{
  const local=fakeStore(),cloud=fakeStore();
  local.snapshot=async()=>({location:'goreecloud',revision:0,nodes:[]});
  cloud.add=async()=>({location:'local',revision:1,nodes:[bookmark('cloud')]});
  const b=new UnifiedBookmarks({local,cloud,hasCloudSession:()=>true});
  assert.equal((await b.snapshot('local')).location,'local');
  assert.equal((await b.save({location:'goreecloud',item:bookmark('cloud')})).location,'goreecloud');
});
test('malformed adapter responses fail instead of producing false success',async()=>{
  const local=fakeStore();
  local.snapshot=async()=>null;
  const b=new UnifiedBookmarks({local});
  await assert.rejects(b.snapshot('local'),{code:'INVALID_ADAPTER_RESULT'});
});

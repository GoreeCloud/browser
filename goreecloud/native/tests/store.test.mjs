// SPDX-License-Identifier: MPL-2.0
import test from 'node:test';
import assert from 'node:assert/strict';
import { BookmarkStore, BookmarkStoreError } from '../bookmarks/store.mjs';
import { exportPortableBookmarks, importPortableBookmarks } from '../bookmarks/portable.mjs';

const bookmark = (id) => ({ id, type: 'bookmark', title: id, url: 'https://example.org/' + id });
const folder = (id, children = []) => ({ id, type: 'folder', title: id, children });

function memoryDriver(initial = null) {
  let text = initial;
  let reads = 0, writes = 0, failure = null;
  return {
    async read() { reads++; return text; },
    async write(value) {
      writes++;
      if (failure === 'before') { failure = null; throw Error('write rejected before commit'); }
      if (failure === 'after') { text = value; failure = null; throw Error('lost acknowledgement after commit'); }
      text = value;
    },
    failNextWrite(stage) { failure = stage; },
    get reads() { return reads; },
    get writes() { return writes; },
    get data() { return text; },
    set data(value) { text = value; },
  };
}

test('fresh profiles support offline local CRUD with independent snapshots', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  const initial = await store.snapshot();
  assert.deepEqual(initial.nodes, []);
  assert.equal(driver.writes, 0);
  const inserted = await store.add({ item: bookmark('a'), expectedRevision: initial.revision });
  assert.equal(inserted.revision, 1);
  inserted.nodes[0].title = 'tampered';
  assert.equal((await store.snapshot()).nodes[0].title, 'a');
  assert.equal(importPortableBookmarks(driver.data).nodes[0].id, 'a');
});

test('folders support nested add, rename, move and confirmed removal', async () => {
  const store = new BookmarkStore(memoryDriver());
  await store.add({ item: folder('group') });
  await store.add({ item: bookmark('a'), parentId: 'group' });
  await store.rename({ id: 'a', title: 'Renamed' });
  await store.move({ id: 'a', parentId: null });
  assert.equal((await store.snapshot()).nodes[0].children.length, 0);
  assert.equal((await store.snapshot()).nodes[1].title, 'Renamed');
  await store.remove({ id: 'group' });
  assert.deepEqual((await store.snapshot()).nodes.map(x => x.id), ['a']);
});

test('deleting nonempty folders requires explicit recursive consent', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  await store.add({ item: folder('group', [bookmark('child')]) });
  const before = await store.snapshot(), writes = driver.writes;
  await assert.rejects(store.remove({ id: 'group' }), BookmarkStoreError);
  assert.equal(driver.writes, writes);
  assert.deepEqual((await store.snapshot()), before);
  await store.remove({ id: 'group', allowRecursive: true });
  assert.equal((await store.snapshot()).nodes.length, 0);
});

test('moving a folder into itself or descendants fails without a write', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  await store.add({ item: folder('parent', [folder('child')]) });
  const writes = driver.writes;
  await assert.rejects(store.move({ id: 'parent', parentId: 'child' }), BookmarkStoreError);
  await assert.rejects(store.move({ id: 'parent', parentId: 'parent' }), BookmarkStoreError);
  assert.equal(driver.writes, writes);
});

test('invalid destinations, duplicate descendant IDs and active schemes never write', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  await store.add({ item: bookmark('a') });
  const writes = driver.writes;
  await assert.rejects(store.add({ item: bookmark('b'), parentId: 'missing' }));
  await assert.rejects(store.add({ item: folder('p', [bookmark('a')]) }));
  await assert.rejects(store.add({ item: { ...bookmark('bad'), url: 'javascript:alert(1)' } }));
  await assert.rejects(store.add({ item: bookmark('b'), index: -1 }));
  assert.equal(driver.writes, writes);
});

test('optimistic revisions reject stale mutations before writing', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  const initial = await store.snapshot();
  await store.add({ item: bookmark('a'), expectedRevision: initial.revision });
  const writes = driver.writes;
  await assert.rejects(store.rename({ id: 'a', title: 'stale', expectedRevision: initial.revision }), BookmarkStoreError);
  assert.equal(driver.writes, writes);
  assert.equal((await store.snapshot()).nodes[0].title, 'a');
});

test('imports need explicit replacement approval and do not corrupt existing data', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  await store.add({ item: bookmark('old') });
  const writes = driver.writes;
  await assert.rejects(store.importJSON({ text: exportPortableBookmarks([bookmark('new')]) }), BookmarkStoreError);
  await assert.rejects(store.importJSON({ text: '{not json', allowReplace: true }));
  assert.equal(driver.writes, writes);
  await store.importJSON({ text: exportPortableBookmarks([bookmark('new')]), allowReplace: true });
  assert.deepEqual(importPortableBookmarks(await store.exportJSON()).nodes.map(x => x.id), ['new']);
});

test('corrupt on-disk bookmark data remains untouched and fails closed', async () => {
  const driver = memoryDriver('{damaged');
  const store = new BookmarkStore(driver);
  await assert.rejects(store.snapshot());
  await assert.rejects(store.add({ item: bookmark('new') }));
  assert.equal(driver.data, '{damaged');
  assert.equal(driver.writes, 0);
});

test('invalid driver read result never initializes an empty replacement store', async () => {
  const driver = memoryDriver();
  driver.read = async () => undefined;
  const store = new BookmarkStore(driver);
  await assert.rejects(store.snapshot(), BookmarkStoreError);
  await assert.rejects(store.add({ item: bookmark('new') }), BookmarkStoreError);
  assert.equal(driver.writes, 0);
});

test('write rejection before commit invalidates stale tokens, then safely reloads', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  const prior = await store.snapshot();
  driver.failNextWrite('before');
  await assert.rejects(store.add({ item: bookmark('a'), expectedRevision: prior.revision }), /before commit/);
  const recovered = await store.snapshot();
  assert.equal(recovered.revision, prior.revision + 1);
  assert.deepEqual(recovered.nodes, []);
  await assert.rejects(store.add({ item: bookmark('b'), expectedRevision: prior.revision }), BookmarkStoreError);
  await store.add({ item: bookmark('b'), expectedRevision: recovered.revision });
  assert.deepEqual((await store.snapshot()).nodes.map(x => x.id), ['b']);
});

test('lost acknowledgement after a durable commit reloads committed bookmarks', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  const before = await store.snapshot();
  driver.failNextWrite('after');
  await assert.rejects(store.add({ item: bookmark('committed') }), /lost acknowledgement/);
  assert.deepEqual(importPortableBookmarks(driver.data).nodes.map(x => x.id), ['committed']);
  const restored = await store.snapshot();
  assert.ok(restored.revision > before.revision);
  assert.deepEqual(restored.nodes.map(x => x.id), ['committed']);
  await store.add({ item: bookmark('second'), expectedRevision: restored.revision });
  assert.deepEqual(importPortableBookmarks(driver.data).nodes.map(x => x.id), ['committed', 'second']);
});

test('corruption during uncertain-write reload fails closed instead of overwriting', async () => {
  const driver = memoryDriver();
  const store = new BookmarkStore(driver);
  driver.failNextWrite('after');
  await assert.rejects(store.add({ item: bookmark('a') }));
  driver.data = '{damaged';
  await assert.rejects(store.snapshot());
  await assert.rejects(store.add({ item: bookmark('b') }));
  assert.equal(driver.writes, 1);
});

test('concurrent mutations serialize while preserving both outcomes', async () => {
  const store = new BookmarkStore(memoryDriver());
  const [first, second] = await Promise.all([
    store.add({ item: bookmark('first') }), store.add({ item: bookmark('second') }),
  ]);
  assert.equal(first.revision, 1);
  assert.equal(second.revision, 2);
  assert.deepEqual((await store.snapshot()).nodes.map(x => x.id), ['first', 'second']);
});

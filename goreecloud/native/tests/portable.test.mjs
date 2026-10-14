// SPDX-License-Identifier: MPL-2.0
import test from 'node:test';
import assert from 'node:assert/strict';
import { BookmarkImportError, BOOKMARKS_FORMAT, exportPortableBookmarks, importPortableBookmarks } from '../bookmarks/portable.mjs';

const sample = [{ id: 'folder-1', type: 'folder', title: 'Work', children: [
  { id: 'b-1', type: 'bookmark', title: 'Site', url: 'https://example.org/a?x=1' },
  { id: 'b-2', type: 'bookmark', title: 'Local file', url: 'file:///home/user/index.html' },
] }];
const invalid = (nodes) => assert.throws(() => exportPortableBookmarks(nodes), BookmarkImportError);

test('round trips nested bookmarks completely offline', () => {
  const json = exportPortableBookmarks(sample);
  const result = importPortableBookmarks(json);
  assert.equal(result.format, BOOKMARKS_FORMAT);
  assert.deepEqual(result.nodes, sample);
  assert.equal(json, exportPortableBookmarks(result.nodes));
});
test('imports an empty local collection', () => assert.deepEqual(importPortableBookmarks(exportPortableBookmarks([])).nodes, []));
test('accepts local about URLs without network access', () => assert.equal(importPortableBookmarks(exportPortableBookmarks([
  { id: 'home', type: 'bookmark', title: 'Blank', url: 'about:blank' }
])).nodes[0].url, 'about:blank'));
test('rejects active and untrusted bookmark URL schemes', () => {
  for (const url of ['javascript:alert(1)', 'data:text/html,evil', 'ftp://example.org/', 'not-a-url']) {
    invalid([{ id: 'bad', type: 'bookmark', title: 'Bad', url }]);
  }
});
test('rejects repeated IDs and malformed folder contents', () => {
  invalid([{ id: 'same', type: 'bookmark', title: 'A', url: 'https://example.org' }, { id: 'same', type: 'folder', title: 'B', children: [] }]);
  invalid([{ id: 'root', type: 'folder', title: 'A' }]);
});
test('fails closed on unsupported schema and unknown properties', () => {
  assert.throws(() => importPortableBookmarks(JSON.stringify({ format: BOOKMARKS_FORMAT, version: 2, nodes: [] })), BookmarkImportError);
  invalid([{ id: 'bad', type: 'bookmark', title: 'A', url: 'https://example.org', extra: 'untrusted' }]);
});
test('rejects large inputs before parsing', () => assert.throws(() => importPortableBookmarks('x'.repeat(2_000_001)), BookmarkImportError));
test('rejects cyclic structures before export', () => {
  const parent = { id: 'f', type: 'folder', title: 'F', children: [] };
  parent.children.push(parent);
  invalid([parent]);
});
test('enforces bounded nesting without modifying inputs', () => {
  let inner = { id: 'end', type: 'bookmark', title: 'End', url: 'https://example.org' };
  for (let i=0; i<25; i++) inner = { id: `folder-${i}`, type: 'folder', title: 'F', children: [inner] };
  invalid([inner]);
});
test('rejects malformed JSON and null input', () => {
  assert.throws(() => importPortableBookmarks('{oops'), BookmarkImportError);
  assert.throws(() => importPortableBookmarks(null), BookmarkImportError);
});

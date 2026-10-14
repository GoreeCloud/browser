// SPDX-License-Identifier: MPL-2.0
// Pure, offline bookmarks interchange primitives for the Gecko integration track.
// Not a persistent store and not wired into Firefox UI yet.

export const BOOKMARKS_FORMAT = 'goreecloud.browser.bookmarks';
export const BOOKMARKS_VERSION = 1;

const MAX_JSON_BYTES = 2_000_000;
const MAX_NODES = 10_000;
const MAX_DEPTH = 24;
const MAX_TITLE = 512;
const MAX_URL = 4096;
const ID_PATTERN = /^[A-Za-z0-9][A-Za-z0-9._:-]{0,127}$/;
const ACCEPTED_SCHEMES = new Set(['http:', 'https:', 'file:', 'about:']);

export class BookmarkImportError extends Error {
  constructor(message) {
    super(message);
    this.name = 'BookmarkImportError';
  }
}

function fail(message) {
  throw new BookmarkImportError(message);
}

function record(value) {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function exactKeys(value, keys, location) {
  for (const key of Object.keys(value)) {
    if (!keys.includes(key)) fail(`Unknown field at ${location}: ${key}`);
  }
}

function validatedTree(document) {
  if (!record(document)) fail('Bookmark document must be an object');
  exactKeys(document, ['format', 'version', 'nodes'], 'root');
  if (document.format !== BOOKMARKS_FORMAT || document.version !== BOOKMARKS_VERSION) {
    fail('Unsupported bookmark interchange format or version');
  }
  if (!Array.isArray(document.nodes)) fail('Bookmark nodes must be an array');

  const ids = new Set();
  const references = new WeakSet();
  let count = 0;
  function node(item, depth) {
    if (depth > MAX_DEPTH) fail('Bookmark tree exceeds maximum depth');
    if (!record(item)) fail('Bookmark item must be an object');
    if (references.has(item)) fail('Repeated or cyclic bookmark tree object');
    references.add(item);
    if (++count > MAX_NODES) fail('Bookmark tree exceeds item limit');
    if (typeof item.id !== 'string' || !ID_PATTERN.test(item.id)) fail('Invalid bookmark item ID');
    if (ids.has(item.id)) fail('Duplicate bookmark item ID');
    ids.add(item.id);
    if (typeof item.title !== 'string' || item.title.length > MAX_TITLE) {
      fail('Bookmark title must be a string of at most 512 characters');
    }
    if (item.type === 'folder') {
      exactKeys(item, ['id', 'type', 'title', 'children'], item.id);
      if (!Array.isArray(item.children)) fail('Bookmark folder children must be an array');
      return { id: item.id, type: 'folder', title: item.title, children: item.children.map(child => node(child, depth + 1)) };
    }
    if (item.type === 'bookmark') {
      exactKeys(item, ['id', 'type', 'title', 'url'], item.id);
      if (typeof item.url !== 'string' || !item.url || item.url.length > MAX_URL) fail('Invalid bookmark URL');
      let url;
      try { url = new URL(item.url); } catch { fail('Invalid bookmark URL'); }
      if (!ACCEPTED_SCHEMES.has(url.protocol)) fail('Unsafe or unsupported bookmark URL scheme');
      if ((url.protocol === 'http:' || url.protocol === 'https:') && !url.hostname) fail('Bookmark URL needs a hostname');
      return { id: item.id, type: 'bookmark', title: item.title, url: item.url };
    }
    fail('Unknown bookmark item type');
  }
  return { format: BOOKMARKS_FORMAT, version: BOOKMARKS_VERSION, nodes: document.nodes.map(child => node(child, 1)) };
}

/** Validate the entire file before the caller changes persistent bookmark data. */
export function importPortableBookmarks(text) {
  if (typeof text !== 'string') fail('Bookmark import must be JSON text');
  if (new TextEncoder().encode(text).byteLength > MAX_JSON_BYTES) fail('Bookmark file is too large');
  let document;
  try { document = JSON.parse(text); } catch { fail('Malformed bookmark JSON'); }
  return validatedTree(document);
}

/** Produce a deterministic, versioned JSON document; does not perform I/O. */
export function exportPortableBookmarks(nodes) {
  const normalized = validatedTree({ format: BOOKMARKS_FORMAT, version: BOOKMARKS_VERSION, nodes });
  const text = JSON.stringify(normalized, null, 2) + '\n';
  if (new TextEncoder().encode(text).byteLength > MAX_JSON_BYTES) fail('Bookmark export is too large');
  return text;
}

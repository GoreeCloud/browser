// SPDX-License-Identifier: MPL-2.0
// Shared, platform-neutral bookmark location contract. No Gecko chrome,
// on-device database or GoreeCloud cloud transport is wired here yet.

import { exportPortableBookmarks, importPortableBookmarks } from './portable.mjs';

export const BOOKMARK_LOCATIONS = Object.freeze({
  LOCAL: 'local',
  GOREECLOUD: 'goreecloud',
});

const OPTIONS = Object.freeze([
  Object.freeze({ id: BOOKMARK_LOCATIONS.LOCAL, label: 'Local Bookmarks' }),
  Object.freeze({ id: BOOKMARK_LOCATIONS.GOREECLOUD, label: 'GoreeCloud Bookmarks' }),
]);

export class UnifiedBookmarkError extends Error {
  constructor(message, code = 'INVALID_REQUEST') {
    super(message);
    this.name = 'UnifiedBookmarkError';
    this.code = code;
  }
}

function requireLocation(location) {
  if (location !== BOOKMARK_LOCATIONS.LOCAL &&
      location !== BOOKMARK_LOCATIONS.GOREECLOUD) {
    throw new UnifiedBookmarkError('Choose Local Bookmarks or GoreeCloud Bookmarks explicitly', 'INVALID_LOCATION');
  }
}

function isStore(store) {
  return store && ['snapshot', 'add', 'rename', 'move', 'remove', 'exportJSON', 'importJSON']
    .every(method => typeof store[method] === 'function');
}

/**
 * Presents exactly two locations; data remains strictly partitioned.
 *
 * local is an offline BookmarkStore with its own profile-scoped driver.
 * cloud is a distinct *account-backed* store adapter. It MUST enforce
 * authentication, account identity, authorization, and remote acknowledgement
 * for every operation. Neither an ordinary local store nor a sync queue can
 * claim remote persistence. hasCloudSession is a UI gating hint, not a
 * substitute for enforcement within the cloud adapter.
 *
 * In particular: never silently redirect an unavailable cloud save to local.
 */
export class UnifiedBookmarks {
  #local;
  #cloud;
  #hasCloudSession;

  constructor({ local, cloud = null, hasCloudSession = () => false } = {}) {
    if (!isStore(local)) {
      throw new UnifiedBookmarkError('A local bookmark store is required');
    }
    if (cloud !== null && !isStore(cloud)) {
      throw new UnifiedBookmarkError('Cloud adapter does not implement the bookmark store contract');
    }
    if (typeof hasCloudSession !== 'function') {
      throw new UnifiedBookmarkError('Cloud session callback must be a function');
    }
    this.#local = local;
    this.#cloud = cloud;
    this.#hasCloudSession = hasCloudSession;
  }

  locations() {
    // A disabled cloud connection still appears as a *choice*, never as a
    // hidden third category or as a fallback to local storage.
    const cloudReady = this.#cloud !== null && this.#hasCloudSession() === true;
    return OPTIONS.map(option => ({
      ...option,
      available: option.id === BOOKMARK_LOCATIONS.LOCAL || cloudReady,
    }));
  }

  #store(location) {
    requireLocation(location);
    if (location === BOOKMARK_LOCATIONS.LOCAL) return this.#local;
    if (this.#cloud === null || this.#hasCloudSession() !== true) {
      throw new UnifiedBookmarkError('Sign in and connect GoreeCloud Bookmarks before using this location',
        'CLOUD_UNAVAILABLE');
    }
    return this.#cloud;
  }

  async snapshot(location) {
    const snapshot = await this.#store(location).snapshot();
    return { location, ...snapshot };
  }
  async save({ location, ...options } = {}) {
    const result = await this.#store(location).add(options);
    return { location, ...result };
  }
  async rename({ location, ...options } = {}) {
    const result = await this.#store(location).rename(options);
    return { location, ...result };
  }
  async move({ location, ...options } = {}) {
    const result = await this.#store(location).move(options);
    return { location, ...result };
  }
  async remove({ location, ...options } = {}) {
    const result = await this.#store(location).remove(options);
    return { location, ...result };
  }
  async exportJSON(location) {
    return this.#store(location).exportJSON();
  }
  async importJSON({ location, ...options } = {}) {
    const result = await this.#store(location).importJSON(options);
    return { location, ...result };
  }
}

const FIREFOX_ROOTS = Object.freeze(['toolbar', 'menu', 'other', 'mobile']);

/**
 * Non-destructive migration preview, never an importer or cloud operation.
 *
 * The source adapter extracts Firefox's legacy roots into arrays of the
 * portable bookmark nodes. Their *contents* are merged into Local Bookmarks,
 * not retained as top-level Firefox categories. Existing nested folders are
 * preserved. Original Firefox profile data remains untouched.
 *
 * Destination IDs are deterministically namespaced by root and preorder
 * index; a separate commit phase must confirm preview, back up the target and
 * transactionally merge rather than replace existing Local Bookmarks.
 */
export function previewFirefoxLegacyBookmarks(roots) {
  if (roots === null || typeof roots !== 'object' || Array.isArray(roots)) {
    throw new UnifiedBookmarkError('Firefox bookmark roots must be an object');
  }
  for (const key of Object.keys(roots)) {
    if (!FIREFOX_ROOTS.includes(key)) {
      throw new UnifiedBookmarkError(`Unknown Firefox bookmark root: ${key}`);
    }
  }
  const merged = [];
  const counts = {};
  for (const root of FIREFOX_ROOTS) {
    const source = Object.hasOwn(roots, root) ? roots[root] : [];
    // Reuse the portable codec for strict validation, deep copying and limits.
    const copied = importPortableBookmarks(exportPortableBookmarks(source)).nodes;
    let count = 0;
    function remap(item) {
      count++;
      const id = `ff-${root}-${count}`;
      return item.type === 'folder'
        ? { id, type: 'folder', title: item.title, children: item.children.map(remap) }
        : { id, type: 'bookmark', title: item.title, url: item.url };
    }
    merged.push(...copied.map(remap));
    counts[root] = count;
  }
  const json = exportPortableBookmarks(merged);
  return {
    location: BOOKMARK_LOCATIONS.LOCAL,
    counts,
    itemCount: Object.values(counts).reduce((a, b) => a + b, 0),
    json,
    // No writes here; explicit review, destination backup, merge and rollback
    // must be implemented by each host platform before migration is enabled.
  };
}

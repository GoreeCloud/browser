// SPDX-License-Identifier: MPL-2.0
// Offline domain model for a future privileged Gecko profile-store integration.
// Its driver owns atomic, durable persistence. No browser data is changed here.

import { exportPortableBookmarks, importPortableBookmarks } from './portable.mjs';

export class BookmarkStoreError extends Error {
  constructor(message) {
    super(message);
    this.name = 'BookmarkStoreError';
  }
}
function requireCondition(value, message) {
  if (!value) throw new BookmarkStoreError(message);
}
function normalized(nodes) {
  return importPortableBookmarks(exportPortableBookmarks(nodes)).nodes;
}
function locate(items, id) {
  for (let i = 0; i < items.length; i++) {
    const item = items[i];
    if (item.id === id) return { item, items, index: i };
    if (item.type === 'folder') {
      const found = locate(item.children, id);
      if (found) return found;
    }
  }
  return null;
}
function containsId(item, id) {
  return item.id === id ||
    (item.type === 'folder' && item.children.some(child => containsId(child, id)));
}
function destination(nodes, parentId) {
  if (parentId === null) return nodes;
  const parent = locate(nodes, parentId);
  requireCondition(parent?.item.type === 'folder', 'Destination folder does not exist');
  return parent.item.children;
}
function position(items, index) {
  if (index === undefined) return items.length;
  requireCondition(Number.isInteger(index) && index >= 0 && index <= items.length,
    'Invalid bookmark position');
  return index;
}
/**
 * Async, serial, single-browser-profile bookmark domain facade.
 *
 * driver.read() -> null (new profile) or UTF-8 string.
 * driver.write(text) -> resolves only after an atomic replacement is durable.
 * Missing/invalid files and read failures are distinct. This class never
 * recovers a corrupt file by overwriting it with an empty collection.
 * The driver must enforce exclusive profile ownership; external writers are
 * not supported until versioned cross-process coordination is implemented.
 * Revisions are in-memory tokens scoped to this store instance, not durable
 * cross-process version IDs.
 */
export class BookmarkStore {
  #driver;
  #queue = Promise.resolve();
  #loaded = false;
  #nodes = [];
  #revision = 0;

  constructor(driver) {
    requireCondition(driver && typeof driver.read === 'function' &&
      typeof driver.write === 'function', 'A read/write storage driver is required');
    this.#driver = driver;
  }
  #serial(action) {
    const operation = this.#queue.then(action);
    this.#queue = operation.then(() => undefined, () => undefined);
    return operation;
  }
  async #load() {
    if (this.#loaded) return;
    const text = await this.#driver.read();
    requireCondition(text === null || typeof text === 'string', 'Invalid storage-driver result');
    this.#nodes = text === null ? [] : importPortableBookmarks(text).nodes;
    this.#loaded = true;
  }
  #snapshot() {
    return { revision: this.#revision, nodes: normalized(this.#nodes) };
  }
  snapshot() {
    return this.#serial(async () => {
      await this.#load();
      return this.#snapshot();
    });
  }
  exportJSON() {
    return this.#serial(async () => {
      await this.#load();
      return exportPortableBookmarks(this.#nodes);
    });
  }
  #mutate(change, expectedRevision) {
    return this.#serial(async () => {
      await this.#load();
      if (expectedRevision !== undefined) {
        requireCondition(expectedRevision === this.#revision, 'Out-of-date bookmark revision');
      }
      const nextNodes = normalized(this.#nodes);
      change(nextNodes);
      const serialized = exportPortableBookmarks(nextNodes);
      // An atomic driver can commit successfully and still reject (for example,
      // if its acknowledgement is lost). Never assume a rejected write means
      // the on-disk data is unchanged: discard the cached snapshot and reload
      // before allowing any further read or mutation. Bump the instance-local
      // revision to invalidate optimistic concurrency tokens from before the
      // uncertain write. A corrupt/failed reload stays fail-closed.
      try {
        await this.#driver.write(serialized);
      } catch (error) {
        this.#loaded = false;
        this.#nodes = [];
        this.#revision++;
        throw error;
      }
      this.#nodes = nextNodes;
      this.#revision++;
      return this.#snapshot();
    });
  }
  add({ item, parentId = null, index, expectedRevision } = {}) {
    return this.#mutate(nodes => {
      requireCondition(item && typeof item.id === 'string', 'Bookmark item is required');
      requireCondition(!locate(nodes, item.id), 'Duplicate bookmark ID');
      const target = destination(nodes, parentId);
      target.splice(position(target, index), 0, item);
    }, expectedRevision);
  }
  rename({ id, title, expectedRevision } = {}) {
    return this.#mutate(nodes => {
      const entry = locate(nodes, id);
      requireCondition(entry, 'Bookmark item not found');
      entry.item.title = title;
    }, expectedRevision);
  }
  move({ id, parentId = null, index, expectedRevision } = {}) {
    return this.#mutate(nodes => {
      const entry = locate(nodes, id);
      requireCondition(entry, 'Bookmark item not found');
      requireCondition(!containsId(entry.item, parentId), 'Cannot move a folder into itself or a descendant');
      entry.items.splice(entry.index, 1);
      const target = destination(nodes, parentId);
      target.splice(position(target, index), 0, entry.item);
    }, expectedRevision);
  }
  remove({ id, allowRecursive = false, expectedRevision } = {}) {
    return this.#mutate(nodes => {
      const entry = locate(nodes, id);
      requireCondition(entry, 'Bookmark item not found');
      requireCondition(allowRecursive || entry.item.type !== 'folder' || entry.item.children.length === 0,
        'Deleting a nonempty folder requires allowRecursive');
      entry.items.splice(entry.index, 1);
    }, expectedRevision);
  }
  /** Destructive imports require explicit opt-in; never auto-run on startup. */
  importJSON({ text, allowReplace = false, expectedRevision } = {}) {
    return this.#mutate(nodes => {
      requireCondition(allowReplace, 'Replacing local bookmarks requires explicit consent');
      // Validate under the same serialized transaction as the replacement.
      const imported = importPortableBookmarks(text);
      nodes.splice(0, nodes.length, ...imported.nodes);
    }, expectedRevision);
  }
}

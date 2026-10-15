// SPDX-License-Identifier: MPL-2.0
// Opt-in Gecko privileged profile adapter. Not installed in browser chrome.
import { importPortableBookmarks } from './portable.mjs';

export const GECKO_PROFILE_BOOKMARK_FILE = 'goreecloud-browser-local-bookmarks.v1.json';
export class GeckoProfileBookmarkDriverError extends Error {
  constructor(message, code='PROFILE_STORE_UNAVAILABLE') {
    super(message);
    this.name = 'GeckoProfileBookmarkDriverError';
    this.code = code;
  }
}

/** The host must enforce exclusive profile ownership. Read/write preflight below
 * detects changes already visible to this instance; it is NOT an atomic
 * cross-process compare-and-swap and does not replace locking.
 * The privileged host must verify an exclusive profile lock, safe profile
 * path and private directory through assertProfileSecurity before any I/O. */
export function createGeckoProfileBookmarkDriver({
  ioUtils, pathUtils, developmentEnabled=false, assertProfileSecurity,
}={}) {
  if(developmentEnabled !== true) {
    throw new GeckoProfileBookmarkDriverError('Profile bookmarks are disabled', 'FEATURE_DISABLED');
  }
  if(!ioUtils || ['readUTF8','writeUTF8','stat','exists','setPermissions']
    .some(key=>typeof ioUtils[key] !== 'function') ||
    !pathUtils || typeof pathUtils.join !== 'function' ||
    typeof pathUtils.isAbsolute !== 'function' ||
    typeof pathUtils.profileDir !== 'string' ||
    !pathUtils.isAbsolute(pathUtils.profileDir)) {
    throw new GeckoProfileBookmarkDriverError('Privileged Gecko profile file APIs required',
      'INVALID_PRIVILEGED_ADAPTER');
  }
  if(typeof assertProfileSecurity !== 'function') {
    throw new GeckoProfileBookmarkDriverError(
      'A privileged profile security guard is required','PROFILE_SECURITY_GUARD_REQUIRED');
  }
  // The guard must only return true when the host still holds exclusive
  // profile ownership and has verified directory privacy/path safety.
  // Calling a guard does not create a lock or validate symlinks itself.
  async function requireProfileSecurity() {
    let authorized=false;
    try { authorized=(await assertProfileSecurity())===true; } catch {}
    if(!authorized) {
      throw new GeckoProfileBookmarkDriverError(
        'Privileged profile access is not verified','PROFILE_SECURITY_UNVERIFIED');
    }
  }
  const base=pathUtils.join(pathUtils.profileDir,GECKO_PROFILE_BOOKMARK_FILE);
  const backup=base+'.bak', temporary=base+'.tmp';
  // undefined: no successfully validated read; null: new profile; string:
  // last validated on-disk bookmark file. Never permit writes after a bad read.
  let observedContent;
  // Serialize all operations on this driver instance. BookmarkStore already
  // serializes its own actions, but direct privileged callers must not
  // interleave a read or commit with an in-flight write.
  let queue=Promise.resolve();
  function serial(action) {
    const next=queue.then(action);
    queue=next.then(()=>undefined,()=>undefined);
    return next;
  }

  async function checkRecoveryFiles() {
    if(await ioUtils.exists(backup) || await ioUtils.exists(temporary)) {
      throw new GeckoProfileBookmarkDriverError(
        'Review orphaned bookmark recovery files before creating new data',
        'RECOVERY_REQUIRED');
    }
  }

  return Object.freeze({
    read() { return serial(async()=>{
      observedContent=undefined;
      await requireProfileSecurity();
      let stat;
      try { stat=await ioUtils.stat(base); }
      catch(error) {
        if(error?.name !== 'NotFoundError') throw error;
        await checkRecoveryFiles();
        await requireProfileSecurity();
        observedContent=null;
        return null;
      }
      if(stat?.type !== 'regular' || !Number.isSafeInteger(stat.size) ||
         stat.size < 0 || stat.size > 2_000_000) {
        throw new GeckoProfileBookmarkDriverError('Invalid or oversized bookmark file',
          'INVALID_PROFILE_FILE');
      }
      let text;
      try { text=await ioUtils.readUTF8(base); }
      catch(error) {
        if(error?.name === 'NotFoundError') {
          throw new GeckoProfileBookmarkDriverError(
            'Bookmark file disappeared after stat', 'PROFILE_STORE_CHANGED');
        }
        throw error;
      }
      if(typeof text !== 'string') {
        throw new GeckoProfileBookmarkDriverError('Invalid bookmark read','INVALID_PROFILE_FILE');
      }
      // Validation belongs at the storage boundary even if a caller bypasses
      // BookmarkStore. A corrupt read must never authorize a replacement.
      importPortableBookmarks(text);
      await requireProfileSecurity();
      observedContent=text;
      return text;
    }); },
    write(text) { return serial(async()=>{
      // Any security denial or write uncertainty invalidates prior approval.
      try {
        await requireProfileSecurity();
        if(observedContent===undefined) {
          throw new GeckoProfileBookmarkDriverError('Read before writing','READ_REQUIRED');
        }
        importPortableBookmarks(text);
        // The privileged host must maintain exclusive ownership for the
        // entire transaction. Preflight alone cannot prevent later races.
        const exists=await ioUtils.exists(base);
        if(exists !== (observedContent!==null)) {
          throw new GeckoProfileBookmarkDriverError(
            'Bookmark file appeared or disappeared after read','PROFILE_STORE_CHANGED');
        }
        // A leftover temporary file may contain the only copy of an
        // interrupted commit. Never reuse the fixed tmpPath until recovery
        // has been explicitly reviewed, even if the primary file exists.
        if(await ioUtils.exists(temporary)) {
          throw new GeckoProfileBookmarkDriverError(
            'Unresolved temporary bookmark file','RECOVERY_REQUIRED');
        }
        if(exists) {
          const current=await ioUtils.readUTF8(base);
          if(current!==observedContent) {
            throw new GeckoProfileBookmarkDriverError(
              'Bookmark file changed after read','PROFILE_STORE_CHANGED');
          }
        } else {
          await checkRecoveryFiles();
        }
        // The security guard might have been revoked while asynchronous
        // profile reads/preflight were pending. Recheck just before the write.
        await requireProfileSecurity();
        const options={tmpPath:temporary,flush:true,mode:'overwrite'};
        if(exists) options.backupFile=backup;
        await ioUtils.writeUTF8(base,text,options);
        // Any post-write permission failure leaves uncertain commit status.
        await ioUtils.setPermissions(base,0o600,false);
        if(exists) await ioUtils.setPermissions(backup,0o600,false);
        observedContent=text;
      } catch(error) {
        // A rejected write may have durably committed. Explicit revalidation
        // is necessary before another direct driver write can proceed.
        observedContent=undefined;
        throw error;
      }
    }); },
  });
}

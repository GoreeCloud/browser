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

/** Host must enforce a single profile owner; this is not a multi-process DB. */
export function createGeckoProfileBookmarkDriver({
  ioUtils, pathUtils, developmentEnabled=false,
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
  const base=pathUtils.join(pathUtils.profileDir,GECKO_PROFILE_BOOKMARK_FILE);
  const backup=base+'.bak', temporary=base+'.tmp';
  let readSucceeded=false;

  async function checkRecoveryFiles() {
    if(await ioUtils.exists(backup) || await ioUtils.exists(temporary)) {
      throw new GeckoProfileBookmarkDriverError(
        'Review orphaned bookmark recovery files before creating new data',
        'RECOVERY_REQUIRED');
    }
  }

  return Object.freeze({
    async read() {
      let stat;
      try { stat=await ioUtils.stat(base); }
      catch(error) {
        if(error?.name !== 'NotFoundError') throw error;
        await checkRecoveryFiles();
        readSucceeded=true;
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
      readSucceeded=true;
      // BookmarkStore validates the loaded JSON and refuses to overwrite corruption.
      return text;
    },
    async write(text) {
      if(!readSucceeded) {
        throw new GeckoProfileBookmarkDriverError('Read before writing','READ_REQUIRED');
      }
      importPortableBookmarks(text);
      const exists=await ioUtils.exists(base);
      if(!exists) await checkRecoveryFiles();
      const options={tmpPath:temporary,flush:true,mode:'overwrite'};
      if(exists) options.backupFile=backup;
      await ioUtils.writeUTF8(base,text,options);
      // Any post-write permission failure is ambiguous to BookmarkStore,
      // which will discard its cache and reload the committed file.
      await ioUtils.setPermissions(base,0o600,false);
      if(exists) await ioUtils.setPermissions(backup,0o600,false);
    },
  });
}

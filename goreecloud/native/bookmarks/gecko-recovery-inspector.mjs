// SPDX-License-Identifier: MPL-2.0
// Development-only read-only profile recovery preview. No automatic restoration.
import {importPortableBookmarks} from './portable.mjs';
import {GECKO_PROFILE_BOOKMARK_FILE} from './gecko-profile-driver.mjs';

export class GeckoBookmarkRecoveryInspectionError extends Error {
  constructor(code) {
    super('Cannot safely inspect bookmark recovery artifacts');
    this.name='GeckoBookmarkRecoveryInspectionError';
    this.code=code;
  }
}
const MAX_BYTES=2_000_000, MAX_FILES=32, MAX_ENTRIES=25_000;
const safeSuffix=/^[A-Za-z0-9._-]{1,96}$/;
const collisionNumber=/^[1-9][0-9]{0,3}$/;

/**
 * Privileged, development-gated, read-only recovery status. The host must
 * independently prove profile ownership, symlink-safe paths and an exclusive
 * profile lock for the entire inspection. Never expose text, paths or URLs.
 */
export function createGeckoBookmarkRecoveryInspector({
  ioUtils,pathUtils,assertProfileSecurity,developmentEnabled=false,
  validatePortable=importPortableBookmarks,
}={}) {
  if(developmentEnabled!==true)throw new GeckoBookmarkRecoveryInspectionError('FEATURE_DISABLED');
  if(!ioUtils || ['stat','readUTF8','getChildren'].some(k=>typeof ioUtils[k]!=='function') ||
     !pathUtils || typeof pathUtils.profileDir!=='string' ||
     typeof pathUtils.join!=='function' || typeof pathUtils.isAbsolute!=='function' ||
     !pathUtils.isAbsolute(pathUtils.profileDir) ||
     typeof assertProfileSecurity!=='function' || typeof validatePortable!=='function') {
    throw new GeckoBookmarkRecoveryInspectionError('INVALID_PRIVILEGED_ADAPTER');
  }
  const base=pathUtils.join(pathUtils.profileDir,GECKO_PROFILE_BOOKMARK_FILE);
  const backup=base+'.bak',legacy=base+'.tmp',tempBase=base+'.tmp-';
  const collisionBase=base+'-';

  async function secure() {
    let ok=false;
    try {ok=(await assertProfileSecurity())===true;}catch {}
    if(!ok)throw new GeckoBookmarkRecoveryInspectionError('PROFILE_SECURITY_UNVERIFIED');
  }
  async function inspectFile(path) {
    await secure();
    let st;
    try {st=await ioUtils.stat(path);}
    catch(e) {if(e?.name==='NotFoundError')return 'absent';return 'unreadable';}
    if(st?.type!=='regular')return 'not-regular';
    if(!Number.isSafeInteger(st.size)||st.size<0||st.size>MAX_BYTES)return 'oversized';
    try {
      await secure();
      const value=await ioUtils.readUTF8(path);
      if(typeof value!=='string' || new TextEncoder().encode(value).byteLength>MAX_BYTES)
        return 'corrupt';
      await validatePortable(value);
      await secure();
      return 'valid';
    }catch(e) {
      if(e?.code==='PROFILE_SECURITY_UNVERIFIED')throw e;
      return 'corrupt-or-unreadable';
    }
  }
  function classifiedTemp(path) {
    if(path.startsWith(tempBase)) {
      const suffix=path.slice(tempBase.length);
      if(suffix!==''&&!safeSuffix.test(suffix))
        throw new GeckoBookmarkRecoveryInspectionError('UNVERIFIABLE_DIRECTORY');
      return true;
    }
    if(path.startsWith(collisionBase)&&path.endsWith('.tmp-')) {
      const n=path.slice(collisionBase.length,-'.tmp-'.length);
      if(!collisionNumber.test(n)||Number(n)>=10000)
        throw new GeckoBookmarkRecoveryInspectionError('UNVERIFIABLE_DIRECTORY');
      return true;
    }
    return false;
  }
  return Object.freeze({
    async inspect() {
      await secure();
      let entries;
      try {entries=await ioUtils.getChildren(pathUtils.profileDir);}
      catch {throw new GeckoBookmarkRecoveryInspectionError('UNVERIFIABLE_DIRECTORY');}
      if(!Array.isArray(entries)||entries.length>MAX_ENTRIES||
         entries.some(p=>typeof p!=='string'||!pathUtils.isAbsolute(p)))
        throw new GeckoBookmarkRecoveryInspectionError('UNVERIFIABLE_DIRECTORY');
      const seen=new Set(),temps=[];
      for(const path of entries) {
        if(!classifiedTemp(path))continue;
        if(seen.has(path))throw new GeckoBookmarkRecoveryInspectionError('UNVERIFIABLE_DIRECTORY');
        seen.add(path);temps.push(path);
      }
      if(temps.length>MAX_FILES)throw new GeckoBookmarkRecoveryInspectionError('RECOVERY_SCAN_LIMIT');
      temps.sort();
      const primary=await inspectFile(base);
      const old=await inspectFile(backup);
      const legacyStatus=await inspectFile(legacy);
      const temporaryStatuses=[];
      for(const p of temps)temporaryStatuses.push(await inspectFile(p));
      await secure();
      const requiresManualReview=!['valid','absent'].includes(primary) ||
        !['valid','absent'].includes(old) ||
        (old!=='absent'&&primary!=='valid') ||
        legacyStatus!=='absent'||temps.length>0;
      return Object.freeze({
        status:requiresManualReview?'recovery-review-required':
          primary==='valid'?'healthy':'empty',
        primary,backup:old,legacyTemporary:legacyStatus,
        privateTemporaries:Object.freeze(temporaryStatuses),
        requiresManualReview,readOnly:true,
      });
    }
  });
}

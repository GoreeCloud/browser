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

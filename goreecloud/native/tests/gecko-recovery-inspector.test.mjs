// SPDX-License-Identifier: MPL-2.0
// Pure Node contract tests. These are not privileged Gecko runtime tests.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createGeckoBookmarkRecoveryInspector as inspector} from '../bookmarks/gecko-recovery-inspector.mjs';
import {GECKO_PROFILE_BOOKMARK_FILE} from '../bookmarks/gecko-profile-driver.mjs';
import {exportPortableBookmarks} from '../bookmarks/portable.mjs';

const profile='/isolated/recovery';
const base=profile+'/'+GECKO_PROFILE_BOOKMARK_FILE;
const valid=exportPortableBookmarks([{id:'a',type:'bookmark',title:'A',url:'https://example.org/a'}]);
function fixture() {
  const files=new Map();
  let authorized=true,reads=0;
  const ioUtils={
    async getChildren(){return [...files.keys()];},
    async stat(p){
      if(!files.has(p))throw Object.assign(Error('not found'),{name:'NotFoundError'});
      const text=files.get(p);
      return {type:'regular',size:Buffer.byteLength(text)};
    },
    async readUTF8(p){reads++;return files.get(p);}
  };
  const options={ioUtils,pathUtils:{profileDir:profile,join:(...xs)=>xs.join('/'),
    isAbsolute:p=>p.startsWith('/')},developmentEnabled:true,
    assertProfileSecurity:async()=>authorized};
  return {files,ioUtils,options,create:()=>inspector(options),
    revoke:()=>{authorized=false;},reads:()=>reads};
}

test('recovery preview is opt-in',()=>{
  const f=fixture();
  assert.throws(()=>inspector({...f.options,developmentEnabled:false}),{code:'FEATURE_DISABLED'});
});
test('empty and healthy profiles return status without raw data',async()=>{
  const f=fixture();
  assert.equal((await f.create().inspect()).status,'empty');
  f.files.set(base,valid);
  const result=await f.create().inspect();
  assert.equal(result.status,'healthy');
  assert.equal(result.primary,'valid');
  assert.equal(result.requiresManualReview,false);
  assert.ok(Object.isFrozen(result));
  assert.ok(!JSON.stringify(result).includes('example.org'));
  assert.ok(!JSON.stringify(result).includes(profile));
});
test('numeric Gecko collision and exact prefix are retained for manual review',async()=>{
  const f=fixture(),collision=base.replace(/\.json$/,'.json-1.tmp-');
  f.files.set(base,valid);
  f.files.set(collision,'{bad');
  f.files.set(base+'.tmp-',valid);
  const before=new Map(f.files);
  const result=await f.create().inspect();
  assert.equal(result.status,'recovery-review-required');
  assert.deepEqual(result.privateTemporaries,['corrupt-or-unreadable','valid']);
  assert.deepEqual(f.files,before);
  assert.equal(result.readOnly,true);
});
test('legacy temp and absent primary require recovery review',async()=>{
  const f=fixture();f.files.set(base+'.tmp',valid);
  const r=await f.create().inspect();
  assert.equal(r.primary,'absent');
  assert.equal(r.legacyTemporary,'valid');
  assert.equal(r.requiresManualReview,true);
});
test('rejected async validation cannot certify a recovery file',async()=>{
  const f=fixture();f.files.set(base,valid);
  f.options.validatePortable=async()=>{throw Error('invalid');};
  assert.equal((await f.create().inspect()).primary,'corrupt-or-unreadable');
});
test('revoked profile security refuses inspection',async()=>{
  const f=fixture();f.files.set(base,valid);f.revoke();
  await assert.rejects(f.create().inspect(),{code:'PROFILE_SECURITY_UNVERIFIED'});
  assert.equal(f.reads(),0);
});
test('directory listing failure hides sensitive profile paths',async()=>{
  const f=fixture();
  f.ioUtils.getChildren=async()=>{throw Error('/sensitive/owner/location');};
  await assert.rejects(f.create().inspect(),e=>
    e.code==='UNVERIFIABLE_DIRECTORY'&&!e.message.includes('/sensitive'));
});
test('oversized directory and excess recovery files fail closed',async()=>{
  const f=fixture();
  f.ioUtils.getChildren=async()=>Array.from({length:25001},(_,i)=>profile+'/unrelated-'+i);
  await assert.rejects(f.create().inspect(),{code:'UNVERIFIABLE_DIRECTORY'});
  f.ioUtils.getChildren=async()=>Array.from({length:33},(_,i)=>base+'.tmp-'+i);
  await assert.rejects(f.create().inspect(),{code:'RECOVERY_SCAN_LIMIT'});
});
test('oversized contents are never passed to validator',async()=>{
  const f=fixture();f.files.set(base,'x'.repeat(2_000_001));
  const r=await f.create().inspect();
  assert.equal(r.primary,'oversized');
  assert.equal(f.reads(),0);
});

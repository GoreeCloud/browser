// SPDX-License-Identifier: MPL-2.0
import test from 'node:test';
import assert from 'node:assert/strict';
import { FEATURE_IDS, NATIVE_FEATURES, NativeCapabilityRegistry } from '../capabilities/catalog.mjs';
import { getOmniboxGlyphs, requestOmniboxIntent } from '../capabilities/omnibox.mjs';

const ready = {status:async()=>({state:'ready'}),open:async()=>true};

test('nine stable native feature identifiers',()=>{
  assert.equal(NATIVE_FEATURES.length,9);
  assert.equal(new Set(NATIVE_FEATURES.map(x=>x.id)).size,9);
});
test('four accessible omnibox glyphs in stable order',async()=>{
  const icons=await getOmniboxGlyphs(new NativeCapabilityRegistry());
  assert.deepEqual(icons.map(x=>x.id),['privacy-shield','wardveil','webspaces','reader-mode']);
  assert.ok(icons.every(x=>x.ariaLabel && !x.enabled));
});
test('security control is disabled until its own adapter is ready',async()=>{
  const r=new NativeCapabilityRegistry();
  await assert.rejects(requestOmniboxIntent(r,'privacy-shield'),{code:'GLYPH_UNAVAILABLE'});
  r.register(FEATURE_IDS.PRIVACY_SHIELD,ready);
  assert.deepEqual(await requestOmniboxIntent(r,'privacy-shield'),
    {type:'open-privacy-shield',source:'omnibox'});
});
test('reader works offline and can be exited',async()=>{
  const r=new NativeCapabilityRegistry();
  assert.equal((await getOmniboxGlyphs(r,{readerActive:true}))[3].selected,true);
  assert.deepEqual(await requestOmniboxIntent(r,'reader-mode',{readerAvailable:true}),
    {type:'toggle-reader-mode',source:'omnibox'});
});
test('unknown glyphs and unsupported reader pages are rejected',async()=>{
  const r=new NativeCapabilityRegistry();
  await assert.rejects(requestOmniboxIntent(r,'danger'),{code:'UNKNOWN_GLYPH'});
  await assert.rejects(requestOmniboxIntent(r,'reader-mode'),{code:'GLYPH_UNAVAILABLE'});
});
test('private windows prohibit account and Webspace controls',async()=>{
  const r=new NativeCapabilityRegistry();r.register(FEATURE_IDS.WEBSPACES,ready);
  r.register(FEATURE_IDS.AI,ready);
  assert.equal((await r.status(FEATURE_IDS.AI,{privateWindow:true})).state,'unavailable');
  assert.equal((await getOmniboxGlyphs(r,{privateWindow:true}))[2].enabled,false);
});
test('adapters do not receive secrets or page content',async()=>{
  let seen;const r=new NativeCapabilityRegistry();
  r.register(FEATURE_IDS.WARDVEIL,{status:async c=>{seen=c;return {state:'ready'}},open:async()=>{}});
  await r.status(FEATURE_IDS.WARDVEIL,{url:'https://secret.example',password:'secret'});
  assert.deepEqual(seen,{privateWindow:false});
});
test('malfunctioning providers do not expose errors',async()=>{
  const r=new NativeCapabilityRegistry();
  r.register(FEATURE_IDS.WARDVEIL,{status:async()=>{throw Error('private token')},open:async()=>{}});
  const state=await r.status(FEATURE_IDS.WARDVEIL);
  assert.equal(state.state,'unavailable');
  assert.equal(JSON.stringify(state).includes('token'),false);
});
test('statuses cannot claim arbitrary protected state',async()=>{
  const r=new NativeCapabilityRegistry();
  r.register(FEATURE_IDS.WARDVEIL,{status:async()=>({state:'protected'}),open:async()=>{}});
  assert.equal((await getOmniboxGlyphs(r))[1].enabled,false);
});
test('readiness is checked again for each action',async()=>{
  let active=true;const r=new NativeCapabilityRegistry();
  r.register(FEATURE_IDS.WEBSPACES,{status:async()=>({state:active?'ready':'unavailable'}),open:async()=>true});
  assert.equal((await getOmniboxGlyphs(r))[2].enabled,true);
  active=false;await assert.rejects(requestOmniboxIntent(r,'webspaces'),{code:'GLYPH_UNAVAILABLE'});
});

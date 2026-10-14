// SPDX-License-Identifier: MPL-2.0
// Source/VM tests; Gecko browser-runtime acceptance is tracked separately.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {join} from 'node:path';
import vm from 'node:vm';

const root=fileURLToPath(new URL('../../../',import.meta.url));
const read=path=>readFileSync(join(root,path),'utf8');
const code=read('browser/base/content/browser-goreecloud-omnibox.js');

class MockNode {
  constructor(){this.hidden=false;this.attrs=new Map();this.children=[];this.classList={add(){}};}
  setAttribute(key,value){this.attrs.set(key,value);}
  appendChild(child){this.children.push(child);}
  addEventListener(){}
  remove(){this.removed=true;}
}
async function browser({enabled=false,privateWindow=false}={}){
  const parent={buttons:[],insertBefore(node){this.buttons.push(node)}};
  const reader=new MockNode();
  let pref=enabled;
  const observers=[];
  const context={
    document:{getElementById:id=>id==='page-action-buttons'?parent:id==='reader-mode-button'?reader:null,
      createXULElement:()=>new MockNode()},
    Services:{prefs:{getBoolPref:()=>pref,addObserver:(_name,o)=>observers.push(o),
      removeObserver:(_name,o)=>{const i=observers.indexOf(o);if(i>=0)observers.splice(i,1)}}},
    PrivateBrowsingUtils:{isWindowPrivate:()=>privateWindow},
    gBrowser:{tabContainer:{addEventListener(){},removeEventListener(){}},
      addTabsProgressListener(){},removeTabsProgressListener(){}},
    window:{addEventListener(){}},
    delayedStartupPromise:new Promise(()=>{}),
  };
  vm.runInNewContext(code,context,{filename:'browser-goreecloud-omnibox.js'});
  const controller=context.GoreeCloudOmnibox;
  controller.init();await controller.refresh();
  return {controller,parent,observers,setPref(value){pref=value;
    for(const observer of observers)observer.observe(null,'nsPref:changed',controller.pref)},
    node:id=>controller.nodes.get(id)};
}
test('jar packages three glyphs and chrome controller',()=>{
  const jar=read('browser/base/jar.mn');
  for(const item of ['browser-goreecloud-omnibox.js',
    'goreecloud/icons/privacy-shield.svg','goreecloud/icons/wardveil.svg',
    'goreecloud/icons/webspaces.svg']){
    assert.ok(jar.includes('content/browser/'+item),item);
  }
  const main=read('browser/base/content/browser-main.js');
  assert.ok(main.indexOf('browser-pageActions.js')<main.indexOf('browser-goreecloud-omnibox.js'));
});
test('Firefox built-in reader button is preserved',()=>{
  const xml=read('browser/base/content/navigator-toolbox.inc.xhtml');
  assert.match(xml,/id="reader-mode-button"/);
  assert.match(xml,/id="page-action-buttons"/);
  assert.match(code,/reader-mode-button/);
});
test('three accessible buttons are created, hidden by default',async()=>{
  const b=await browser();
  assert.equal(b.parent.buttons.length,3);
  assert.ok(b.parent.buttons.every(x=>x.hidden));
  for(const item of b.controller.definitions) {
    const n=b.node(item.id);
    assert.equal(n.attrs.get('role'),'button');
    assert.equal(n.attrs.get('aria-label'),item.label);
  }
  b.controller.destroy();
});
test('only ready adapter displays and opens after opt in',async()=>{
  const b=await browser({enabled:true});let opened=0;
  b.controller.register('privacy-shield',{status:async()=>({state:'ready'}),
    open:async()=>{opened++}});
  await b.controller.refresh();
  assert.equal(b.node('privacy-shield').hidden,false);
  assert.equal(b.node('wardveil').hidden,true);
  await b.controller.activate('privacy-shield');
  assert.equal(opened,1);
  b.controller.destroy();
});
test('Webspaces is unavailable inside a private window',async()=>{
  const b=await browser({enabled:true,privateWindow:true});let opened=0;
  b.controller.register('webspaces',{status:async()=>({state:'ready'}),open:async()=>{opened++}});
  await b.controller.refresh();
  assert.equal(b.node('webspaces').hidden,true);
  await b.controller.activate('webspaces');
  assert.equal(opened,0);
  b.controller.destroy();
});
test('disabling experimental pref prevents new actions',async()=>{
  const b=await browser({enabled:true});let opened=0;
  b.controller.register('wardveil',{status:async()=>({state:'ready'}),open:async()=>{opened++}});
  await b.controller.refresh();
  assert.equal(b.node('wardveil').hidden,false);
  b.setPref(false);await b.controller.refresh();
  await b.controller.activate('wardveil');
  assert.equal(opened,0);
  b.controller.destroy();
});
test('navigation invalidates pending service status',async()=>{
  const b=await browser({enabled:true});let mode='ready',resolveOld;
  b.controller.register('wardveil',{status:async()=>mode==='old'
    ? new Promise(resolve=>{resolveOld=resolve})
    : {state:mode},open:async()=>{}});
  await b.controller.refresh();
  assert.equal(b.node('wardveil').hidden,false);
  mode='old';const pending=b.controller.refresh();
  assert.equal(b.node('wardveil').hidden,true);
  mode='unavailable';await b.controller.refresh();
  resolveOld({state:'ready'});await pending;
  assert.equal(b.node('wardveil').hidden,true);
  b.controller.destroy();
});
test('teardown removes generated buttons and service references',async()=>{
  const b=await browser();
  b.controller.destroy();
  assert.equal(b.controller.nodes.size,0);
  assert.equal(b.observers.length,0);
  assert.ok(b.parent.buttons.every(x=>x.removed));
});

// SPDX-License-Identifier: MPL-2.0
// Source checks only: these do not execute a Gecko chrome-mochitest.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {join} from 'node:path';
import vm from 'node:vm';

const root=fileURLToPath(new URL('../../../',import.meta.url));
const source=path=>readFileSync(join(root,path),'utf8');
const filename='test_goreecloud_bookmark_private_temp.html';
const dirname='xpcom/ioutils/tests/';

test('Gecko native bookmark privacy test is registered in its chrome suite',()=>{
  const manifest=source(dirname+'chrome.toml');
  assert.ok(manifest.includes('["'+filename+'"]'));
  const build=source('xpcom/ioutils/moz.build');
  assert.ok(build.includes('MOCHITEST_CHROME_MANIFESTS'));
  assert.ok(build.includes('tests/chrome.toml'));
});

test('native bookmark privacy test script parses without executing privileged IO',()=>{
  const html=source(dirname+filename);
  const script=html.match(/<script>\s*([\s\S]*?)<\/script>/);
  assert.ok(script,'Expected an inline privileged test script');
  assert.doesNotThrow(()=>new vm.Script(script[1],{filename}));
});

test('native privacy test does not access user profiles or log bookmarks',()=>{
  const html=source(dirname+filename);
  assert.ok(html.includes('PathUtils.tempDir'));
  assert.ok(html.includes('IOUtils.createUniqueFile'));
  assert.ok(html.includes('IOUtils.writeUTF8'));
  assert.ok(html.includes('registerCleanupFunction'));
  assert.ok(html.includes('Services.appinfo.OS==="WINNT"'));
  assert.ok(html.includes('return;'),'Windows must not run the bookmark write');
  assert.ok(!html.includes('PathUtils.profileDir'));
  assert.ok(!html.includes('console.log'));
});

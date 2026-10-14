// SPDX-License-Identifier: MPL-2.0
// UI-facing omnibox model. Returns inert intents, not executable privileged
// commands or remote requests. Gecko is responsible for actual button wiring.
import { FEATURE_IDS } from './catalog.mjs';

const GLYPHS = Object.freeze([
  Object.freeze({id:'privacy-shield',label:'Privacy Shield',icon:'shield',feature:FEATURE_IDS.PRIVACY_SHIELD,intent:'open-privacy-shield'}),
  Object.freeze({id:'wardveil',label:'Wardveil Security',icon:'shield-check',feature:FEATURE_IDS.WARDVEIL,intent:'open-wardveil'}),
  Object.freeze({id:'webspaces',label:'Webspaces',icon:'layers',feature:FEATURE_IDS.WEBSPACES,intent:'open-webspaces'}),
  Object.freeze({id:'reader-mode',label:'Reader Mode',icon:'book-open',feature:null,intent:'toggle-reader-mode'}),
]);
const byId=new Map(GLYPHS.map(g => [g.id,g]));

export class OmniboxActionError extends Error {
  constructor(message, code='GLYPH_UNAVAILABLE') {
    super(message);
    this.name='OmniboxActionError';
    this.code=code;
  }
}

const basicContext = ctx => Object.freeze({
  privateWindow: ctx?.privateWindow === true,
  readerAvailable: ctx?.readerAvailable === true,
  readerActive: ctx?.readerActive === true,
});

/** Strictly non-identifying glyph descriptors for keyboard/screen-reader UI. */
export async function getOmniboxGlyphs(registry, context={}) {
  if(!registry || typeof registry.status !== 'function') throw new TypeError('Capability registry required');
  const ctx=basicContext(context);
  const response=[];
  for(const glyph of GLYPHS) {
    const status=glyph.feature ? await registry.status(glyph.feature, ctx) : null;
    const enabled=glyph.feature ? status.state === 'ready' : ctx.readerAvailable || ctx.readerActive;
    const reason=enabled ? '' : glyph.feature ? status.reason : 'Reader Mode is unavailable on this page';
    response.push(Object.freeze({
      id:glyph.id, label:glyph.label, ariaLabel:glyph.label,
      icon:glyph.icon, enabled, reason,
      // A stable value for accessible button dispatch, not an action itself.
      action:glyph.intent,
      selected:glyph.id==='reader-mode' && ctx.readerActive,
    }));
  }
  return Object.freeze(response);
}

/**
 * Rechecks readiness at activation time. No navigation, network, AI request,
 * privacy setting or Webspace container change happens in this module.
 * Caller must route the resulting intent through privileged Gecko chrome.
 */
export async function requestOmniboxIntent(registry, glyphId, context={}) {
  if(!byId.has(glyphId)) throw new OmniboxActionError('Unknown omnibox glyph','UNKNOWN_GLYPH');
  const glyph=(await getOmniboxGlyphs(registry,context)).find(g=>g.id===glyphId);
  if(!glyph.enabled) throw new OmniboxActionError('Omnibox capability is unavailable');
  // No URL/title/text/context is embedded in intents.
  return Object.freeze({type:glyph.action,source:'omnibox'});
}

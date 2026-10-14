// SPDX-License-Identifier: MPL-2.0
// Native feature registry contract; no network, extension install or UI side effects.

export const FEATURE_IDS = Object.freeze({
  PRIVACY_SHIELD: 'privacy-shield',
  WARDVEIL: 'wardveil',
  TAB_MANAGER: 'advanced-tab-manager',
  DOWNLOAD_MANAGER: 'advanced-download-manager',
  PRIVATE_WINDOW_MANAGER: 'advanced-private-window-manager',
  PROFILE_MANAGER: 'advanced-profile-manager',
  WEBSPACES: 'webspaces',
  SEARCH: 'search',
  AI: 'ai',
});

const catalog = [
  { id: FEATURE_IDS.PRIVACY_SHIELD, label: 'Privacy Shield', owner: 'Privacy Shield', local: true, privateWindow: true },
  { id: FEATURE_IDS.WARDVEIL, label: 'Wardveil Security', owner: 'Wardveil Security', local: true, privateWindow: true },
  { id: FEATURE_IDS.TAB_MANAGER, label: 'GoreeCloud Advanced Tab Manager', owner: 'Browser', local: true, privateWindow: true },
  { id: FEATURE_IDS.DOWNLOAD_MANAGER, label: 'GoreeCloud Advanced Download Manager', owner: 'Advanced Download Manager', local: true, privateWindow: true },
  { id: FEATURE_IDS.PRIVATE_WINDOW_MANAGER, label: 'GoreeCloud Advanced Private Window Manager', owner: 'Browser', local: true, privateWindow: true },
  { id: FEATURE_IDS.PROFILE_MANAGER, label: 'GoreeCloud Advanced Profile Manager', owner: 'Browser', local: true, privateWindow: false },
  { id: FEATURE_IDS.WEBSPACES, label: 'GoreeCloud Webspaces', owner: 'Webspaces', local: true, privateWindow: false },
  { id: FEATURE_IDS.SEARCH, label: 'GoreeCloud Search', owner: 'Search', local: false, privateWindow: false },
  { id: FEATURE_IDS.AI, label: 'GoreeCloud AI', owner: 'AI', local: false, privateWindow: false },
];
export const NATIVE_FEATURES = Object.freeze(catalog.map(feature => Object.freeze(feature)));
const featureById = new Map(NATIVE_FEATURES.map(feature => [feature.id, feature]));
export const CAPABILITY_STATES = Object.freeze(['unavailable', 'ready', 'attention']);

export class NativeCapabilityError extends Error {
  constructor(message, code='CAPABILITY_UNAVAILABLE') {
    super(message);
    this.name = 'NativeCapabilityError';
    this.code = code;
  }
}

/**
 * Provider contract (privileged parent process only):
 * - register only after verifying the actual Gecko native service adapter;
 * - provider.status(context) returns {state:'ready'|'attention'|'unavailable', reason?};
 * - provider.open(context) must independently authorize the user action;
 * - callbacks do not receive URL, browsing history, credentials or page text.
 */
export class NativeCapabilityRegistry {
  #providers = new Map();

  register(featureId, provider) {
    if (!featureById.has(featureId)) {
      throw new NativeCapabilityError('Unknown native feature', 'UNKNOWN_FEATURE');
    }
    if (!provider || typeof provider.status !== 'function' || typeof provider.open !== 'function') {
      throw new NativeCapabilityError('Invalid privileged adapter', 'INVALID_ADAPTER');
    }
    if (this.#providers.has(featureId)) {
      throw new NativeCapabilityError('Adapter already registered', 'DUPLICATE_ADAPTER');
    }
    this.#providers.set(featureId, provider);
  }

  unregister(featureId) { this.#providers.delete(featureId); }

  #context(context) {
    // Only non-identifying flags are passed. No page URL or content ever crosses
    // the native integration layer by default.
    return Object.freeze({privateWindow: context?.privateWindow === true});
  }

  async status(featureId, context = {}) {
    const feature = featureById.get(featureId);
    if (!feature) throw new NativeCapabilityError('Unknown native feature', 'UNKNOWN_FEATURE');
    if (context?.privateWindow === true && !feature.privateWindow) {
      return Object.freeze({id:feature.id,label:feature.label,state:'unavailable',reason:'Not available in private windows'});
    }
    const provider = this.#providers.get(featureId);
    if (!provider) return Object.freeze({id:feature.id,label:feature.label,state:'unavailable',reason:'Not integrated in this build'});
    try {
      const status = await provider.status(this.#context(context));
      if (!status || !CAPABILITY_STATES.includes(status.state)) {
        return Object.freeze({id:feature.id,label:feature.label,state:'unavailable',reason:'Invalid native service state'});
      }
      // Readiness is not proof of active shields on the current webpage.
      const reason = typeof status.reason === 'string' ? status.reason.slice(0, 200) : '';
      return Object.freeze({id:feature.id,label:feature.label,state:status.state,reason});
    } catch {
      // Avoid leaking provider exceptions (may contain sensitive URLs/keys).
      return Object.freeze({id:feature.id,label:feature.label,state:'unavailable',reason:'Native service is unavailable'});
    }
  }

  async snapshot(context = {}) {
    const results=[];
    for(const feature of NATIVE_FEATURES) results.push(await this.status(feature.id,context));
    return Object.freeze(results);
  }

  async open(featureId, context={}) {
    const state = await this.status(featureId,context);
    if(state.state !== 'ready') {
      throw new NativeCapabilityError('Native feature is unavailable', 'CAPABILITY_UNAVAILABLE');
    }
    const provider=this.#providers.get(featureId);
    // The adapter rechecks security when activated (TOCTOU protection).
    const result=await provider.open(this.#context(context));
    return result === undefined ? null : result;
  }
}

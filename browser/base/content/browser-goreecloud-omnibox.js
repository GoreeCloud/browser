/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Development-only privileged Gecko URL-bar actions.
 * Hidden until opt-in AND a native first-party adapter reports ready.
 * Firefox's reader-mode-button remains untouched. */
var GoreeCloudOmnibox = {
  pref: "goreecloud.browser.omnibox.experimental",
  definitions: Object.freeze([
    Object.freeze({ id: "privacy-shield", label: "Privacy Shield", privateAllowed: true }),
    Object.freeze({ id: "wardveil", label: "Wardveil Security", privateAllowed: true }),
    Object.freeze({ id: "webspaces", label: "Webspaces", privateAllowed: false }),
  ]),
  providers: new Map(),
  nodes: new Map(),
  inFlight: new Set(),
  generation: 0,
  started: false,
  progressListener: null,

  register(id, provider) {
    if (!this.definitions.some(item => item.id === id) ||
        !provider || typeof provider.status !== "function" ||
        typeof provider.open !== "function" || this.providers.has(id)) {
      throw new Error("Invalid native omnibox adapter");
    }
    this.providers.set(id, provider);
    void this.refresh();
  },
  unregister(id) {
    this.providers.delete(id);
    void this.refresh();
  },
  init() {
    if (this.started) return;
    const parent = document.getElementById("page-action-buttons");
    const reader = document.getElementById("reader-mode-button");
    if (!parent || !reader) return;
    this.started = true;
    for (const item of this.definitions) {
      const button = document.createXULElement("hbox");
      button.id = "goreecloud-" + item.id + "-button";
      button.classList.add("urlbar-page-action");
      button.setAttribute("role", "button");
      button.setAttribute("tabindex", "0");
      button.setAttribute("aria-label", item.label);
      button.setAttribute("tooltiptext", item.label);
      button.hidden = true;
      const image = document.createXULElement("image");
      image.classList.add("urlbar-icon");
      image.setAttribute("src", "chrome://browser/content/goreecloud/icons/" + item.id + ".svg");
      button.appendChild(image);
      button.addEventListener("click", event => {
        if (event.button === 0) void this.activate(item.id);
      });
      button.addEventListener("keydown", event => {
        if (event.key !== "Enter" && event.key !== " ") return;
        event.preventDefault();
        event.stopPropagation();
        void this.activate(item.id);
      });
      parent.insertBefore(button, reader);
      this.nodes.set(item.id, button);
    }
    Services.prefs.addObserver(this.pref, this);
    gBrowser.tabContainer.addEventListener("TabSelect", this);
    this.progressListener = { onLocationChange: () => void this.refresh() };
    gBrowser.addTabsProgressListener(this.progressListener);
    window.addEventListener("unload", () => this.destroy(), { once: true });
    void this.refresh();
  },
  handleEvent(event) {
    if (event.type === "TabSelect") void this.refresh();
  },
  observe(_subject, topic, name) {
    if (topic === "nsPref:changed" && name === this.pref) void this.refresh();
  },
  context() {
    return Object.freeze({ privateWindow: PrivateBrowsingUtils.isWindowPrivate(window) });
  },
  async refresh() {
    const token = ++this.generation;
    if (!this.started) return;
    const allowed = Services.prefs.getBoolPref(this.pref, false);
    const context = this.context();
    for (const node of this.nodes.values()) node.hidden = true;
    for (const item of this.definitions) {
      const provider = this.providers.get(item.id);
      if (!allowed || !provider || (context.privateWindow && !item.privateAllowed)) continue;
      try {
        const result = await provider.status(context);
        if (token !== this.generation || !this.started) return;
        // Ready permits access to a panel, NOT a claim of site protection.
        this.nodes.get(item.id).hidden = result?.state !== "ready";
      } catch {
        if (token !== this.generation || !this.started) return;
        this.nodes.get(item.id).hidden = true;
      }
    }
  },
  async activate(id) {
    const item = this.definitions.find(entry => entry.id === id);
    const node = this.nodes.get(id);
    const provider = this.providers.get(id);
    if (!item || !node || node.hidden || !provider ||
        !Services.prefs.getBoolPref(this.pref, false)) return;
    const context = this.context();
    if ((context.privateWindow && !item.privateAllowed) || this.inFlight.has(id)) return;
    this.inFlight.add(id);
    const generation = this.generation;
    try {
      const status = await provider.status(context);
      if (!this.started || generation !== this.generation ||
          this.providers.get(id) !== provider || node.hidden ||
          !Services.prefs.getBoolPref(this.pref, false) ||
          status?.state !== "ready") return;
      // The provider MUST re-authorize its own current browser/site context.
      await provider.open(context);
    } catch {
      if (this.started) await this.refresh();
    } finally {
      this.inFlight.delete(id);
    }
  },
  destroy() {
    if (!this.started) return;
    this.started = false;
    ++this.generation;
    Services.prefs.removeObserver(this.pref, this);
    gBrowser.tabContainer.removeEventListener("TabSelect", this);
    if (this.progressListener) gBrowser.removeTabsProgressListener(this.progressListener);
    this.progressListener = null;
    for (const node of this.nodes.values()) node.remove();
    this.nodes.clear();
    this.inFlight.clear();
    this.providers.clear();
  },
};

delayedStartupPromise.then(() => GoreeCloudOmnibox.init());

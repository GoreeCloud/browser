# GoreeCloud Browser — native capabilities and omnibox plan

**Status: source-level development contract, not an enabled Gecko browser feature.**

**Repository:** `GoreeCloud/browser`. **Upstream base:** Firefox 158.0 stable; stacked above the Gecko Unified Bookmarks draft. **Existing production Browser:** CEF/Chromium is unaffected. **Design target:** Glaze 1.7 stable-compatible, pending visual/runtime verification.

## Native systems and authority

| Browser-native integration | Authoritative owner | Browser responsibility | Initial safety constraint |
| --- | --- | --- | --- |
| Privacy Shield | GoreeCloud Privacy Shield | First-party protection indicator, site preferences, local controls | Never report blocking/protection without per-page Gecko evidence; don't disable native Firefox protections |
| Wardveil Security | Wardveil Security | Contextual security health and incident details | No unsupported 'safe' verdict, TLS bypass, hidden failure or unsafe IPC |
| Advanced Tab Manager | GoreeCloud Advanced Tab Manager | Native sidebar, tab search/tree, native groups, sessions and bulk actions | Gecko remains authoritative for tabs, windows, sessions, private states |
| Advanced Download Manager | GoreeCloud Advanced Download Manager | Native download surfaces, queue/state/recovery and secure handoff | Keep Gecko download file permissions, quarantine, prompts and path boundaries |
| Advanced Private Window Manager | Browser | Native private-window organization, state and clear controls | Never persist private-only history/session content by default |
| Advanced Profile Manager | Browser | Profile creation/switching, separation, migration and recovery | Separate profile storage/credentials; never auto-migrate or overwrite |
| Webspaces | GoreeCloud Webspaces | Native container glyph, picker, contextual-identity association | One distinct Gecko contextual identity/cookieStoreId per isolated Webspace; no false IP/history isolation claim |
| Search | GoreeCloud Search | Native search-mode/address-field provider with ordinary URL entry | No default transmission of omnibox text; network lookup only on user action/approved setting |
| AI | GoreeCloud AI | Native voluntary assistant side panel, context selection, consent | No automatic forwarding of page text, URLs, credentials or private browsing data |

All services are **privileged Browser integrations**, not required third-party extensions. WebExtensions compatibility remains supported for separately installed extensions. Client adapters must not claim maturity or depend on unverified cloud endpoints.

## Omnibox actions

Exactly four first-party glyphs belong inside the main **page-action region**. Source inspection of Firefox 158 confirms `browser/base/content/navigator-toolbox.inc.xhtml` has a `page-action-buttons` container within `moz-urlbar`, including a **built-in** `reader-mode-button`. Reuse that Reader Mode action instead of duplicating its button and rewriting its eligibility rules.

| Glyph | Accessible button label | Required source | Intended action |
| --- | --- | --- | --- |
| shield | Privacy Shield | Verified Gecko Privacy Shield adapter and current site state | Open Privacy Shield site panel |
| shield-check | Wardveil Security | Verified Wardveil Gecko adapter | Open Wardveil security panel |
| layers | Webspaces | Verified contextual-identity authority | Open site/Webspace picker |
| book-open | Reader Mode | Gecko's native reader-eligibility state | Enter or exit Reader View |

The glyphs use **Glaze** tokenized styles and accessible, non-color state cues. Buttons must support keyboard Tab/Enter/Space, tooltip and screen-reader name, focus visibility, high contrast, theme mode, pointer/touch sizing, responsive overflow, and reduced motion.

Security panel actions are inert while the adapter is unavailable. 'Service ready' is **not** synonymous with 'page protected'. Distinguish disconnected, checking, attention, and evidence-backed per-page protection states in the actual panel; never fabricate them from a configured preference.

## Implementation surface

The two initial modules at `goreecloud/native/capabilities/` are **pure offline models**, not packaged into Firefox yet.

- `catalog.mjs` declares nine first-party capabilities and a conservative parent-process provider registry. Missing, invalid or malfunctioning adapters report unavailable. Only private-window booleans cross its default boundary.
- `omnibox.mjs` produces four accessible glyph descriptors and inert, fixed action intents. It does **not** change settings, navigate, access the network, mutate cookies, read credentials, or send pages to AI.
- Gecko adapter must be reviewed, compiled and wired through the existing `moz-urlbar` page-action chrome and life-cycle events. Never attach an unprivileged web-content listener to invoke privileged controls; all actions require chrome authorization at activation time.
- Keep backend adapters opt-in and disabled until each has unit tests, Gecko integration tests, service contract review, accessibility verification, and incident/recovery handling.

## Delivery gates

1. **Source contracts:** Node tests for nine feature IDs, four accessible glyphs, reader eligibility, private-window gating, secret minimization, failure states and action replay.
2. **Privileged runtime:** signed/reviewed native adapters, build packaging and governed chrome entry points. No enabled UI while dependency modules remain unavailable.
3. **Real feature work:** tab/group/session recovery, download safety, private window lifecycle, profile isolation, Webspace cookie boundaries, Search privacy controls, consented AI requests, and per-page security/privacy evidence.
4. **Runtime acceptance:** full Gecko Linux build, actual browser UI tests and keyboard/screen-reader tests, no-network and private-mode tests, release gates and rollback.
5. **Platforms:** Linux/Windows native Gecko UI; Android GeckoView-native equivalents where supported; iOS must comply with platform-mandated browser-engine and extension restrictions. Cross-platform names and user experience can be consistent even when engine implementation differs.

**Not implemented:** browser chrome glyphs, live Privacy Shield/Wardveil evaluation, protected data movement, downloads, profile manager, Webspaces session routing, live Search or AI request flow, integration security acceptance, or compiled runtime evidence.

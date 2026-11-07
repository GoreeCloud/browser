# GoreeCloud Browser — Linux feature status and Privacy Shield roadmap

**Authority:** GoreeCloud founder's November 6, 2026 Linux-browser requirements and
the accompanying Privacy Shield proposal. This file distinguishes implemented,
tested, partial, and planned behavior; a feature description is not proof of security.

## Founder-directed desktop interface

| Requirement | v0.2.3 development-preview status |
| --- | --- |
| Light theme default; user preference respected | Implemented: new users light, existing Light/Dark QSettings preserved |
| Toolbar exact order | Implemented: Back, Forward, Refresh, Home, Omnibox, Bookmark, Extensions, Settings, Profile |
| Premium glyph-only toolbar buttons | Implemented using native SVG-rendered monochrome icons with accessible names/tooltips |
| Omnibox | Implemented: Verification and Permissions on left, input in center, Webspaces on right. Permissions opens a real per-Webspace stored-decision manager, not a placeholder |
| Native Home and distinct New Tab | Retained, separately rendered, accessible at 700x480 by scrolling |
| Advanced Tab Manager | Initial native search/switch/close by tab and Webspace; advanced grouping/sleep/session restore planned |
| Advanced Download Manager | Initial active/cancel/progress/session view atop private staged no-overwrite saves; malware scanning and persisted history planned |
| Webspaces (Firefox-style containers) | Initial custom Webspaces with **separate Qt WebEngine profiles**, cookie jars, cache and HTML5 storage; real local HTTP isolation test. No per-Webspace process, IP or proxy isolation; no account sync |
| Theme Manager | Light/Dark selection in Settings |
| Privacy Shield site exceptions | Native exact-first-party-host manager with explicit confirmation before disabling OISD/StevenBlack and fallback tracker-domain blocking; restores protection immediately for new requests. Browser-wide across Webspaces (not per-Webspace) |
| Extensions button | Status entry only: installing extensions remains unsupported |
| Settings: ChatGPT.com prompt integrations | Native five editable/copyable prompt templates and explicit open action; **no automatic page transfer, prompt submission or account access** |
| Settings: Cookies Manager | Cookie names/domains/flags and delete controls for the active Webspace; cookie values hidden; site-data reset is separate |
| Settings: Website Permissions | Stored Qt permission decisions scoped to active Webspace and optional current origin, with individual/confirmed bulk reset to Ask. Transient capture permissions may not appear and active streams may require tab closure |
| Wardveil Linux WebEngine sandbox preflight | Rejects explicit sandbox-bypass environment/command-line options before browser profile access; separate offline Linux `/proc` diagnostic observed one seccomp-filtered namespaced descendant on one host. No complete renderer/process-role certification |
| Settings: DNS Manager | Honest read-only OS DNS status; custom DNS/DoH/DoT not implemented |
| Advanced Proxy Manager | Initial **global** System/Direct/HTTP/SOCKS5 configuration, validated host/port, applied on next restart through Qt Network before WebEngine starts. HTTP loopback route verified; SOCKS5, authentication, PAC and per-Webspace routing not accepted |

## Built-in adblocking: enabled lists versus verified filtering

The default enabled subscriptions are:

- [OISD Small](https://small.oisd.nl/), mainly domain-based ad blocking
- [StevenBlack Unified](https://github.com/StevenBlack/hosts), consolidated host-domain data

The browser asynchronously fetches only the fixed upstream HTTPS list URLs on
startup if enabled caches are missing or older than 24 hours; the user may also
refresh manually. This discloses the user's connection IP to the two list hosts,
but sends **no browsing URLs, cookies or account identifiers**. Lists undergo
strict UTF-8/domain validation, size and count limits, and atomic private-cache
replacement. A failed update retains last-known-good rules and reports failure.
The UI distinguishes a checked-but-not-yet-loaded list from an active domain set.

Filtering is restricted to **third-party HTTP(S) subresource requests** with
matched domains, preserving top-level navigations to avoid misleading rewrites.
This is not cosmetic filtering, anti-fingerprinting, script injection, complete
malvertising protection or DNS-level blocking. Small local tracker fallback rules
remain independently toggleable. Exact first-party site exceptions (browser-wide across Webspaces) are now
available with strict hostname validation, explicit confirmation and reversible
removal. A live Qt WebEngine loopback HTTP test now proves blocked-by-default,
allowed-after-exception and blocked-again-after-removal behavior for a third-party
image request, with only process-local `.invalid` host resolution. This does not
establish universal real-site compatibility or DNS-level filtering. More granular
subresource-domain allow/deny rules and compatibility management remain planned. Outside System proxy mode, list updates are paused to
avoid bypassing the configured Qt WebEngine proxy with a separate downloader.

### Privacy Shield's nine proposed protection layers

| Proposed layer | Current evidence and remaining work |
| --- | --- |
| 1. URL Privacy | Conservative cleaning of *entered/pasted* addresses; signed queries preserved. Clicked links, redirects, path rules and bulk sanitization not implemented |
| 2. Network Privacy | Verified domain-list parser, cache and Qt request interceptor; broad behavioral analytics and exhaustive blocking not implemented |
| 3. Cookie & Identifier Privacy | Webspace-separated cookie jars; per-Webspace cookie inspector and deletion; supercookie identification and advanced third-party restrictions pending |
| 4. Fingerprinting Protection | Detection/canvas defenses not implemented |
| 5. CDN Privacy | Local resource emulation, version mapping and library-replacement verification not implemented |
| 6. Embedded Content Privacy | Click-to-activate social media, widget and video replacements not implemented |
| 7. Privacy Preference Enforcement | GPC/DNT signals not implemented; privacy-domain filtering does not depend on voluntary site signals |
| 8. Adaptive Protection Intelligence | Manual and age-based fixed-source list updates implemented; behavior learning and signed intelligence feeds not implemented |
| 9. Privacy Management | Native settings, per-Webspace cookie controls, per-Webspace stored permission review, exact-first-party-host browser-wide privacy exceptions and truthful filter status; granular resource rules, counters, import/export and diagnostics pending |

The 115 proposed Privacy Shield capabilities are a **product roadmap**, not
115 released or fully implemented features. GoreeShield owns privacy decisions.
Wardveil remains responsible for cybersecurity, malware defenses, dependency
review, incident response and browser-engine isolation.

## Mandatory future acceptance

- Complete Qt WebEngine role-specific process sandbox acceptance across Linux
  distributions; the explicit bypass guard and one-host `/proc` seccomp/namespace
  observation do not prove that all renderer/network/GPU processes are isolated.
  Confirm transient permission/device capture handling, TLS/origin display,
  download threat detection, site-data completeness and cookie/data retention. Stored permission reset is verified,
  but real camera/microphone capture termination remains unqualified.
- Prove actual per-Webspace storage isolation on representative Linux hosts;
  close/rollback handling for all persisted profiles.
- Add rigorous high-contrast, keyboard/zoom, assistive technology and human
  accessibility acceptance beyond automated tooltips and geometry.
- Qualify SOCKS5 route and proxy/fallback leakage, system DNS and secure DNS
  options, credential security, site exceptions and privacy telemetry.
- Implement extension isolation only after reviewing WebEngine support;
  untrusted arbitrary extensions must never be loaded as Python plugins.
- Sign and package Linux builds; establish patch cadence and rollback.
- Do not activate GoreeCloud Search until independently verified stable.
- Windows/macOS remain deferred until Linux release gates are satisfied.

**This is a development preview, not a security-hardened daily-driver release.**

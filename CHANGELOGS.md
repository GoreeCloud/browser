# GoreeCloud Browser — Changelogs

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## 2026-10-02 — Android Desktop site and Clear browsing data controls

### Added

- Added a Browser-owned **Desktop site** control that changes the current Android Browser session between mobile and desktop-style presentation, derives its desktop user agent from the active WebView engine identity, and enables wide-viewport/overview behavior without pinning a separate Chromium version.
- Added a confirmed **Clear browsing data** flow that explicitly lists data to remove versus settings to preserve before clearing Android WebView cookies/sign-in state, website storage, cache, form data, navigation history, and SSL preferences.
- Added unit coverage for desktop user-agent derivation and browsing-data scope plus managed-emulator coverage that verifies WebView cookie removal.

### Changed

- Desktop site state is preserved across Android Activity recreation but remains session-local rather than becoming a durable or synchronized preference.
- Clear browsing data returns Browser to the local Start page and clears prior WebView navigation history after that local page finishes loading.
- Advanced the Android beta artifact identity to `0.1.0-beta.1+android.9` / versionCode `10009` and advanced the CI package/version verification contract with it.

### Privacy boundary

Clear browsing data does not erase Browser preferences or Android app permissions. It is an app-wide Android WebView data reset in the current Development shell, not profile-scoped Private/Isolated Private or Close & Forget acceptance.

### Acceptance boundary

These are Development local browsing controls. Representative-device usability, destructive-action accessibility, TalkBack/Switch Access, localization/RTL, profile-scoped deletion, private-context isolation, production approval, Stable, and Anchor qualification remain open.

## 2026-10-02 — Android Find in page and page text-size controls

### Added

- Added a Browser-owned **Find in page** Glaze bottom sheet backed by the active Android WebView's local find facility, with bounded 512-character input, live match status, Previous/Next navigation, and highlight cleanup on close.
- Added Browser-owned **Page text size** controls from 75% through 200% in 25-point steps with Reset to 100%.
- Added focused unit coverage for find-status presentation and bounded text-zoom behavior, plus managed-emulator verification of the default WebView text-zoom state.

### Changed

- Page text zoom now persists as an application-local Browser preference across pages and launches without changing Android system font scale.
- Advanced the Android beta artifact identity to `0.1.0-beta.1+android.8` / versionCode `10008`.
- Updated the feature inventory and user manual so Find in page is explicitly local-only and page text zoom is not represented as synchronized Browser state.

### Privacy boundary

Find in page does not delegate text to GoreeCloud Search or another provider. Page text zoom stores only the selected percentage as local application preference state.

### Acceptance boundary

These are Development browser-utility capabilities. They do not establish representative-device accessibility, large-text interaction quality, localization/RTL behavior, form-factor acceptance, Browser Sync inclusion, production approval, Stable, or Anchor qualification.

## 2026-10-02 — Android origin disclosure, site information, and privacy-status controls

### Added

- Added a Browser-owned **Site information** bottom sheet that presents the current website origin and HTTPS/HTTP transport classification without turning HTTPS into a certificate, Wardveil, or trust verdict.
- Added a scrollable Browser-owned **Privacy & security** bottom sheet that truthfully exposes enforced Android/WebView defaults and explicit fail-closed Search, permission, download, and provider boundaries.
- Added focused unit coverage for page-address disclosure, site-information presentation, protection-state rendering, numeric-host handling, and port-range validation.

### Changed

- Page-address Copy/Share actions now appear only when the current surface has a canonical Browser-approved HTTP(S) URL, preventing Browser-owned `goreecloud://`, local-resource, data, blob, file, or content addresses from being disclosed through Android clipboard/share flows.
- Android navigation now rejects zero/out-of-range ports and ambiguous or invalid numeric IPv4 host forms in addition to the existing credential, unsupported-scheme, and control-character fail-closed checks.
- Reconciled Browser capability records to the merged Linux Wayland pointer/wheel/focus forwarding in PR #111 and updated the user manual to the current Android beta identity and actual `BrowserActivityV2` behavior.
- Updated the documented canonical Glaze repository identity from the retired `GoreeCloud/glaze-ui` name to `GoreeCloud/glaze`.

### Acceptance boundary

These changes are Development hardening and truthful Browser-owned status UX. The Site information surface does not replace engine certificate details, the Privacy & security surface does not manufacture Privacy Shield/Wardveil/Policy/Search authority, and this change does not establish representative-device accessibility/usability, production signing, Release Candidate, Stable, or Anchor acceptance.

## 2026-09-29 — Linux Wayland page rendering integrated; pointer interaction candidate

### Changed

- Integrated the CEF windowless/software-rendering path after exact-head CI verified HTTPS completion and materially non-uniform page frames, and representative Zorin OS 17.3 Wayland evidence showed the real `https://example.com/` page inside the Browser content surface.
- Preserved the native X11/XWayland child-window rendering path.
- Advanced a separate Development candidate that forwards GTK pointer motion, mouse buttons, wheel input, and focus into the windowless CEF browser without duplicating native-child input.
- Stage the canonical Browser SVG with GTK development binaries so the desktop chrome can render product artwork when launched from the build directory.
- Correct stale Browser fallback copy that still described native Wayland page rendering as unavailable.

### Acceptance boundary

Integrated Wayland page pixels are Development evidence, not production acceptance. The pointer/branding continuation remains candidate work until exact-head automated and representative-device interaction evidence exists. Keyboard/IME, cursor propagation, popup/context-menu behavior, drag-and-drop, clipboard, accessibility, accelerated rendering, sustained performance, Browser-wide Glaze acceptance, production approval, Stable, and Anchor remain open.

## 2026-09-28 — Linux Wayland CEF software-rendering candidate

### Changed

- Added a Browser-owned software-frame surface contract for engine renderers that cannot attach a native child window.
- Added CEF windowless/off-screen rendering support with `CefRenderHandler` view metrics and BGRA frame delivery while preserving the existing X11/XWayland child-window path.
- Added GTK drawing-area presentation of the software CEF frame and automatic fallback selection on non-X11 GTK backends.
- Added a Development-only `GOREECLOUD_BROWSER_FORCE_WINDOWLESS=1` validation switch and a Core CI runtime gate that requires both HTTPS main-frame completion and an observed software-frame presentation.

### Acceptance boundary

This source change is a Development render candidate until the changed exact head passes applicable CI and the owner-device Wayland test shows real page pixels. It does **not** yet establish page input/IME/popup/drag-and-drop/accessibility correctness, accelerated rendering, sustained performance, Browser-wide visual acceptance, production approval, Stable, or Anchor maturity.

## 2026-09-28 — Linux Wayland-session startup resilience

### Changed

- Allow the GTK3 GoreeCloud Browser shell to start on an available non-X11 GTK display backend instead of terminating the entire application at the host boundary.
- Preserve the current CEF X11/XWayland child-surface contract and present an explicit renderer-compatibility surface when that native page surface is unavailable.
- Resolve persistent CEF profile cache paths against the Browser cache root so the default relative `profile` path no longer falls back to in-memory storage solely because it is relative.
- Correct the stale Android Glaze documentation baseline from V1.5/1.5.1 to the current V1.6/1.6.0 Stable source baseline.

### Acceptance boundary

This is a Development resilience and documentation correction. It does **not** implement native Wayland CEF page rendering, establish visible web-page pixels on Wayland, close representative-device visual acceptance, or promote Browser to Release Candidate, production-approved, Stable, or Anchor maturity.

## 2026-09-26 — Immutable Linux CEF archive identity hardening

### Changed

- Preserved the current CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` runtime candidate while pinning the reviewed Linux minimal archive to both SHA-1 and SHA-256 identities.
- Require the upstream checksum sidecar, downloaded archive, cached archive, and extracted provenance record to match the repository-owned digest pair rather than accepting future bytes published under the same archive name.
- Made the CMake CEF version pin source-owned rather than a user-overridable cache value.
- Extended the offline CEF bootstrap contract gate to assert the immutable archive identities and recorded the exact dependency constraint in the Platform Contract manifest.

### Acceptance boundary

This is Development supply-chain hardening only. It does not change the already verified renderer-start or HTTPS-navigation evidence and does not establish visible page pixels, representative-device interaction, long-session stability, Browser-wide GLAZE UI acceptance, Release Candidate, production approval, Stable, or Anchor maturity.

## 2026-09-26 — Linux CEF HTTPS navigation smoke accepted

### Changed

- Added bounded CEF main-frame load start, load-end, and load-error diagnostics behind the existing runtime-diagnostics boundary.
- Advanced the Ubuntu 22.04 sandboxed CEF smoke from renderer-process detection to successful engine-level HTTPS navigation.
- Exact-head Core CI now requires both a Chromium renderer subprocess and a main-frame `OnLoadEnd` with HTTP 200 for `https://example.com/`.
- The smoke fails closed on a CEF main-frame load error.

### Acceptance boundary

This proves engine-level HTTPS navigation completion in the pinned sandboxed CEF runtime under Ubuntu 22.04 CI. It does **not** yet prove visible page pixels inside the GoreeCloud GTK child surface, representative-owner-device interaction, navigation-control behavior against visible content, TLS/certificate UX, renderer crash recovery, private-context isolation, accessibility, performance, packaging acceptance, Release Candidate, production approval, Stable, or Anchor maturity.

## 2026-09-26 — Linux CEF sandboxed runtime smoke accepted

### Changed

- Restacked the Linux CEF runtime milestone onto current `main` and revalidated it at exact head.
- Verified the repository-pinned CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` candidate through the Ubuntu 22.04 compile gate.
- Verified the CEF runtime payload, sandbox-preserving Xvfb + D-Bus startup, Browser-process initialization, subprocess role handling, and creation of a Chromium renderer subprocess in Core CI.
- Preserved the Linux Chromium sandbox rather than bypassing it to obtain a green runtime result.
- Kept the initial Browser URL outside inherited CEF subprocess positional arguments.
- Integrated the current CEF client/media-probe/runtime compatibility fixes required by real compilation and launch validation.

### Acceptance boundary

This is Development runtime-start evidence. It proves that the exact Browser candidate can compile and launch the pinned CEF runtime and reach a Chromium renderer subprocess under Ubuntu 22.04 CI with sandboxing preserved. It does **not** yet establish visible page pixels, representative-owner-device HTTPS rendering/navigation, TLS/certificate UX, renderer crash recovery, private-context isolation/cleanup, accessibility, performance, packaging acceptance, Release Candidate, production approval, Stable, or Anchor maturity.

## 2026-09-25 — Android browser-chrome bidirectional-text hardening

### Changed

- Added a shared Android Browser chrome presentation boundary that strips Unicode bidirectional formatting controls from untrusted page-title and unfocused-address text.
- Applied the boundary to Android page-title presentation and condensed omnibox presentation without changing the full URL used for navigation authority.
- Added focused JVM regression coverage for bidi override and isolate characters in Browser-owned title/address surfaces.
- Preserved ordinary Browser navigation, Search, permissions, networking, and engine authority boundaries.

### Acceptance boundary

This is Development presentation hardening only. It does not establish complete IDN/confusable/origin/certificate/bidirectional-text acceptance, representative-device usability or accessibility, Browser-wide GLAZE UI acceptance, production signing/distribution, Release Candidate, production approval, Stable, or Anchor product maturity.

## 2026-09-25 — Linux CEF render-candidate compile validation

### Changed

- Added an exact-head Ubuntu 22.04 Core CI job that bootstraps and compiles the pinned CEF 152 / Chromium 152 Linux render candidate with GTK/X11 dependencies.
- Corrected the CMake C-language probe requirement, stale Linux smoke source path, CEF media-probe bridge/API usage, render-app construction, and GoreeCloud exception/RTTI build requirements exposed by real compilation.
- Verified the resulting exact candidate through the repository Core CI render-build gate before integration.

### Acceptance boundary

Successful CI compilation establishes Development buildability of the pinned Linux render candidate. It does not establish representative-device HTTPS rendering, TLS/certificate presentation, renderer crash handling, private-context isolation/cleanup, accessibility, performance, packaging, Release Candidate, production approval, Stable, or Anchor product maturity.

## 2026-09-25 — Android Search-query boundary hardening

### Changed

- Added one shared Android Search-boundary text-safety policy used before omnibox classification and again before remote Search request construction.
- Reject C0/C1 control characters and the narrow Unicode Bidi_Control formatting set before trimming or delegation, while preserving ordinary Unicode shaping such as ZWJ emoji.
- Bounded normalized free-text Search input to 2,048 characters before transport construction.
- Reject Search capability evidence that advertises more than the Browser-supported 100-result maximum.
- Added focused JVM regression coverage for leading/trailing/interior controls, bidirectional formatting controls, exact-limit and over-limit queries, ordinary Unicode shaping, and incompatible capability limits.
- Preserved direct structurally valid HTTP(S) navigation and all existing Privacy Shield, Identity, production-capability, POST-body, no-telemetry, and fail-closed remote-Search boundaries.

### Acceptance boundary

This is Development input/contract hardening only. It does not enable remote Search, establish live Search/Privacy Shield/Identity authority, prove representative physical-device usability, complete Unicode confusable/origin trust acceptance, satisfy Browser-wide GLAZE UI acceptance, or establish Release Candidate, production, Stable, or Anchor product maturity.

## 2026-09-25 — Pinned Linux CEF render bootstrap

### Changed

- Pinned the current Linux render milestone to CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` using the official CEF Linux x86_64 minimal binary distribution.
- Added a Browser-owned bootstrap that fetches the official CEF checksum first, verifies the archive, rejects unsafe extraction members, records local SHA-256 provenance, and reuses only a matching accepted cache.
- Added CMake validation that rejects a CEF root whose `cef_version.h` does not report the exact repository pin.
- Added one-command Linux render-candidate build and sandbox-preserving launch scripts.
- Propagated the host Browser process `argc/argv` into CEF browser-process initialization and corrected the default subprocess executable name.
- Corrected the X11 parent-window conversion to the integral CEF Linux window-handle type.
- Added an offline CI contract gate that syntax-checks the bootstrap/launch scripts and verifies the bootstrap and CMake pins cannot silently diverge.

### Acceptance boundary

This establishes reproducible Development dependency/bootstrap evidence only. It does not establish successful CEF compilation on a representative machine, HTTPS rendering, private-context runtime acceptance, render-capable Beta status, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Linux panel-dismissal and visual-density follow-up

### Changed

- Added explicit close affordances to Browser-owned Linux Development panels and restored the prior internal/web content surface when a panel is dismissed.
- Added Escape panel dismissal plus Ctrl+K and F6 unified-location focus alongside the existing keyboard-first Browser controls.
- Normalized GTK symbolic icon sizing to 20 px and changed the Downloads toolbar glyph to a more reliable native save/download symbol.
- Pulled first-party cards and Browser panels upward from the vertical dead zone while preserving Glaze spacing and responsive minimums.
- Reduced the visual prominence of the Development lifecycle badge without removing lifecycle truth.
- Corrected literal escaped-newline text in the Linux build guide and documented the current keyboard control set.

### Acceptance boundary

This follow-up remains Development presentation behavior. It does not establish renderer integration, complete accessibility acceptance, production-grade panel behavior, Release Candidate, production approval, or Stable/Anchor maturity.

## 2026-09-25 — Linux polish, semantic panels, and keyboard navigation

### Changed

- Replaced the generic Linux `Browser tool` placeholder with semantic Glaze panels for Bookmarks, Reader Mode, Privacy Shield, Wardveil Security, Clipboard, DNS, Proxy, Search-unavailable, and live Development Downloads presentation.
- Added keyboard-first desktop navigation for location focus, new/close tab, forward/back tab cycling, reload, Back/Forward, and Home without bypassing Browser-owned command routing.
- Added explicit next/previous tab lifecycle operations to `WindowController` and smoke coverage for cyclic tab activation.
- Added a structured Settings section grid while keeping non-implemented controls visibly non-authoritative.
- Simplified redundant first-party Development badges so the start surfaces retain one bounded renderer/status indicator while the top chrome continues to expose Development lifecycle.
- Added pure Browser panel-presentation contracts and smoke coverage so provider-owned privacy/security/network truth remains fail-closed.
- Reconciled stale implementation-status references from superseded Tabmark artwork to the canonical Browser compass identity.

### Acceptance boundary

This tranche remains Development source work. It does not establish renderer integration, live Bookmarks/Privacy Shield/Wardveil/DNS/Proxy authority, complete Settings controls, representative-device accessibility/performance acceptance, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Android 15 managed-emulator Browser smoke

### Changed

- Added AndroidX instrumentation support and an exact-head Android 15 managed-emulator lane.
- Added runtime smoke coverage for Browser-owned chrome controls, initial Back/Forward disabled state, package identity, conservative WebView file/content/mixed-content/cookie defaults, and fail-closed free-text Search.
- Updated the APK build lane to assemble the instrumentation APK before the dependent emulator job.
- Reconciled Android Beta documentation from stale V1.5.1 wording to the integrated GLAZE UI V1.6 / 1.6.0 source and qualification mapping.
- Preserved the branding-provenance validation and `assets/branding/**` Android workflow trigger integrated on current main.

### Acceptance boundary

Managed-emulator evidence is Development evidence only. It does not establish representative physical-device launcher rendering, accessibility, practical network browsing, sustained performance, OEM behavior, production signing, release qualification, Production Acceptance, or Anchor product maturity.

## 2026-09-25 — Android launcher branding provenance guard

### Changed

- Added fail-closed source checks that pin the Browser-local full-color and monochrome branding assets to the current canonical `GoreeCloud/branding-assets` Git blobs.
- Pinned the reviewed Android launcher background, foreground, and monochrome packaging derivatives so silent redraws or substitutions fail CI.
- Tightened adaptive-icon validation to require the expected Browser launcher drawable references.
- Updated Android Beta workflow path filters so branding-only changes under `assets/branding/**` trigger launcher identity validation and APK verification.
- Preserved the newer canonical Browser artwork and branding documentation already integrated by the Linux visual-acceptance tranche rather than reapplying superseded PR #75 artwork.

### Acceptance boundary

These checks establish source provenance and CI coverage only. They do not establish physical-launcher rendering quality, representative-device acceptance, accessibility, production signing, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Linux real-device visual acceptance follow-up

### Changed

- Corrected first-party internal-page presentation so `goreecloud://` implementation URLs no longer leak into the visible tab title or omnibox presentation for Browser-owned internal surfaces.
- Replaced ambiguous primary Unicode toolbar glyphs with platform-native symbolic icons with text fallbacks.
- Added an interactive Linux tab strip with visible tab titles and Development create/activate/close behavior.
- Added a functional first-party start-surface search field plus Bookmarks, Downloads, and Settings quick actions.
- Synchronized the Linux Browser full-color and monochrome consumer artwork to canonical `GoreeCloud/branding-assets` compass sources.
- Reconciled stale Browser-local branding documentation that still described the superseded aqua Tabmark or artwork-pending state.

### Authority and acceptance boundary

Canonical Browser branding is verified at `GoreeCloud/branding-assets` main revision `a831479976fd3f82dc31cf9b7785145a757e7e28`. Exact-head Browser Core CI and security workflows passed for the integrated Linux follow-up; fresh owner-device rendered review remains required. Android branding validation did not run for branding-only changes under the prior workflow path filter, which is corrected by the launcher-provenance follow-up below. This does not establish Browser-wide Glaze acceptance, renderer acceptance, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Glaze V1.6 contract and responsive follow-up

### Changed

- Reconciled stale V1.5.1 qualification assertions and evidence anchors that remained after the Linux V1.6 source migration.
- Added exact V1.6 smoke gates for accepted release source, qualification source, qualification evidence integration, rollback baseline, and Browser presentation invariants.
- Migrated the Android-native Glaze contract and tests from V1.5.1 to V1.6 / 1.6.0 and added explicit disabled-control presentation.
- Hardened the GTK first-party cards for narrower windows by removing 560–600 px minimum-size assumptions, constraining text measure, and allowing status chips to wrap.
- Reconciled the Browser adoption record with canonical GLAZE UI V1.6 Anchor lifecycle terminology while retaining Stable as compatibility release-channel vocabulary.

### Lifecycle boundary

This follow-up remains Development source evidence. Browser-wide rendered/native, accessibility, large-text, localization/RTL, representative-device/form-factor, performance, rollback, renderer-integrated, release, and production acceptance remain open.

## 2026-09-25 — Linux desktop Glaze V1.6 source migration tranche

### Changed

- Mapped Browser source guards from historical GLAZE UI V1.5.1 to exact current Stable GLAZE UI V1.6 / 1.6.0 release source `a7180679ea851389e0f3004515f9a25f420e716d`.
- Reworked the GTK/X11 Development chrome around a compact active-tab surface, primary navigation capsule, bounded secondary tools popover, branded first-party surfaces, and explicit Development state.
- Added friendly first-party tab titles for New Tab, Home, Settings, and Private Browsing.
- Reordered initial GTK presentation so the visible first-party surface is selected after the window is shown.
- Lowered the project CMake floor to 3.22 and moved the dedicated GTK beta-shell CI lane to Ubuntu 22.04 for Zorin/Ubuntu 22.04-class compatibility.
- Corrected stale Browser Glaze documentation that referenced an obsolete Glaze 2.0 baseline.

### Lifecycle boundary

This tranche is Development source implementation only. It does not establish rendered/accessibility/device/performance acceptance, render-capable CEF/Chromium desktop acceptance, Release Candidate, production approval, or Stable status.

## 2026-09-22 — Repository feature/changelog governance migration

### Added

- `IMPLEMENTED-FEATURES.md` as the authoritative implemented-feature inventory.
- `PLANNED-FEATURES.md` as the authoritative planned/incomplete-feature inventory.
- `CHANGELOGS.md` as the authoritative repository change-history record.

### Changed

- Retired the repository `FEATURE-ROADMAP.md` control model.
- Removed the obsolete requirement to synchronize feature-roadmap authority with Google Drive.
- Preserved `FEATURES.md`, `SPECIFICATIONS.md`, supporting contracts, Git history, pull requests, workflow evidence, and `NOTES.md` as product/specification/evidence sources rather than duplicate feature-lifecycle authority.
- Recorded the current V1.5.1 Browser source mapping as migration-required against current Official Stable GLAZE UI V1.6 / 1.6.0 rather than silently rebinding earlier evidence.

### Lifecycle boundary

This migration changes documentation/control-plane authority only. Browser remains Development/non-Stable. Representative-device usability/accessibility/performance, current V1.6 application acceptance, platform integrations, recovery, production signing/deployment, Release Candidate, and Stable qualification remain open.

## Historical change evidence

Historical implementation and validation evidence remains preserved by Git history, merged pull requests, repository documentation, workflow records, exact-source evidence, and product-specific governed evidence. Future material integrated changes must be recorded here.

## Maintenance rule

Record material integrated changes here with enough exact repository evidence to distinguish authoritative `main` state from draft/unmerged work. Do not convert source presence, an installable beta artifact, or green CI into production or Stable claims.
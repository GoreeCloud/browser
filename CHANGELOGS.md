# GoreeCloud Browser — Changelogs

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

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
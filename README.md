# GoreeCloud Browser

GoreeCloud Browser is GoreeCloud's first-party native web browser. GoreeCloud owns the browser product, chrome, user experience, data model, privacy controls, security integrations, synchronization behavior, session model, and first-party service boundaries. Mature web engines remain replaceable implementation dependencies rather than the identity or permanent source of product authority.

## Status

**Development — `0.1.0-beta.1`, not production-approved or Stable.**

The repository contains an engine-independent native browser core, a Linux GTK/X11 shell, a render-capable desktop Chromium/CEF integration path under continued qualification, and an installable Android beta target using Android System WebView/Chromium as its web-engine dependency.

Source, CI, unit, build, emulator, or installable-beta success does not by itself establish representative-device, accessibility, privacy, security, recovery, production-signing, release, or Stable qualification.

## GLAZE UI V1.4 requirement

GoreeCloud Browser tracks the **latest approved Stable GLAZE UI release**. The current repository-local migration target is:

- Product identity: **GLAZE UI V1.4 — Optical Intelligence**
- Machine version: **`1.4.0`**
- Exact merged Stable source revision: **`84cb3db4884042f0fa25ed6d475a127fb110f596`**
- Browser policy: **`latest-approved-stable`**
- Browser product conformance: **not yet accepted**
- Human/manual/physical-device optical validation: **V1.4.1 follow-up**

The previous repository state contained split Glaze authority: Android presentation contracts targeted the retired 2.2 line, shared desktop/native C++ metadata still targeted 2.0, and the Platform Contract compatibility floor remained on 1.1. The V1.4 migration removes that split and establishes one exact design-system authority across Browser-owned Android and desktop/native source contracts.

Central Glaze Stable status does not automatically certify Browser. Browser remains `applicable-migration-required` and product-level `nonconformant` until Browser-specific rendered, accessibility, adaptive, representative-device, platform, privacy/security, release, and production acceptance are complete.

See [`docs/GLAZE_UI_ADOPTION.md`](docs/GLAZE_UI_ADOPTION.md).

## Trusted browser chrome rule

**Web content is an untrusted rendering plane. Browser chrome is trusted application UI.**

GLAZE UI V1.4 Optical Intelligence must never allow webpage appearance to impersonate Browser, origin, certificate, security, privacy, permission, download-safety, or authorization state.

Browser therefore prohibits the following from implicitly driving trusted Browser chrome optics:

- webpage pixels or dominant colors;
- CSS/theme colors;
- favicon colors;
- document text or media frames;
- origin/domain identity;
- browsing-history or query content;
- certificate/security indicators;
- Privacy Shield state;
- Wardveil Security state; or
- remote/telemetry-derived visual context.

Trusted-chrome Environmental Color Memory influence is **0%**. Decorative page-derived tinting is disabled. Optical appearance cannot manufacture security or privacy truth.

The Android `GlazeBrowserOptics` contract and shared C++ `BrowserGlazeOpticalPolicy` encode these restrictions directly.

## Accessibility and optical fallback

Browser preserves effects-free operation as a first-class path. Blur, transparency, animation, and decorative color are not usability dependencies.

Accessibility precedence is:

1. Forced Colors or the closest supported platform equivalent.
2. Reduced Transparency / solid effects-free presentation.
3. Increased Contrast and visible focus.
4. Security, privacy, origin, certificate, permission, and workflow truth.
5. Task clarity, readable text, and interaction-target requirements.
6. Decorative material and optical expression.

On Android, Reduced Transparency and Forced Colors resolve to solid-accessible behavior with zero blur and full semantic protection. Increased Contrast increases protective clarity without enabling decorative page-driven tinting.

General touch-oriented interaction targets remain at least 48 dp. A 56 dp Touch Assistance/far-view floor remains represented where applicable; source representation does not claim that a product-wide platform preference is already wired.

## Product identity

GoreeCloud Browser is the sole normal user-facing Browser product identity. Upstream and third-party implementation technology may appear where technically, legally, operationally, diagnostically, or compliance-wise required, but it does not become GoreeCloud product branding.

## Architectural direction

GoreeCloud Browser is **not** intended to be a conventional Chromium skin or permanently deep Chromium fork.

GoreeCloud-owned application layers remain separated from the rendering engine behind explicit interfaces. Chromium/Blink is the initial engine foundation, while the architecture keeps room for platform-native engines, alternative engines, and future GoreeCloud-controlled engine work when there is a concrete privacy, security, performance, compatibility, platform, or product reason.

### Core systems

- **GoreeCloud Browser** — native application, chrome, tabs, profiles, preferences, history, permissions, downloads, sessions, and product experience.
- **Browser Engine Layer** — GoreeCloud-owned boundary between the product and web-engine implementations.
- **Chromium/Blink / Android System WebView** — current rendering, JavaScript, Web APIs, media, graphics, accessibility, and web-platform foundations.
- **GLAZE UI** — authoritative presentation and interaction system for Browser-owned UI.
- **GoreeCloud Search** — approved Browser search/current-information provider relationship for non-URL query workflows.
- **Privacy Shield** — privacy and content-protection authority.
- **Wardveil Security** — security classification, protection state, evidence-backed Browser security status, and download-verification authority where applicable.
- **Everkeep** — approved Browser backup, recovery, portability, and session-preservation authority.
- **GoreeCloud Identity** — profiles, device authorization, synchronization identity, account-bound preferences, authentication, and authorization.
- **GoreeCloud Vault** — credentials, passkeys, secrets, secure autofill, and protected authentication material.
- **GoreeCloud DNS** — DNS resolution and resolver privacy/policy integration.
- **GoreeCloud Network** — networking and private-connectivity authority.
- **GoreeCloud Mesh** — first-party service coordination and capability governance.
- **GoreeCloud Wayfinder** — Browser-owned navigation, organization, transfer, capture, session, and workflow feature family.
- **Media Hover** — Browser-owned contextual media interaction with explicit privacy/security/service boundaries.

## Android beta

The repository includes a real installable Android beta target using package `io.goreecloud.browser.beta`, version `0.1.0-beta.1+android.5`, versionCode `10005`, Android API 26+, and target API 35.

Android CI runs JVM unit tests, lint, APK assembly, signature verification, package/label validation, SHA-256 generation, and artifact upload. Android System WebView/Chromium remains the rendering-engine dependency while GoreeCloud owns browser chrome, navigation/search policy, privacy defaults, permission behavior, Glaze mapping, and beta capability gates.

The Android source preserves:

- no action bar or development banner in normal browsing;
- full-width top omnibox;
- vector Browser navigation icons;
- separate 56 dp bottom navigation toolbar;
- overlaid progress indication;
- 48 dp ordinary interaction-target floor;
- native light/dark adaptation;
- focus and pressed states;
- effects-free fallback behavior;
- leading-hostname address presentation;
- scroll-aware top-chrome collapse; and
- Browser-owned menu-sheet presentation.

The V1.4 migration additionally pins the exact current Stable authority and adds the trusted-chrome optical policy. It does not claim complete V1.4 Android acceptance.

## Desktop/Linux source boundary

Shared native C++ Glaze metadata now targets the same exact V1.4 authority as Android. `include/goreecloud/browser/glaze.hpp` exposes the V1.4 capability and trusted-chrome optical policy; `include/goreecloud/browser/glaze_acceptance.hpp` records production acceptance areas; and `apps/browser/glaze_contract_smoke.cpp` compiles these invariants through Core CI.

The CMake configuration also fails closed unless the expected V1.4 version is selected. This prevents Android and desktop/native design-system lifecycle versions from drifting silently again.

Desktop source acceptance is still not rendered/runtime acceptance. Real GTK/native rendering, keyboard/focus behavior, mixed input, localization, window/safe-area behavior, effects-free fallback, representative hardware, and production release evidence remain separate gates.

## First-party capability acceptance boundary

Browser contains a fail-closed generic first-party capability consumer boundary. A service capability is usable through that gate only when exactly one matching record is present and is versioned, current, authoritative, available, explicitly production-accepted, and version-compatible where required.

Duplicate, ambiguous, stale, unversioned, non-authoritative, degraded, wrong-version, or pre-Stable producer evidence is rejected.

This consumer boundary does not create producer authority. Search, Vault, Sync, Identity, Mesh, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, Bookmarks, and other first-party services retain independent contracts and lifecycle evidence.

## Wardveil download security

The Advanced Download Manager has a source-level Wardveil Scan release gate. Completed transfer bytes remain in Browser-owned staging until an authoritative, current, correctly scoped Wardveil `clean` finding is bound to the exact SHA-256 digest and revalidated before final release.

Suspicious downloads are held for review. Malicious or unverifiable downloads remain blocked. Browser does not execute canonical quarantine itself and does not connect directly to ClamAV. Private Browsing retains malware-verification requirements while private download queue state remains non-persistent.

This is source integration evidence, not a deployed malware-protection claim. Production acceptance still requires authenticated Browser-to-Wardveil transport, scanner/signature health, controlled runtime malware tests, safe staging release, authorized quarantine evidence, and accepted privacy/security behavior. Android downloads remain fail-closed until its own path can satisfy the applicable release contract.

See [`docs/WARDVEIL_DOWNLOAD_SCANNING.md`](docs/WARDVEIL_DOWNLOAD_SCANNING.md).

## Privacy and private browsing

Private Browsing must remain distinguishable by explicit persistence/behavior semantics, not by unsupported visual claims. Privacy Shield remains authoritative for privacy behavior.

A webpage cannot alter trusted Browser optics to make ordinary browsing appear private, private browsing appear more protected, or any session appear to possess privacy/security guarantees not backed by the applicable producer evidence.

Approved durable normal-profile state may participate in Everkeep recovery only through explicit scope and acceptance. Private and Isolated Private state remains excluded from ordinary persistence/backup unless a separately approved contract says otherwise.

## Design principles

1. GoreeCloud owns the Browser product layer.
2. The rendering engine is replaceable infrastructure, not product identity.
3. GoreeCloud-specific behavior should stay behind explicit application/service/engine boundaries rather than being scattered through upstream engine code.
4. Privacy Shield, Wardveil Security, Everkeep, GLAZE UI, GoreeCloud Mesh, and GoreeCloud Identity remain substantive independent authorities.
5. Browser state should use GoreeCloud-owned schemas and services instead of treating Chromium profile formats as permanent canonical product state.
6. Security updates from the underlying engine must remain practical to consume.
7. Engine-specific exceptions must be documented and isolated.
8. Planned capabilities must not be presented as production-ready without implementation and acceptance evidence.
9. Security, privacy, credential, identity, DNS, networking, search, synchronization, and design-system authority boundaries remain explicit.
10. Browser-owned user-facing surfaces must conform to the latest approved Stable GLAZE UI release before production acceptance.
11. Page content must never become implicit trusted-chrome authority.

## V1.4.1 human-validation boundary

Shared Glaze UI human/manual/physical-device/subjective optical validation is assigned to **V1.4.1** and is not represented as passed V1.4.0 evidence.

Browser-specific release acceptance may remain stricter. Applicable follow-up includes subjective optical-quality review, TalkBack/assistive-technology review, representative physical-device/platform behavior, spoof-resistance review, and real-device performance/power/thermal qualification.

Historical visual/device evidence against earlier Glaze versions remains historical evidence only.

## Remaining production blockers

Browser is not Stable until applicable evidence exists for:

- complete V1.4 rendered and accessibility acceptance across Android/Linux and supported layouts;
- trusted-chrome/page-content spoof-resistance review;
- Reduced Motion, Reduced Transparency, Increased Contrast, Forced Colors/platform equivalents, 200% text/reflow, RTL/localization, Touch Assistance where supported, and mixed-input behavior;
- representative Android phone/tablet/foldable and Linux/window-system behavior;
- production Privacy Shield, Wardveil Security, Everkeep, Identity, Mesh, Manager, Search, DNS/Network, and Vault integration where applicable;
- authenticated and runtime-tested download-security release flow;
- clean recovery/restore and rollback evidence;
- desktop real-runtime Chromium/CEF acceptance;
- Android production signing, update, rollback, migration, and release provenance;
- production health/readiness integration where required; and
- release approval and Stable qualification.

## Repository documentation

- [`SPECIFICATIONS.md`](SPECIFICATIONS.md) — product/source specification and authority boundaries.
- [`FEATURES.md`](FEATURES.md) — functionality and implementation state.
- [`BENEFITS.md`](BENEFITS.md) — supportable product value.
- [`COMPETITIVE-OBJECTIVES.md`](COMPETITIVE-OBJECTIVES.md) — product-quality objectives.
- [`USER-MANUAL.md`](USER-MANUAL.md) — current beta user guidance.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — architecture and ownership boundaries.
- [`docs/ENGINE_LAYER.md`](docs/ENGINE_LAYER.md) — Browser Engine Layer contracts.
- [`docs/ANDROID_BETA.md`](docs/ANDROID_BETA.md) — Android beta package/runtime/acceptance details.
- [`docs/GLAZE_UI_ADOPTION.md`](docs/GLAZE_UI_ADOPTION.md) — exact current Glaze migration and acceptance boundary.
- [`docs/GLAZE_UI_BROWSER_SURFACES.md`](docs/GLAZE_UI_BROWSER_SURFACES.md) — V1.4 Browser-owned surface model.
- [`docs/WARDVEIL_DOWNLOAD_SCANNING.md`](docs/WARDVEIL_DOWNLOAD_SCANNING.md) — download-security release contract.
- [`docs/PRODUCT_INVENTORY.md`](docs/PRODUCT_INVENTORY.md) — broader planned product inventory.

## Repository layout

```text
apps/browser/                 Native desktop application shell and smoke tests
apps/android/                 Installable Android beta application target
browser/core/                 Engine-independent browser domain model
browser/engine/               Browser Engine Layer interfaces
browser/engine/chromium/      Chromium adapter implementation
browser/services/             GoreeCloud service contracts and adapters
browser/storage/              GoreeCloud-owned browser data stores
browser/ui/                   Browser-owned presentation work
include/goreecloud/browser/   Shared/native C++ interfaces and contracts
src/                          Implementation and platform hosts
docs/                         Architecture and engineering documentation
packaging/                    Linux, Flatpak, Windows, and other packaging work
```

## License

GPL-3.0. See `LICENSE`.

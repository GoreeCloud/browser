# GoreeCloud Browser — Implemented Features

**Record type:** Repository implemented-feature inventory  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Authority:** Current `main` source and accepted repository evidence  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## Interpretation

This record describes capabilities present in the current GoreeCloud Browser Development source. It does **not** establish production deployment, accepted Browser-wide GLAZE UI conformance, representative-device acceptance, Release Candidate, or Stable qualification.

`FEATURES.md` remains the product-facing feature description. This file is the lifecycle authority for what is implemented.

## Implemented Development capabilities

### Browser core and authority boundaries

- GoreeCloud-owned engine-independent browser core and replaceable Browser Engine Layer.
- Linux GTK/X11 native beta shell build path.
- Linux Development tab chrome with multi-tab presentation plus create, activate, explicit close, next-tab, and previous-tab controls.
- Keyboard-first Linux Browser controls for location focus, new/close/cycle tab, reload, back/forward, Home, panel dismissal, and F6 address focus.
- First-party Linux New Tab/Home search field and wired Bookmarks, Downloads, and Settings quick actions.
- Structured Browser-owned Development panels for Bookmarks, Reader Mode, Wardveil Security, Privacy Shield, Clipboard, DNS, Proxy, Search failure, and Advanced Download Manager state instead of the earlier generic “Browser tool” placeholder.
- First-party Settings architecture surface with explicit General, Appearance, Search, Privacy, Security, Downloads, Network, Permissions, and Accessibility group presentation without falsely claiming unavailable runtime controls.
- Friendly Browser-owned internal-page title/location presentation that suppresses implementation-only `goreecloud://` locations from ordinary visible chrome.
- Canonical Browser compass branding synchronized from `GoreeCloud/branding-assets` full-color and monochrome identity sources.
- GLAZE UI V1.6 / 1.6.0 source mapping with exact accepted release/qualification anchors, Android-native V1.6 mapping, and a Browser-owned Linux desktop Development presentation tranche: branded chrome, compact active-tab surface, primary navigation capsule, bounded secondary tools popover, responsive first-party cards, friendly internal-page titles, and redesigned first-party internal surfaces.
- GoreeCloud Search integration as the sole integrated query authority while direct structurally valid HTTP(S) navigation remains independent from search.
- Fail-closed first-party capability evidence that does not manufacture Search, Vault, Sync, Identity, Mesh, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, or Bookmarks authority.
- Browser-owned Sync submission/retrieval contracts with bounded validation, pagination, privacy-safe tombstones, and signer-shape checks.
- Advanced Download Manager core with Wardveil-oriented staged-transfer/release-gate architecture; production downloads remain blocked until accepted Wardveil verification exists.
- Privacy-safe local session-recovery checkpoint/candidate core excluding Private and Isolated Private windows before persistence.
- Privacy-safe local liveness/readiness contract separating process health from release readiness.

### Android beta foundation

- Installable Development APK target with Android API 26 minimum and API 35 target.
- Android System WebView/Chromium rendering dependency behind GoreeCloud-owned product policy and behavior.
- Back, Forward, Reload, Go, unified address/search field, progress state, and web-content region.
- Direct HTTP/HTTPS navigation, HTTPS upgrade for bare hosts, and GoreeCloud Search for non-URL input.
- Browser-intent handling for HTTP/HTTPS links.
- TLS errors fail closed; Android Safe Browsing enabled with return-to-safety behavior.
- Mixed content disabled, third-party cookies disabled by default, and WebView file/content access disabled.
- Website permissions/geolocation denied until Browser-owned policy surfaces and authority adapters are accepted.
- Downloads remain fail-closed until the Wardveil release contract is satisfied.
- CI covers Browser-owned navigation behavior, Android lint/build, package/signature verification, SHA-256, artifact provenance, and an Android 15 managed-emulator smoke lane for Browser-owned chrome, conservative WebView defaults, and fail-closed local Search behavior.

### PermissionBroker Development foundation

- Platform-neutral website-permission request/profile/privacy-context/tab/origin binding.
- Typed camera, microphone, geolocation, protected-media, and MIDI SysEx resources.
- Duplicate/invalid request rejection, request expiry, deterministic lifecycle, and final context revalidation.
- Independent host-OS state handling and fail-closed Policy, Privacy Shield, and Wardveil authority gates.
- Explicit user allow/deny scopes without automatic engine grant before all gates pass.
- Persistent permission decisions prohibited in Private and Isolated Private contexts.
- Request/context-close cancellation and core smoke coverage.

### Native extension Development foundation

- Native manifest/API v1 models and `.gcex` package inventory validation.
- Bounded GCEX package-byte reader and strict manifest decoding.
- Closed native extension-permission vocabulary and fail-closed authorization model.
- Permission ledger covering one-shot, one-hour, tab/session, website, browser-session, profile binding, expiry, and revocation semantics.
- Bounded durable permission-ledger snapshot/recovery for approved lifetimes with corruption/clock/profile checks.
- Bounded package-local Ed25519 signature-verification foundation; a valid package signature can establish only package-signature state, not developer identity or production trust.

## Implemented-but-not-accepted boundaries

The following foundations exist but remain acceptance-gated and therefore also appear in `PLANNED-FEATURES.md`:

- Android Browser runtime without representative-device/accessibility/performance/production acceptance.
- Current repository source mapping targets GLAZE UI V1.6 / 1.6.0, but Browser-specific rendered/accessibility/adaptive/device/performance/rollback/Human Visual Excellence acceptance remains incomplete.
- Session recovery without authenticated encrypted persistence, lifecycle wiring, restore execution, Everkeep integration, and runtime acceptance.
- PermissionBroker without Android prompt/OS/live-authority adapters and durable Normal-context decision storage.
- Native extension package/signature/permission foundations without installation, execution, sandboxing, privileged APIs, trusted developer-key authority, or production acceptance.

## Maintenance rule

When an obligation in `PLANNED-FEATURES.md` becomes implemented and verified on the authoritative integration line, reconcile it here and record the material change in `CHANGELOGS.md`. Draft or unmerged pull requests are not implementation authority.
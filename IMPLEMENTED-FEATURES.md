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
- Internationalized HTTP(S) DNS hosts are canonicalized to ASCII A-label identity before direct Android navigation and unfocused omnibox presentation; malformed STD3 labels, invalid ports, bracketed non-IPv6 hosts, and ambiguous numeric dotted forms fail closed, while canonical four-octet decimal IPv4 remains accepted.
- Browser-intent handling for HTTP/HTTPS links.
- TLS errors fail closed; Android Safe Browsing enabled with return-to-safety behavior.
- Mixed content disabled, third-party cookies disabled by default, and WebView file/content access disabled.
- Website permissions/geolocation denied until Browser-owned policy surfaces and authority adapters are accepted.
- Downloads remain fail-closed until the Wardveil release contract is satisfied.
- CI covers Browser-owned navigation behavior, Android lint/build, package/signature verification, SHA-256, and artifact provenance.

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
- Current repository source mapping to GLAZE UI V1.5.1; current shared Stable authority is V1.6, so Browser requires a fresh V1.6 migration/acceptance tranche.
- Session recovery without authenticated encrypted persistence, lifecycle wiring, restore execution, Everkeep integration, and runtime acceptance.
- PermissionBroker without Android prompt/OS/live-authority adapters and durable Normal-context decision storage.
- Native extension package/signature/permission foundations without installation, execution, sandboxing, privileged APIs, trusted developer-key authority, or production acceptance.

## Maintenance rule

When an obligation in `PLANNED-FEATURES.md` becomes implemented and verified on the authoritative integration line, reconcile it here and record the material change in `CHANGELOGS.md`. Draft or unmerged pull requests are not implementation authority.
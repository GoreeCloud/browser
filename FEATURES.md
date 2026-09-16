# GoreeCloud Browser Features

This file records Browser functionality and implementation state. A listed feature is not a production-readiness claim unless its acceptance state explicitly supports that conclusion.

## Implemented and build-tested foundations

- GoreeCloud-owned engine-independent browser core.
- Browser Engine Layer abstraction for replaceable rendering/runtime foundations.
- Linux GTK/X11 native beta shell build path.
- GoreeCloud Search integration as the sole integrated query authority.
- Transport-neutral first-party service capability evidence that keeps producer authority outside Browser and fails closed unless an exact capability is current, authoritative, explicitly production-accepted, and—when requested—on the exact expected contract version.
- Browser-owned Sync submission/retrieval contracts with privacy-safe tombstones, capability/schema validation, pagination, record-ID bounds, and signer-shape validation.
- Advanced Download Manager core with staged transfer and source-level Wardveil release-gate architecture.
- Privacy-safe native session-recovery checkpoint/candidate core that excludes Private and Isolated Private windows before persistence, validates bounded persisted window/tab topology and active-tab references, and only considers the newest checkpoint so older crash state cannot silently resurface after a newer clean or private-only session.
- Media Hover architecture and first-party Browser feature contracts.

The first-party capability gate is a consumer-side contract only. A healthy transport, a recognized service, or current authoritative producer evidence is insufficient by itself: Browser will not treat a capability as usable through this gate until the producer evidence explicitly states production acceptance. Browser does not create or strengthen Search, Vault, Sync, Identity, Mesh, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, or Bookmarks authority.

The session-recovery core is not yet a user-ready recovery implementation. Durable authenticated-encrypted checkpoint storage, protected platform key integration, Browser lifecycle wiring, restore execution, Glaze recovery UI, Everkeep continuity integration, and runtime acceptance remain pending.

## Native extension Development source foundation

Exact implementation source `10cfb5baad6081382c02ea5067d115a7f43b2185` adds the first Browser-owned native-extension foundation:

- Manifest/API version 1 constants and GoreeCloud-native manifest data structures.
- `.gcex` package-inventory validation with fail-closed package-path, manifest-entry, duplicate-entry, and entry-point checks.
- A closed native extension-permission vocabulary with human-readable permission labels.
- Fail-closed permission authorization for declared capability, exact website scope, profile binding, private-browsing opt-in, and explicit grant scope/lifetime classes.
- `ExtensionPermissionLedger` state for one-shot consumption, exact one-hour timestamp expiry, tab/session-bound grants, website-scope closure, explicit lease revocation, and profile-wide revocation.
- CTest smoke coverage for malformed/undeclared permission and package cases plus temporary-permission lifecycle behavior.

On exact source `10cfb5baad6081382c02ea5067d115a7f43b2185`, Platform Contract #54 and Android Beta APK #232 completed successfully. GoreeCloud Browser Core CI #473 remains queued, so this section is Development source evidence and is not represented as full exact-head Core CI acceptance.

This native extension foundation does not parse or install real `.gcex` archives, verify package signatures, execute extension code, provide a sandbox/process runtime, durably persist/recover permission leases, implement wildcard/site-pattern matching, expose privileged Extension APIs, provide extension networking/storage/UI surfaces, or establish production extension behavior. External extension compatibility layers and a centralized GoreeCloud extension store are not part of this foundation.

## Android beta — implemented

- Installable debug-signed APK target.
- Package `io.goreecloud.browser.beta`.
- Current migration-candidate identity `0.1.0-beta.1+android.5` / versionCode `10005`.
- Android API 26 minimum and API 35 target.
- Android System WebView/Chromium rendering dependency behind GoreeCloud-owned product behavior.
- Back, Forward, Reload, Go, unified address/search field, progress state, and web-content region.
- Direct HTTP/HTTPS navigation.
- HTTPS upgrade for bare hosts.
- GoreeCloud Search for non-URL input.
- Browser-intent handling for HTTP/HTTPS links.
- TLS certificate errors fail closed.
- Android Safe Browsing enabled with return-to-safety behavior.
- Mixed-content loading disabled.
- Third-party cookies disabled by default.
- WebView file/content access disabled.
- Website permissions and geolocation denied until Browser-owned policy surfaces are accepted.
- Downloads blocked until the Android path satisfies the authoritative Wardveil release contract.
- Unit tests for Browser-owned navigation resolution.
- CI unit test, Android lint, APK build, signature/package verification, SHA-256, and artifact upload.

## Android beta — Glaze UI 2.2 source mapping candidate

- Current Stable Glaze UI version `2.2.0` and Stable release revision `6731098b28dd0393faa878c70d989a221d714a20` recorded in source.
- Accepted Glaze UI visual source `0411b0f6dd877aea30e2c5674e1acde0105fd97b` recorded as central design-system provenance, not Browser acceptance.
- Canvas/Surface/Soft Glaze/Glaze semantic material mapping preserved from the successful 2.x Browser shell.
- Browser-owned chrome explicitly remains in Application scope rather than claiming Universal Search, Control Center, System Panel, or Critical System authority.
- Ordinary System Glaze budget represented as at most one dominant Glaze panel and up to three small floating Glaze controls.
- Native Android controls retained for platform semantics.
- 48dp minimum general touch-oriented interaction target.
- 56dp Touch Assistance/far-view target floor recorded where applicable; complete Android runtime mapping remains pending.
- Light/dark appearance adaptation.
- Visible pressed/focus state mapping.
- Glaze UI 2.2 semantic state-priority contract with disabled state above error, pressed, focus, selected, hover, and rest.
- Calm expression and Balanced clarity for the current browser shell.
- Effects-free fallback that does not require blur/transparency.
- Contract tests for current Stable provenance, 48dp/56dp target floors, Application authority, System Glaze budget, non-claim of Universal Search/Control Center, and state priority.

This section is source-level migration evidence only. A successful build does not automatically establish Browser-specific Glaze UI 2.2 visual, native-device, accessibility, Human Visual Excellence, performance, or production acceptance.

## Planned / incomplete Android capabilities

- Browser-owned tab strip/tab switcher and multi-tab lifecycle.
- Private Browsing and Close & Forget runtime isolation.
- Browser-owned website permission prompts.
- Wardveil-authenticated download staging, scan, release, hold, and quarantine handoff.
- Full Privacy Shield filtering, consent, data-use, and diagnostics integration.
- Everkeep backup/recovery/portability integration.
- GoreeCloud Identity profile/session integration.
- GoreeCloud Vault credential/passkey/autofill integration.
- GoreeCloud Sync runtime integration.
- GoreeCloud DNS and GoreeCloud Network runtime adapters.
- GoreeCloud Mesh capability coordination.
- Complete Touch Assistance / far-view Android runtime mapping and native acceptance where supported.
- Bookmarks, history, library, settings, downloads UI, Reader Mode, and Wayfinder mobile surfaces.
- Controlled beta/production signing, managed updates, rollback, and migration.
- Store packaging and publication.
- Representative real-device, accessibility, performance, battery, and compatibility acceptance.

## Acceptance principle

Implemented source, successful CI, an installable package, runtime integration, target-environment validation, security validation, accessibility acceptance, design-system acceptance, and production approval are separate states. Browser documentation must preserve those distinctions.

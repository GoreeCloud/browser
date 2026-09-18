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

## Browser permission-broker Development foundation

The current Development branch adds a platform-neutral, Browser-owned permission-decision foundation for the Section 16 permission contract:

- typed camera, microphone, geolocation, protected-media, and MIDI SysEx resources;
- request/profile/privacy-context/tab-owner/origin/time binding;
- Normal, Private, and Isolated Private context separation;
- independent Privacy Shield, Wardveil, and host-OS decision inputs;
- per-resource least-privilege results rather than all-or-nothing broad grants;
- fail-closed handling for unavailable/invalid authority, OS denial/restriction/error, malformed requests, duplicate resources, expiry, owner destruction, privacy-context destruction, and origin change;
- explicit user decision scopes for once, session, and persistent outcomes;
- persistent decisions prohibited in Private and Isolated Private contexts;
- no implicit engine grant merely because the host OS permission exists;
- CTest smoke coverage for authority loss, OS blocks, private persistence, mixed-resource requests, expiry, owner/origin invalidation, malformed requests, and pending user decisions.

This is a source-level broker foundation only. Android WebView permission/geolocation callbacks remain denied and are **not** wired to grant through this broker. No production permission persistence, user prompt, OS runtime-permission request flow, live Privacy Shield/Wardveil adapter, diagnostics store, Close & Forget integration, representative-device acceptance, or production authorization is claimed.

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
- Current migration-candidate identity `0.1.0-beta.1+android.6` / versionCode `10006`.
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

## Android beta — current Glaze UI V1.5.1 source mapping

- Current Official Stable Glaze UI target `1.5.1`.
- Stable promotion revision `98da57064ede0f334627b632bc16801f580331af`.
- Reviewed V1.5 implementation anchor `ee1032a0822ab8e103f8afe48e5c1859fde65cc9`.
- V1.5.1 qualification source anchor `5b59d0e36950d737dba35b58ae58058684e0831b`.
- Immediate Stable rollback baseline `1.5.0`; inherited optical baseline `1.4.1`.
- Browser-owned Canvas/Surface/Soft Glaze material mapping retained.
- Browser chrome remains Application scope and does not claim system-shell authority.
- Capability absence/conflicting ownership fails closed.
- Presentation does not infer authorization, permission, provider precedence, privacy/security truth, or automatic consequential/fallback execution.
- 48dp general and 56dp Touch Assistance target floors remain represented in source.
- Accessibility precedence, effects-free fallback, vector Browser chrome, and current interaction-state rules remain covered by regression tests.
- Android artifact candidate is `0.1.0-beta.1+android.6` / versionCode `10006`.
- Android CI now checks out/verifies the exact PR head and records `SOURCE_REVISION` plus SHA-256 artifact evidence.

This is Development source/build-contract evidence only. Browser-local rendered/native visual, accessibility, large-text, localization/RTL, representative-device/posture, sustained performance, workflow, rollback, production, and Stable acceptance remain separate gates.

## Planned / incomplete Android capabilities

- Browser-owned tab strip/tab switcher and multi-tab lifecycle.
- Private Browsing and Close & Forget runtime isolation.
- Browser-owned website permission prompts and Android adapter wiring from the new fail-closed PermissionBroker foundation.
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

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
- Privacy-safe Browser-owned Normal-session recovery core with versioned journal/checkpoint persistence, deterministic replay, clean-vs-abnormal lifecycle classification, bounded validated tab/window projections, private-before-persistence exclusion, retired-ID tombstones, and Android Development recovery-resume integration through the C++ authority.
- Media Hover architecture and first-party Browser feature contracts.

The first-party capability gate is a consumer-side contract only. A healthy transport, a recognized service, or current authoritative producer evidence is insufficient by itself: Browser will not treat a capability as usable through this gate until the producer evidence explicitly states production acceptance. Browser does not create or strengthen Search, Vault, Sync, Identity, Mesh, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, or Bookmarks authority.

The session-recovery core and Android adapter are Development functionality, not production recovery acceptance. Android can reconstruct accepted Normal tab identity/order/selection and safe navigation/title projections after an abnormal app-process termination, but it creates fresh WebViews and does not recover raw engine memory, form contents, credentials, Private/Isolated Private state, or session-local Page-control choices. Released-version migrations, authenticated adversarial-storage protection where required, desktop restore execution, Everkeep continuity, representative physical-device/OEM/WebView fault evidence, and production acceptance remain pending.

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
- Current migration-candidate identity `0.1.0-beta.1+android.12` / versionCode `10012`.
- Android API 26 minimum and API 35 target.
- Android System WebView/Chromium rendering dependency behind GoreeCloud-owned product behavior.
- Back, Forward, Reload, Go, unified address/search field, progress state, and web-content region.
- Browser-owned regular-tab strip with independent live WebViews, stable logical tab IDs, vector new/close controls, selected-tab-only visible engine binding, and background callback isolation. Same-process Activity recreation uses process-token-bound Android saved state; after abnormal app-process termination, Android Development can instead consume the validated C++ Normal-session projection, rebuild the logical regular-tab graph before WebViews, and resume the active safe URL/title projection with fresh WebViews.
- Direct HTTP/HTTPS navigation.
- Direct-navigation safety rejects credential-bearing URLs, unsupported schemes, control-character injection, zero/out-of-range ports, and ambiguous/invalid numeric IPv4 host forms.
- Internationalized HTTP(S) DNS hosts are canonicalized to lowercase ASCII A-label identity before Android navigation and unfocused address presentation; malformed STD3 labels and bracketed non-IPv6 authorities fail closed.
- HTTPS upgrade for bare hosts.
- GoreeCloud Search intent classification for non-URL input; remote Search remains fail-closed until accepted Privacy Shield authorization and compatible Search capability evidence are available.
- Browser-intent handling for HTTP/HTTPS links.
- TLS certificate errors fail closed.
- Android Safe Browsing enabled with return-to-safety behavior.
- Mixed-content loading disabled.
- Third-party cookies disabled by default.
- WebView file/content access disabled.
- Browser-owned Site information and Privacy & security status sheets for truthful origin/transport and local protection-state visibility.
- Copy/Share page-address actions are available only for Browser-approved HTTP(S) pages; local Browser/resource addresses remain internal.
- Local Find in page with live match counting, previous/next navigation, and no remote query delegation.
- Page text-size controls from 75% to 200% in 25-point steps, persisted as an application-local Browser preference without changing Android system font scale.
- Session-local Desktop site mode that derives a desktop-style user agent from the active WebView engine version, enables wide-viewport presentation, and can return to the original mobile user agent without pinning Browser to a stale Chromium version.
- Confirmed Clear browsing data control for Android WebView cookies/sign-in state, website storage, cache, form data, navigation history, and SSL preferences while preserving Browser preferences and Android app permissions.
- Compact Browser-owned Page controls sheet groups Text size, Desktop site, session-local JavaScript enable/disable, and session-local automatic image loading enable/disable without turning those choices into durable profile or Sync policy.
- Resumable three-step first-use setup explains Browser/engine ownership, current privacy/security defaults, live regular-tab controls, Search boundaries, and Page controls. Fresh setup is required, voluntary replay is dismissible, optional contextual tips are device-local, and all setup/tip state stays outside account/Sync authority.
- Website permissions and geolocation denied until Browser-owned policy surfaces are accepted.
- Downloads blocked until the Android path satisfies the authoritative Wardveil release contract.
- Unit tests for Browser-owned navigation resolution.
- CI unit test, Android lint, APK build, signature/package verification, SHA-256, and artifact upload.

## Android beta — current Glaze V1.7 source mapping

- Current Official/Anchor Glaze target `1.7.0`; its bounded Stable runtime inherits accepted V1.6.0 behavior.
- V1.7 release integration revision `1a5756daed2294155be2e9972b24f580f6222b7b`.
- V1.7 Stable-contract qualification source `7c4ded83d7a8725165bb6a55dfb175667cc9589e`.
- Inherited accepted V1.6.0 runtime source `a7180679ea851389e0f3004515f9a25f420e716d`; historical V1.6 qualification evidence integration `354f5759385c28596fcfec26a3ad525e89fb1c35` remains provenance for that inherited runtime.
- Immediate known-good rollback baseline `1.6.0`; inherited optical/runtime foundations remain versioned historical provenance.
- Browser-owned Canvas/Surface/Soft Glaze material mapping retained.
- Browser chrome remains Application scope and does not claim system-shell authority.
- Capability absence/conflicting ownership fails closed.
- Presentation does not infer authorization, permission, provider precedence, privacy/security truth, or automatic consequential/fallback execution.
- 48dp general and 56dp Touch Assistance target floors remain represented in source.
- Accessibility precedence, effects-free fallback, vector Browser chrome, and current interaction-state rules remain covered by regression tests.
- Android artifact candidate is `0.1.0-beta.1+android.12` / versionCode `10012`.
- Android CI now checks out/verifies the exact PR head and records `SOURCE_REVISION` plus SHA-256 artifact evidence.

This is Development source/build-contract evidence only. Browser-local rendered/native visual, accessibility, large-text, localization/RTL, representative-device/posture, sustained performance, workflow, rollback, production, and Stable acceptance remain separate gates.

## PermissionBroker Phase 1 Development source

The current Development branch adds the first platform-neutral website-permission control plane:

- canonical request/profile/privacy-context/tab/origin binding;
- typed camera, microphone, geolocation, protected-media, and MIDI SysEx resources;
- duplicate/invalid request rejection and request expiry;
- deterministic request lifecycle and final context revalidation;
- independent per-resource host-OS state handling;
- explicit GoreeCloud Policy, Privacy Shield, and Wardveil fail-closed authority gates;
- explicit user allow/deny scopes with no automatic engine grant before all gates pass;
- persistent permission decisions prohibited in Private and Isolated Private contexts;
- request cancellation and context-close cancellation;
- C++ smoke coverage integrated into the normal core test suite.

This does not enable Android website permissions. Android callbacks remain fail-closed/direct-deny until the host-OS adapter, live authority adapters, Browser prompt UX, persistent Normal permission store, revocation/reset controls, private-context cleanup integration, and representative-device acceptance are completed.

## Local health/readiness Development contract

Browser now has a privacy-safe local operational contract that keeps liveness separate from readiness and models bounded dependency condition/freshness without accepting browsing content, URLs, queries, account/profile names, or credentials.

- Browser process/event-loop liveness is independent from release readiness.
- Core readiness requires the rendering engine, Browser-owned local state, and Browser UI.
- Required dependency unavailable/unknown/stale state blocks readiness.
- Optional dependency degradation lowers readiness without falsely declaring the entire Browser unavailable.
- Duplicate dependency signals fail closed.
- Enumerated reason codes keep diagnostics bounded and content-free.
- No network endpoint, Manager publication adapter, Observability exporter, alert route, or production telemetry is created by this source contract.

See `docs/HEALTH_READINESS_CONTRACT.md`. Manager/Observability runtime integration and production acceptance remain open.

## Planned / incomplete Android capabilities

- Complete multi-tab/session lifecycle beyond the implemented Android live regular-tab strip, same-process Activity recreation, and Development Normal-session abnormal process-death recovery, including released-version migration, Private/Isolated Private semantics, profiles/Webspaces, memory-pressure discard/recreation, large-scale tab UX, OEM/WebView fault coverage, and representative-device acceptance.
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

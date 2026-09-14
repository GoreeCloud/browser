# GoreeCloud Browser Specifications

## Product

GoreeCloud Browser is an original GoreeCloud-owned native web browser. GoreeCloud owns the product experience, application architecture, browser chrome, state models, navigation/search behavior, privacy/security integrations, synchronization behavior, and service boundaries. Mature rendering engines may be used as narrowly scoped technical foundations.

## Current lifecycle

- Browser channel: `0.1.0-beta.1`
- Production approved: No
- Stable: No
- Current mandatory Glaze UI target: `1.4.0`
- Glaze UI Stable promotion revision: `84cb3db4884042f0fa25ed6d475a127fb110f596`
- Current Stable hardening revision reviewed for Browser migration: `ee057ce9e729296aeaeda182d01db89f52bd66f3`
- Android Glaze UI V1.4 source mapping: Development migration candidate on `+android.5`; rendered/native-device application acceptance pending
- Platform Contract: `0.3`, using the seven authoritative Integral Platform Systems
- Canonical source repository: `GoreeCloud/goreecloud-browser`

The seven Integral Platform Systems are GoreeCloud Manager, Privacy Shield, Wardveil Security, Everkeep, Glaze UI, GoreeCloud Mesh, and GoreeCloud Identity. GoreeCloud Search, GoreeCloud Index, GoreeCloud Sync, Vault, DNS, Network, Bookmarks, and other first-party products/services remain separate capability or authority relationships and must not be represented as additional Integral Platform Systems.

## Android beta

- User-facing identity: GoreeCloud Browser Beta
- Debug-beta package: `io.goreecloud.browser.beta`
- Debug-beta version on the current Development line: `0.1.0-beta.1+android.5`
- Android versionCode: `10005`
- Minimum Android: API 26
- Compile/target API: 35
- Java/Kotlin target: 17
- Rendering runtime: Android System WebView/Chromium as an engine dependency
- Beta signing: Android debug signing for fresh-install testing
- Production signing material: not stored in source control and not yet accepted

The Android beta must remain installable without implying production readiness. CI validates unit tests, Android lint, APK assembly, APK signature/package identity, checksum generation, and artifact upload.

## Native session recovery contract

The engine-independent native core includes a source-level session-recovery checkpoint/candidate contract for normal Browser windows.

The current contract:

- models normal, Private, and Isolated Private window privacy modes but persists only normal-window state;
- deliberately exposes no policy switch that can enable private-window checkpoint persistence;
- requires every accepted checkpoint to have a non-empty checkpoint identifier and nonzero capture timestamp;
- sanitizes checkpoint windows before handing state to a persistence implementation;
- distinguishes running, clean-shutdown, and unclean-shutdown checkpoint states;
- treats a newest running or unclean normal-window checkpoint as a recovery candidate;
- never searches past the newest checkpoint for an older crash candidate, preventing stale crash state from silently resurfacing after a newer clean or private-only checkpoint;
- masks a newest checkpoint containing only private state into no recoverable candidate;
- supports explicit checkpoint discard through the abstract recovery-store boundary.

This is a tested source contract only. The repository does not yet claim durable authenticated-encrypted recovery persistence on the current mainline, platform-backed recovery-key protection, complete application lifecycle checkpoint wiring, restore execution, user-facing Glaze recovery UI, Everkeep recovery acceptance, or production recovery behavior.

## First-party service capability consumer contract

Authoritative Browser main contains a fail-closed first-party capability consumer boundary integrated through PR #16 as `974786cdccceac7a0198881d4bd5f4e5d4b28c58` from exact validated source `0de830edfdc21fe77de33bf5ec00986510ce4dc2`.

A capability is usable through this generic Browser gate only when the service is available and exactly one matching capability record is present, versioned, current, authoritative, explicitly production-accepted, and—when requested—an exact contract-version match. Duplicate or ambiguous evidence, unversioned records, stale or non-authoritative evidence, wrong capability/version, degraded services, and pre-Stable producer evidence fail closed.

The consumer boundary does not manufacture producer authority, automatically wire every first-party service adapter, establish a live production integration, or qualify Browser as Stable.

## Android navigation and Search contract

The unified address/search field resolves input through Browser-owned classification before remote action:

1. Empty input resolves to the local GoreeCloud Search Home surface.
2. Accepted HTTP/HTTPS URLs or supported bare-host input resolve to Browser navigation.
3. Non-URL Internet/current-information text becomes local GoreeCloud Search intent only.
4. Unsupported, malformed, ambiguous, or credential-bearing input fails closed locally rather than being silently executed or converted into a remote Search query.

Search classification retains normalized query text as local intent and does not manufacture a query-bearing `?q=` URL. The current Android Development runtime does not transmit Search queries remotely.

Production Browser Search delegation requires one unambiguous `search.query` capability record from `/api/v1/status` / `capability_evidence`, POST + `json_body`, JSON request/response media types, request/result bounds, required Privacy Shield capability-reference transport, required server enforcement, and authenticated requester evidence. Current capability validation requires requester authority `goreecloud-identity`, authentication scheme `bearer`, and requester-authentication header `Authorization`.

Browser creates and validates an operation-scoped Privacy Shield authorization request/decision boundary with request-ID correlation and requires a canonical opaque `psc_*` capability reference. The transport-neutral Search contract validates compatible evidence and can construct the bounded Search POST description, but it performs no network I/O and does not create a GoreeCloud Identity session, bearer credential, or accepted authenticated transport.

GoreeCloud Search remains the sole approved Internet/current-information query authority for Browser search entry points. GoreeCloud Index remains the universal/local federated discovery authority. Browser must not merge Search and Index into an ambiguous authority boundary or silently fall back to a third-party search engine.

## Android mobile chrome contract

The installed beta uses a Browser-owned two-region mobile shell:

- a 56dp top omnibox inside an 8dp top/bottom chrome gutter;
- a 56dp persistent bottom navigation toolbar;
- full-width web content between those regions;
- Browser-owned vector controls for Back, Forward, Search Home, Reload/Stop, and Browser menu;
- page-load progress overlaid on web content rather than allocated its own chrome row;
- unfocused address presentation that removes the scheme while keeping the hostname at the leading edge;
- complete URL exposure and selection when the omnibox receives focus;
- scroll-aware top chrome that collapses after meaningful downward page scrolling and returns on upward scrolling, page-top return, omnibox focus, new navigation, or accessibility-state requirements;
- Browser-owned Glaze menu presentation rather than the Android platform-default popup menu.

Expanded fixed Browser chrome is 128dp before Android system bars. The collapsed scrolling state retains only the 56dp bottom navigation toolbar.

## Android Glaze UI contract

The current Browser Development line targets **GLAZE UI V1.4 / `1.4.0`**. The canonical Stable promotion revision is `84cb3db4884042f0fa25ed6d475a127fb110f596`; Browser migration review also records current Stable hardening revision `ee057ce9e729296aeaeda182d01db89f52bd66f3`.

Historical Browser mappings to Glaze UI 2.0, 2.2, V1.2, and V1.3 remain historical implementation/rollback evidence only. They do not satisfy the current V1.4 consumer requirement and must not be relabeled as current acceptance.

Browser-owned Android presentation must preserve the V1.4 rules applicable to this product:

- neutral Frost/glass remains a bounded material foundation rather than semantic authority;
- Browser chrome is Application scope and must not claim Universal Search, Control Center, System Panel, Critical System UI, or other platform authority;
- ordinary interactive controls retain at least the normal 48dp floor, with the 56dp assisted target available where the applicable accessibility/input mode requires it;
- readable and critical-decision content remains solid where appropriate;
- Reduced Transparency and Forced Colors fail to accessible solid presentation;
- Increased Contrast suppresses decorative optical treatment where required and strengthens clarity;
- Reduced Motion must not be bypassed by Browser animation behavior;
- environmental tinting, warmth, chromatic depth, and color-memory influence remain decorative and bounded;
- optical behavior must not be driven by typed query content, navigation URLs, browsing history, credentials, cookies, account identity, protected platform evidence, telemetry, analytics, camera access, or remote environmental sensing;
- semantic labels, focus, pressed, selected, disabled, loading, warning, and error states remain understandable without relying on optical effects;
- native/effects-free operation must remain possible without blur/transparency.

Glaze UI may present Browser state but cannot manufacture Privacy Shield authorization, Wardveil security state, Everkeep continuity, GoreeCloud Identity authority, Search/Index authority, Mesh coordination, Sync state, or successful Browser workflow state.

This source mapping is not equivalent to Browser application acceptance. Production acceptance requires exact-revision rendered/native visual review, TalkBack/screen-reader and broader accessibility evidence, 200% text or supported equivalent, RTL/localization, Reduced Motion, Reduced Transparency, Increased Contrast, Forced Colors or platform equivalent, input/form-factor behavior, representative device coverage, performance/sustained-use evidence, and applicable V1.4.1 human/manual/physical-device qualification.

## Security boundary

Wardveil Security is the authoritative GoreeCloud security system. Android Browser must not manufacture Wardveil status.

Current Android beta security behavior includes TLS fail-closed handling, Android Safe Browsing, mixed-content blocking, disabled WebView file/content access, default-denied site permissions, and a blocked download path until Android can satisfy the Browser-to-Wardveil release contract.

Android System WebView remains responsible for its engine/platform security mechanisms; Wardveil remains responsible for GoreeCloud security evaluation, protection, evidence, and response where integrated.

## Privacy boundary

Privacy Shield is the authoritative privacy and data-use governance system. The beta currently uses privacy-protective defaults including third-party-cookie blocking and denied permission/geolocation requests.

The native session-recovery core excludes Private and Isolated Private windows before persistence. Search delegation additionally requires an operation-scoped accepted Privacy Shield decision and canonical capability reference before any production remote query path may proceed.

These are source-level privacy invariants, not a claim that complete private-browsing, Search authorization transport, or broader Browser Privacy Shield runtime acceptance has reached production.

## Continuity boundary

Everkeep is the authoritative resilience, recovery, preservation, portability, and continuity system. The native session-recovery core establishes a Browser-owned checkpoint/candidate boundary that future Everkeep integration can consume, but durable recovery storage, lifecycle integration, restoration, backup, portability, and migration acceptance remain pending.

## Identity and integration boundaries

GoreeCloud Identity is authoritative for identity, authentication, authorization, requester/service identity, accounts, devices, credentials, and sessions. The Search capability boundary may require Identity-backed requester authentication metadata, but capability validation alone does not create an authenticated Browser session or transport credential.

GoreeCloud Mesh is authoritative for platform coordination and capability discovery. GoreeCloud Search is the sole approved Internet/current-information query authority for Browser search entry points. GoreeCloud Index is the local/universal federated discovery authority. GoreeCloud Vault, Sync, DNS, Network, Bookmarks, Wayfinder, and other GoreeCloud products/services remain separate capability authorities according to their contracts.

GoreeCloud Sync remains separately governed from the seven Integral Platform Systems. Browser must define accepted Sync datasets before runtime synchronization and must exclude Private/Isolated Private state, credentials, passkeys, reusable secrets, unrelated protected evidence, and other prohibited state from ordinary synchronization.

## Production promotion blockers

At minimum, Stable Android promotion remains blocked by:

- controlled signing and update/rollback key operations;
- complete GLAZE UI V1.4 rendered/native-device visual/accessibility/form-factor acceptance on the exact Browser adoption revision;
- authenticated Wardveil download verification/release integration;
- accepted Privacy Shield runtime decision/capability acquisition and evidence handling;
- accepted GoreeCloud Identity requester authentication for protected first-party delegation;
- Search-side capability-reference verification and required-mode enforcement before remote Search transmission;
- accepted Everkeep recovery/continuity integration;
- private-browsing isolation and Close & Forget evidence;
- Browser-owned permission workflows;
- required Identity/Vault/Sync/DNS/Network/Mesh/Manager adapters;
- representative supported-device testing;
- accessibility, RTL/localization, large-text, reduced-effects, and input acceptance;
- upgrade/downgrade/data-migration acceptance;
- release provenance and operational recovery evidence.

A lower acceptance state must never be represented as a higher one.

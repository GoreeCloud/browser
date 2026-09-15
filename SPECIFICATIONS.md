---
title: "GoreeCloud Browser — Project Specification"
document_owner: "LaDamian Goree"
version: "v0.11"
document_status: "Under Review"
project_status: "Active Development / nonconformant"
classification: "Internal"
document_type: "Native Web Browser Project Specification and Implementation Blueprint"
project_name: "GoreeCloud Browser"
repository: "GoreeCloud/goreecloud-browser"
authoritative_record: true
canonical_path: "SPECIFICATIONS.md"
created: "2026-08-29"
last_updated: "2026-09-15"
supersedes: "Project Specification — Browser.docx v0.9 after verified Markdown migration"
---

# GoreeCloud Browser — Project Specification

> **Authority and migration note:** This Markdown file is the canonical Browser project specification after verified migration from the legacy Drive DOCX. It consolidates current requirements and architecture rather than repeating the legacy document's long chronological Development narrative. Historical checkpoints remain preserved in Git history, pull-request evidence, and the explicitly superseded legacy DOCX. The format migration itself does not change implementation state. Version `v0.11` retains the permission/private-context and Wardveil download runtime contracts in Sections 16–17 and adds the implementation-ready tabs, sessions, profiles, and Webspaces runtime contract in Section 18.

## Document Metadata

- **Document Owner:** LaDamian Goree
- **Version:** v0.11
- **Document Status:** Under Review
- **Project Status:** Active Development / nonconformant
- **Classification:** Internal
- **Document Type:** Native Web Browser Project Specification and Implementation Blueprint
- **Repository:** `GoreeCloud/goreecloud-browser`
- **Authoritative Record:** Yes
- **Primary Android Beta Package:** `io.goreecloud.browser.beta`
- **Android Beta Version:** `0.1.0-beta.1+android.5`
- **Android versionCode:** `10005`
- **Android Support Floor:** API 26
- **Android Target API:** 35
- **Current GLAZE UI Stable Authority:** V1.4.1 / `1.4.1`
- **Current Development Line:** PR #49, `chatgpt/search-index-browser-2026-09-13`
- **Latest independently validated implementation checkpoint:** `f62e650a0fdb7a85fb5c7cd997bed7750a3ba82f`
- **Validated checks for that exact implementation checkpoint:** Platform Contract #34, GoreeCloud Browser Core CI #451, Android Beta APK #212

The current documentation head may be newer than the validated implementation checkpoint. Documentation-only commits must not inherit implementation validation by implication.

## 1. Project Definition

GoreeCloud Browser is GoreeCloud's first-party native web browser. GoreeCloud owns the browser product identity, browser chrome, interaction model, navigation and search policy, browser state, privacy controls, security integration, synchronization behavior, sessions, profiles, downloads, preferences, and GoreeCloud service integrations.

A mature rendering engine may be used as a bounded implementation dependency where reimplementation would materially increase standards, compatibility, security-update, media, JavaScript, WebAssembly, or accessibility risk. The engine does not become the GoreeCloud product identity or policy authority.

## 2. Product and Platform Authority

GoreeCloud Browser must preserve independent authority boundaries:

- **GoreeCloud Search** is the sole approved Internet/current-information query authority for Browser search entry points. Direct structurally valid HTTP(S) navigation remains independent from Search.
- **GoreeCloud Index** remains the local/universal federated discovery authority and must not be merged with Search into an ambiguous authority boundary.
- **Privacy Shield** is authoritative for privacy authorization, consent, minimization, data governance, transparency, retention, and user control.
- **Wardveil Security** is authoritative for GoreeCloud security evaluation, verification, scanning, quarantine, and security response.
- **Everkeep** is authoritative for resilience, backup, recovery, preservation, portability, succession, and continuity obligations.
- **GLAZE UI** is authoritative for Browser-owned interface, interaction, accessibility, responsiveness, and design-system governance.
- **GoreeCloud Mesh** is authoritative for platform coordination and capability discovery without transferring subsystem authority.
- **GoreeCloud Identity** is authoritative for identity, authentication, authorization, accounts, devices, credentials, sessions, and delegated authority.
- **GoreeCloud Vault** remains the sole authority for protected credentials, passkeys, secrets, secure autofill material, payment data, and protected identity material.
- **GoreeCloud DNS** and **GoreeCloud Network** retain their applicable DNS, resolver, networking, and private-connectivity authority.
- **GoreeCloud Sync** is a separately governed synchronization platform rather than an additional Integral Platform System.

The Browser Platform Contract evaluates exactly seven Integral Platform Systems: GoreeCloud Manager, Privacy Shield, Wardveil Security, Everkeep, GLAZE UI, GoreeCloud Mesh, and GoreeCloud Identity. Search, Index, Sync, Vault, DNS, Network, Bookmarks, Webspaces, and other first-party products/services remain separate capability or authority relationships.

## 3. Architecture Direction

GoreeCloud Browser is not intended to be a permanent Chromium skin or deep fork for branding. Product-defining behavior belongs in GoreeCloud-owned application, service, storage, policy, and UI layers whenever a stable engine boundary is practical.

The Browser Engine Layer is replaceable. Android currently uses Android System WebView/Chromium as an engine dependency. Desktop may use CEF/Chromium or another mature engine through an explicit adapter boundary. Rendering engines must not manufacture GoreeCloud privacy, security, identity, synchronization, recovery, or search authority.

## 4. Current Android Development Target

The Android path under `apps/android` is an installable Development beta, not a production release.

Current verified source behavior includes:

- GoreeCloud Browser Beta launcher and HTTP/HTTPS browser intent handling.
- Browser-owned unified address/search chrome and bottom navigation controls.
- Browser-owned classification of input before remote action.
- Direct HTTP/HTTPS navigation and bare-host HTTPS upgrade.
- GoreeCloud Search as the sole intended remote Search authority, while the current Search path remains non-transmitting until authorization and requester-authentication gates are accepted.
- JavaScript and DOM storage for web compatibility.
- cleartext traffic disabled;
- mixed content blocked;
- Android Safe Browsing enabled with fail-closed return-to-safety behavior;
- TLS/certificate errors cancelled rather than bypassed;
- third-party cookies disabled by default;
- WebView file/content access disabled;
- website permission and geolocation callbacks denied by default;
- downloads blocked until the Wardveil release gate is integrated;
- current GLAZE UI V1.4.1 source mapping with `applicable-migration-required` status.

Current source-level defenses and contracts are Development evidence only. They do not establish rendered/native-device GLAZE acceptance, complete Privacy Shield or Wardveil integration, production signing, Release Candidate status, production approval, or Stable qualification.

## 5. Android Build and Evidence Contract

The canonical Android beta workflow is `.github/workflows/android-beta.yml`. It uses Java 17, Android API 35, Android Build Tools 35.0.0, and Gradle 8.9. It runs unit tests, Android lint, and debug APK assembly, then verifies APK signing, package identity, application label, SHA-256 generation, and artifact upload.

A passing workflow proves only the claims made by that workflow for the exact tested revision. It does not establish production security, privacy, performance, accessibility, compatibility, device qualification, or Stable status.

## 6. Signing Boundary

Android debug signing is used for Development installability. It is not production signing authority. Production keys must never be committed to source control.

Controlled beta/production signing, key custody, recovery, rotation, in-place upgrade, downgrade, rollback, data migration, release provenance, checksums, release notes, and controlled distribution remain independent promotion gates.

## 7. Security and Privacy Boundary

The Browser must fail closed when required authority is unavailable.

Current Android permissions/geolocation are denied directly. Current Android downloads are blocked. These defaults must remain intact until Browser-owned runtime brokers and the applicable first-party authority adapters return accepted decisions.

Privacy and security state must be truthful. Browser UI must not display an allow, safe, verified, private, authenticated, synchronized, recovered, or protected state unless the responsible authority and local state machine support that claim.

## 8. Current Restrictions

The current Browser Development line does not establish:

- production or Stable readiness;
- rendered/native-device GLAZE UI V1.4.1 acceptance;
- production GoreeCloud Identity, Vault, Sync, Everkeep, DNS, Network, or Mesh integration;
- complete private-browsing runtime isolation and Close & Forget acceptance;
- production Wardveil download verification, quarantine, or release;
- complete Privacy Shield filtering, consent, authorization, minimization, retention, or evidence surfaces;
- Browser-owned production website-permission UI and Android runtime-permission mapping;
- complete multi-tab, session, settings, profile, private-browsing, or Webspaces workflows;
- production signing, store packaging, update, rollback, downgrade, or migration acceptance;
- sustained representative-device, accessibility, performance, battery, and compatibility evidence.

## 9. Production and Stable Promotion Gates

Before Android Browser may be represented as production-approved or Stable, it must complete and verify at least:

1. Controlled production signing and key-recovery operations.
2. Exact-revision GLAZE UI V1.4.1 rendered/native visual, accessibility, localization/RTL, large-text, reduced-effects, adaptive/form-factor, performance, rollback, and Human Visual Excellence acceptance.
3. Authenticated Browser-to-Wardveil download scanning, disposition, quarantine, and release evidence.
4. Privacy Shield runtime authorization, consent, minimization, filtering, retention, and evidence integration where applicable.
5. Private and Isolated Private request-context/storage isolation with Close & Forget evidence.
6. Required Identity, Vault, Sync, Everkeep, DNS, Network, Mesh, and Manager adapters.
7. Browser-owned website permission prompts and Android runtime-permission mapping.
8. Real-device validation across supported Android versions, screen sizes, WebView versions, accessibility configurations, network transitions, process/background restoration, and sustained use.
9. Signed upgrade, downgrade, rollback, and application-data migration testing.
10. Release provenance, checksums, release notes, controlled distribution, operational recovery, Release Candidate qualification, and production acceptance evidence.

## 10. Current Development Status and Evidence Boundary

PR #49 remains open Development/beta work on `chatgpt/search-index-browser-2026-09-13`. The latest independently validated implementation checkpoint remains exact source `f62e650a0fdb7a85fb5c7cd997bed7750a3ba82f`, which passed Platform Contract #34, GoreeCloud Browser Core CI #451, and Android Beta APK #212.

The current Android package/version remains `io.goreecloud.browser.beta`, versionName `0.1.0-beta.1+android.5`, versionCode `10005`. Documentation changes do not promote that application version.

Historical V1.2, V1.3, V1.4.0, earlier PR, and superseded architectural checkpoints remain historical evidence in Git, pull-request history, and the retained superseded legacy specification. They must not be presented as current acceptance when the current exact source has not been independently verified for the same capability.

## 11. GoreeCloud Browser Sync Architecture

**Status:** Proposed / planned architecture. No accepted runtime Browser Sync implementation exists.

### 11.1 Architectural Model

GoreeCloud Browser Sync is the Browser-specific synchronization service built on the shared GoreeCloud Sync platform:

```text
GoreeCloud Browser → GoreeCloud Browser Sync → GoreeCloud Sync → GoreeCloud Platform Services
```

Browser Sync owns Browser data models, dataset registration, merge semantics, exclusions, category controls, user experience, and Browser-specific privacy/security policy. GoreeCloud Sync owns the shared synchronization substrate, transport, change propagation, trusted-device integration, retry/offline queues, and common synchronization infrastructure.

### 11.2 Synchronization Domains

Independently controllable Browser Sync domains may include bookmarks, open tabs, tab groups, windows, sessions, history, Reading List, search-engine configuration, compatible Browser settings, extension state, Webspace definitions, site permissions, download metadata, and compatible appearance/profile preferences.

Private and Isolated Private browsing state must never participate in ordinary synchronization. Executable extension packages should normally be reacquired from approved sources rather than transferred device-to-device. Downloaded file payloads belong to approved storage services, not ordinary Browser Sync.

### 11.3 Vault-Class Synchronization

Passwords, passkeys, authentication credentials, secure autofill identities, payment data, secure notes, and related secrets remain under GoreeCloud Vault authority. Browser Sync may coordinate Vault-backed synchronization only through explicit versioned Vault capabilities. Browser Sync must not become credential authority.

### 11.4 Cryptographic and Trust Domains

Browser Sync should separate standard Browser state, privacy-sensitive Browser state, and secure Vault-class state into independently authorized cryptographic domains. Sensitive synchronized data should support end-to-end encryption before leaving a trusted device. Key handling should integrate with GoreeCloud Identity and platform-backed key facilities where available.

### 11.5 Trusted Devices and Per-Device Policy

Users should be able to approve, revoke, inspect, pause, and rename participating devices, and independently control categories per device. Newly trusted devices must not silently receive sensitive categories without policy authorization.

### 11.6 Versioned Dataset and Object Contract

Each dataset must declare a stable dataset identifier, schema version, compatibility range, stable object identity, logical version/operation metadata, deletion/tombstone semantics, retention policy, authorization class, and migration behavior.

The data plane must support durable local change journaling, idempotent incremental push/pull, acknowledgements, bounded batching, retry, compaction, offline resume, duplicate suppression, and deterministic convergence.

### 11.7 Merge, Deletion, and Stale-Device Semantics

Merge behavior must be domain-specific rather than a generic last-write-wins rule. Bookmarks, history, tabs, sessions, settings, Webspaces, and permissions have different conflict and deletion semantics. Tombstones or equivalent deletion records must prevent stale devices from resurrecting deleted state. Close & Forget exclusions must not reappear from Sync.

### 11.8 Privacy Shield and Authority Boundaries

Every synchronized category must preserve explicit purpose, minimization, destination, retention, and disclosure controls. Browser Sync must preserve independent Identity, Privacy Shield, Vault, Wardveil, Everkeep, Mesh, DNS, Network, and Browser authority rather than inferring one authority from another.

### 11.9 Offline-First and Transport Selection

Local changes are recorded immediately and synchronized when connectivity returns. The shared Sync layer may choose among trusted local connectivity, GoreeCloud Mesh, peer-to-peer encrypted transport, self-hosted infrastructure, and GoreeCloud-hosted infrastructure without changing the Browser data model.

### 11.10 Hosted, Self-Hosted, and Hybrid Operation

Browser Sync should support GoreeCloud-hosted, self-hosted, and hybrid deployments. Storage location, retention, transport, peer-to-peer use, metered-network behavior, bandwidth, and battery controls remain user/operator controlled within applicable policy.

### 11.11 Recovery, Diagnostics, and Acceptance

Recovery must preserve the authorization boundary of the domain being restored. Diagnostics must be privacy-safe and must not expose secrets, raw private history, or reusable authorization material.

Production acceptance requires dataset/schema migration tests, conflict/deletion tests, stale-device return, account/profile/device isolation, cryptographic-domain evidence, revocation, hosted/self-hosted/hybrid transport, offline/retry behavior, import/export, resource scheduling, representative-device testing, and privacy/security review.

## 12. Native GoreeCloud Vault Integration

**Status:** Proposed / planned. No accepted native Vault Browser adapter or production credential UX exists.

GoreeCloud Browser should expose Vault as a first-party Browser capability while preserving GoreeCloud Vault as the sole credential authority.

Planned capabilities include username/password autofill, inline suggestions, password/passphrase generation, save/update prompts, passkeys/WebAuthn, TOTP where permitted, payment/identity autofill, Vault quick access, lock/reauthentication, origin-aware matching, and explicit user controls.

Browser must not persist reusable Vault secrets in ordinary Browser storage, logs, diagnostics, Sync datasets, session recovery, or private-context state. Private and Isolated Private contexts must not silently convert Vault access into durable Browser history or credential persistence.

Vault operations require explicit versioned capabilities, authenticated requester identity where required, origin binding, user-presence/reauthentication policy where required, fail-closed handling, and representative credential-flow security testing.

## 13. Website Permissions and Privacy Contexts

**Status:** Proposed / incomplete. Current Android runtime denies website permission and geolocation requests directly.

Browser owns the permission workflow and must mediate engine requests, host OS runtime permissions, site decisions, privacy context, Privacy Shield policy, and applicable Wardveil policy without allowing any one layer to bypass the others.

Supported privacy contexts are:

- **Normal** — eligible for explicitly allowed persistent site decisions.
- **Private** — separate ephemeral state; no persistent site grants.
- **Isolated Private** — stricter ephemeral partition with no persistent grants and no ordinary cross-context reuse.

Permission decisions must be origin-bound, profile-bound, context-bound, type-specific, revocable, observable in Browser controls, and fail closed when authority is unavailable or request state becomes stale.

Close & Forget must cancel unresolved private requests and destroy private permission/session state that is not explicitly required by a higher retention authority.

## 14. Secure Downloads and Wardveil Verification

**Status:** Proposed / incomplete. Current Android runtime intentionally blocks downloads.

Downloads must use quarantine-first staging. A remote object must not be exposed as a normal released user file until transfer completion, digest finalization, accepted authenticated Wardveil verification, exact object/verdict correlation, and a safe release decision.

The Browser owns request normalization, transfer/staging state, destination workflow, release transaction, private-context semantics, and user experience. Wardveil owns the security verdict. A rendering engine callback must not write directly to the final user destination.

`UNAVAILABLE`, authentication failure, capability mismatch, timeout, invalid response, stale verdict, or digest mismatch must fail closed rather than becoming an implicit allow.

Release must be TOCTOU-resistant and atomic to the extent supported by the host platform. Post-release File Manager, Drive, MediaStore, or external-application handoff is allowed only for the released object.

## 15. Tabs, Sessions, Profiles, and Webspaces Runtime Architecture

**Status:** Proposed / partial. Complete runtime workflows are not accepted.

Browser runtime state should be local-first. Tabs belong to windows and privacy contexts; windows belong to profiles; Webspaces provide higher-level organization/isolation policy without becoming identity or security authority.

Normal session state may participate in durable recovery and accepted synchronization. Private and Isolated Private state must be excluded from ordinary durable recovery and Sync. Process-death restoration must distinguish recoverable normal state from clean shutdown, private-only state, or stale checkpoints.

Profiles must isolate cookies, storage, permissions, history, settings, Sync namespace, Vault delegation context, and recovery state as required by the profile model. Webspace isolation rules must not weaken profile/privacy boundaries.

Session recovery and Everkeep integration remain separate: Browser owns the current runtime/session model; Everkeep owns broader resilience and recovery authority.

## 16. Permission and Privacy-Context Runtime Implementation Contract

**Status:** Proposed implementation contract. Current Android callbacks continue to deny permission/geolocation requests directly; this section does not claim an accepted broker runtime.

### 16.1 Runtime Ownership

All engine permission callbacks must enter a Browser-owned `PermissionBroker`. The engine adapter may translate requests and apply the broker's final decision, but it may not independently grant access, persist decisions, or infer authority.

The broker coordinates Browser site policy, privacy context, host OS runtime permission state, Privacy Shield authorization where required, applicable Wardveil policy, user choice, and final request revalidation.

### 16.2 Canonical Privacy Context Type

The runtime model must expose an explicit privacy context enum equivalent to:

```text
NORMAL
PRIVATE
ISOLATED_PRIVATE
```

Every permission request must bind to exactly one profile and one privacy context for its entire lifetime. Context changes invalidate the request rather than silently rebinding it.

### 16.3 Permission Request Model

A normalized request must contain, at minimum:

- unique `request_id`;
- `profile_id`;
- privacy context;
- tab/window identity or equivalent transient owner;
- canonical top-level origin;
- canonical requesting origin;
- normalized typed permission resources;
- opaque engine request handle that is never persisted as authority;
- user-gesture state where available and relevant;
- creation and expiry timestamps;
- current site-decision snapshot;
- host OS permission snapshot;
- required Privacy Shield/Wardveil policy flags and correlation references where applicable;
- schema/contract version.

Unknown or unsupported resource types must fail closed.

### 16.4 Normalized Permission Types

The initial Android model should explicitly distinguish at least camera/video capture, microphone/audio capture, geolocation, protected-media access when supported, and MIDI system-exclusive access when supported. Multiple requested resources must be evaluated independently and must not be collapsed into a broader grant.

### 16.5 Decision Model

The broker must distinguish at least:

```text
ALLOW_ONCE
ALLOW_SESSION
ALLOW_PERSISTENT
DENY_ONCE
DENY_SESSION
DENY_PERSISTENT
CANCELLED
EXPIRED
BLOCKED_POLICY
BLOCKED_SECURITY
BLOCKED_OS
UNAVAILABLE
ERROR_FAIL_CLOSED
```

`ALLOW_PERSISTENT` is prohibited for Private and Isolated Private contexts. A host OS grant does not equal a website grant, and a website/site decision does not override an OS denial or controlling GoreeCloud policy.

### 16.6 Deterministic State Machine

The request lifecycle must be equivalent to:

```text
RECEIVED
  → VALIDATING
  → CONTEXT_BOUND
  → POLICY_EVALUATION
  → OS_CAPABILITY_CHECK
  → USER_DECISION
  → RESOLVING_ENGINE
  → COMPLETED
```

Terminal paths include denied, blocked, cancelled, expired, unavailable, and fail-closed error outcomes.

Before `RESOLVING_ENGINE`, Browser must revalidate the tab/request owner, profile, privacy context, top-level/requesting origins, request expiry, OS permission state, and required policy decisions. Navigation, origin change, profile switch, context destruction, request replacement, or Close & Forget invalidates stale permission requests.

### 16.7 PermissionBroker Interface

The platform-neutral contract should expose operations equivalent to:

- `receive(request)`;
- `evaluate(request_id)`;
- `apply_user_decision(request_id, decision)`;
- `cancel(request_id, reason)`;
- `revoke(profile_id, origin, permission_type)`;
- `close_context(context_id)`.

The Android adapter should translate `PermissionRequest` and geolocation callbacks into normalized requests, then apply only the broker's final accepted decision.

### 16.8 Host OS Permission Adapter

The host adapter must distinguish granted, denied-but-requestable, denied/no-reprompt, restricted by device/OS policy, unavailable, and adapter error. Browser must explain when an OS-level permission blocks a site decision without misrepresenting the OS decision as a Privacy Shield or Wardveil decision.

### 16.9 Privacy Shield and Wardveil Adapters

Where a permission requires Privacy Shield authorization, Browser must request an operation-scoped decision/capability and correlate the response to the exact normalized request. Browser must not manufacture or locally infer Privacy Shield authorization.

Where Wardveil provides a relevant security block or risk decision, Browser may enforce that result, but Browser must not manufacture Wardveil status. Only minimum purpose-limited facts may cross adapter boundaries.

### 16.10 Persistence, Expiry, and Revocation

Normal persistent decisions require a versioned Browser-owned permission store keyed by profile, canonical origin, and permission type, with timestamps, source, expiry/retention semantics, and revocation state.

Private and Isolated Private decisions are ephemeral and must not enter the persistent Normal permission store, Sync, ordinary recovery, or ordinary analytics. Close & Forget destroys applicable private permission state and cancels outstanding private requests.

### 16.11 Permission UX

The user-facing prompt must clearly identify requesting site/origin, requested capability, active profile/privacy context, meaningful consequences, and decision scope such as once/session/persistent when allowed. Browser settings must expose per-site review/reset/revocation controls.

GLAZE UI may present Browser and authority state but may not invent policy or security decisions. Permission UI requires accessibility, keyboard/input, localization/RTL, large-text, reduced-effects, screen-reader, and representative-device acceptance.

### 16.12 Privacy-Safe Diagnostics

Diagnostics may record opaque request identifiers, normalized permission type, high-level lifecycle state, bounded failure category, adapter/capability version, and timing needed for reliability analysis. They must not log reusable credentials, raw private browsing history, unnecessary full URLs, media contents, user-entered form data, or protected capability material.

### 16.13 Permission Acceptance Matrix

Production acceptance requires automated, fault-injection, and representative-device evidence for at least:

1. Missing/unavailable broker or authority fails closed.
2. Unknown resources are denied.
3. Multi-resource requests do not gain broader access than individually accepted resources.
4. Top-level or requesting-origin changes invalidate stale requests.
5. Navigation, tab destruction, profile switch, or privacy-context destruction invalidates stale requests.
6. Private and Isolated Private contexts cannot create persistent permission grants.
7. Close & Forget cancels pending private requests and destroys private permission state.
8. Normal persistent grants expire/revoke/reset correctly.
9. OS grant and site grant remain independent decisions.
10. OS denial cannot be bypassed by Browser/site policy.
11. Privacy Shield timeout, mismatch, invalid response, or unavailable state fails closed when authorization is required.
12. Applicable Wardveil security blocks cannot be converted into Browser allow decisions.
13. Race conditions between callbacks, navigation, and user decisions resolve deterministically.
14. Repeated callbacks/cancellation are idempotent.
15. Permission UX passes accessibility/localization/RTL/large-text/reduced-effects/device review.
16. Diagnostics reconstruct lifecycle failures without collecting protected payloads or reusable credentials.

Passing unit tests with mock adapters does not establish production acceptance; live authority, exact-revision, representative-device, and applicable privacy/security acceptance remain required.

## 17. Wardveil-Gated Download Runtime Implementation Contract

**Status:** Proposed implementation contract. Current Android runtime continues to block downloads; this section does not claim an accepted scanner/release runtime.

### 17.1 Runtime Ownership

The Browser owns request normalization, download lifecycle state, quarantine staging, transfer orchestration, destination selection, release transaction, privacy-context semantics, diagnostics, and post-release handoff. Wardveil owns the authoritative security verification/disposition. The rendering engine must not write directly to the final destination.

### 17.2 Download Record Model

Each attempt must contain, at minimum:

- stable `download_id`;
- unique `attempt_id`;
- profile and privacy-context identity;
- transient tab/request owner where applicable;
- canonical source origin and a bounded source URI representation only where operationally required;
- proposed and normalized filename;
- declared and observed MIME/type metadata when available;
- expected and actual byte length;
- quarantine staging object identifier;
- finalized cryptographic digest;
- current lifecycle state;
- Wardveil request/correlation identifier, capability version, and normalized verdict;
- final destination only after release authorization;
- schema version.

Reusable cookies, authorization headers, form secrets, Vault material, and unrelated browsing history must not be stored in the durable download record.

### 17.3 Required Interfaces

The platform-neutral pipeline should separate interfaces equivalent to:

- `DownloadRequestAdapter`;
- `DownloadStagingStore`;
- `DownloadTransferEngine`;
- `DownloadDigestVerifier`;
- `WardveilDownloadVerifier`;
- `DownloadReleaseManager`;
- `DownloadDestinationAdapter`;
- `DownloadRecoveryStore`.

This separation keeps engine, storage, scanner, destination, and release authority independently testable.

### 17.4 Download State Machine

The normal success path must be equivalent to:

```text
REQUESTED
  → VALIDATING_REQUEST
  → AWAITING_DESTINATION
  → STAGING_CREATED
  → TRANSFERRING
  → STAGED_COMPLETE
  → DIGEST_FINALIZED
  → AWAITING_WARDVEIL
  → VERIFYING
  → VERIFIED_SAFE
  → RELEASING
  → RELEASED
```

Terminal or non-release states include `BLOCKED`, `QUARANTINED`, `REVIEW_REQUIRED`, `CANCELLED`, `FAILED`, `UNAVAILABLE`, and `EXPIRED`.

There must be no direct transition from request/transfer completion to final release without an accepted object-bound Wardveil verdict.

### 17.5 Quarantine-First Staging

Staging must use Browser-controlled storage unavailable as an ordinary completed user file. Partial data must never be represented as released. Staged objects must be immutable once finalized for scanning. Abandoned staging data requires bounded cleanup/retention rules.

### 17.6 Filename, Type, and Destination Safety

Before staging/release, Browser must safely handle path traversal, reserved platform names, bidi/control characters that could mislead presentation, duplicate-name collisions, excessive filename length, content-disposition variants, and declared-versus-observed type mismatch where the accepted security pipeline exposes that fact.

A webpage must not silently choose an unrestricted filesystem path. Destination selection remains Browser-owned state until release.

### 17.7 Authenticated Wardveil Verification

Production Browser must use an explicit versioned Wardveil download-verification capability and independently authenticated requester/service identity when required by the accepted Wardveil contract.

A verification request must bind the verdict to the exact staged object using the Browser correlation identifier, `download_id`, `attempt_id`, immutable staging object/content handle, finalized digest, byte length, bounded filename/type metadata needed for security evaluation, minimum purpose-limited source context, and capability/schema version.

Wardveil responses must be authenticated and correlated to the original request. An uncorrelated, stale, malformed, unsupported-version, ambiguous, or unauthenticated response fails closed.

### 17.8 Verdict Model

Browser must normalize at least:

```text
SAFE
BLOCK
QUARANTINE
REVIEW_REQUIRED
UNAVAILABLE
ERROR
EXPIRED
INVALID_RESPONSE
```

Only an accepted `SAFE` verdict for the exact staged object may advance to `VERIFIED_SAFE`. Scanner-health failure, authentication failure, capability mismatch, timeout, unavailable service, or invalid response must never become an implicit allow.

Any future security override requires a separately governed explicit policy; it must not emerge from an error path.

### 17.9 TOCTOU-Resistant Release

Before release, the staging object remains immutable; Browser revalidates object identity; digest and byte length must match the accepted Wardveil verdict; the verdict must remain current; and destination authorization must still be valid.

Release should use an atomic rename/move where supported. Across storage boundaries, Browser must use a bounded copy-and-verify sequence that does not expose the destination as complete until the copied object is revalidated. Mutation or digest mismatch invalidates the verdict and returns the attempt to a non-released fail-closed state.

### 17.10 Private and Isolated Private Downloads

A user may intentionally retain a file downloaded from a private context, but Browser must not persist the private source URL, tab/session identifiers, or private history into ordinary Browser history, Sync, diagnostics, or recovery merely because the file was retained.

Closing a private context cancels unresolved downloads owned by that context unless a separately designed and accepted user-visible continuation feature explicitly says otherwise.

### 17.11 Restart, Retry, and Recovery

Recovery must never interpret staged or quarantined content as released. Resume is allowed only when the transfer protocol/server supports safe resumption. Any resumed or mutated object requires digest finalization and a current Wardveil verdict for that exact object. Expired/object-mismatched verdicts cannot be reused. Cancellation and cleanup must be idempotent.

Browser Sync must not synchronize staged/quarantined payloads as ordinary Browser data. Everkeep must not be assumed to preserve quarantined or staged payloads unless an explicit accepted continuity policy says so.

### 17.12 Post-Release Ecosystem Handoff

Only after `RELEASED` may Browser hand an object to GoreeCloud File Manager, GoreeCloud Drive, MediaStore, or a user-selected external application. Handoff must grant only minimum required file/URI access and must not imply that the receiving product inherited Browser, Wardveil, Privacy Shield, or Vault authority.

Opening a file is a distinct action from downloading it unless a separately accepted safe auto-open policy exists.

### 17.13 Diagnostics and Evidence

Production evidence must reconstruct which Browser attempt produced the released object, finalized digest/length, accepted Wardveil capability/version, normalized verdict, and verified transition from staged to released object without logging cookies, bearer credentials, private form content, Vault secrets, reusable Privacy Shield capability material, or raw private browsing history.

### 17.14 Download Acceptance Matrix

Production acceptance requires automated, fault-injection, and representative-device evidence for at least:

1. Missing/unavailable Wardveil adapter keeps downloads non-released.
2. Partial transfer cannot appear as a released file.
3. Completed staging cannot release before digest finalization.
4. Unauthenticated, uncorrelated, stale, malformed, unsupported-version, or ambiguous Wardveil response fails closed.
5. `SAFE` for object A cannot release object B.
6. Object mutation/digest mismatch after scanning invalidates the verdict.
7. Filename traversal, bidi/control spoofing, invalid/reserved names, duplicates, and overlong names are handled safely.
8. Cancellation, timeout, network loss, storage exhaustion, and process death preserve a non-released state.
9. Retry/resume cannot duplicate released files or reuse invalid verdicts.
10. Concurrent downloads isolate staging IDs, verdicts, destinations, and release operations.
11. Quarantine/block/review-required remain distinct from ordinary transfer failure.
12. Private-context release can preserve an intentional user file without persisting private source/session history.
13. Post-release handoff grants only minimum required file/URI access.
14. File Manager, Drive, MediaStore, or external-app handoff cannot occur from an unverified staging object.
15. Restart recovery never interprets staged/quarantined content as already released.
16. Browser Sync and Everkeep do not accidentally replicate staged/quarantined payloads as ordinary Browser state.
17. Download UX passes accessibility, localization/RTL, large-text, reduced-effects, representative-device, sustained-use, and error-recovery review.
18. Privacy-safe diagnostics can reconstruct release decisions without protected payloads or reusable credentials.

Passing this matrix with a mock verifier is not production Wardveil acceptance. Production qualification requires the accepted live Wardveil capability, authenticated requester path, exact implementation revision, representative-device evidence, and applicable security/privacy approval.

### 17.15 Implementation Sequence

1. Land platform-neutral permission and download models/state machines with deterministic unit tests.
2. Introduce `PermissionBroker`; keep engine callbacks fail closed until an accepted decision is available.
3. Add host OS permission mapping and context-safe Browser UI without persistent private grants.
4. Connect accepted Privacy Shield permission authorization and applicable Wardveil blocking adapters.
5. Introduce quarantine-first download staging and digest finalization while keeping release disabled.
6. Connect authenticated Wardveil verification for immutable staged objects.
7. Add TOCTOU-resistant release and post-release destination adapters.
8. Complete process-death, retry, cancellation, private-context, diagnostics, accessibility, and representative-device acceptance.
9. Enable production permission grants and downloads only after the exact candidate satisfies all applicable gates.

The implementation principle is: **no engine callback or file-release path may bypass Browser-owned state, privacy context, or required GoreeCloud authority.**

## 18. Tabs, Sessions, Profiles, and Webspaces Runtime Implementation Contract

**Status:** Proposed implementation contract. Section 15 defines the target architecture; this section defines the implementation-facing runtime model and acceptance requirements. It does not claim a complete multi-tab, session-recovery, profile, or Webspaces runtime.

### 18.1 Runtime Ownership

Browser owns the canonical profile, privacy-context, window, tab, session-journal, checkpoint, Webspace-assignment, restoration, resource-management, and user-facing state machines. The rendering engine owns page execution/rendering state behind a replaceable adapter and must not become the authority for Browser profile identity, durable session truth, privacy-context lifetime, Webspace policy, Sync eligibility, or recovery classification.

Browser Sync may synchronize explicitly eligible projections of Browser state after local state is committed. Everkeep may preserve approved recovery material through its own accepted contract. Neither Sync nor Everkeep replaces the Browser runtime state store or may resurrect state that Browser has classified as destroyed, private-only, or ineligible.

### 18.2 Canonical Identifiers and Object Model

Runtime objects must use stable Browser-owned identifiers with explicit schema versions. The model should include at least:

- `profile_id` — stable identity for one Browser profile;
- `privacy_context_id` plus context type `NORMAL`, `PRIVATE`, or `ISOLATED_PRIVATE`;
- `window_id` — stable Browser window/task identity within a profile;
- `tab_id` — stable tab identity independent from any transient engine instance;
- optional `webspace_id` — Browser organizational/isolation policy assignment;
- `navigation_id` or equivalent generation counter for the current committed navigation;
- `session_epoch` — identifies one Browser runtime session/recovery generation;
- `journal_sequence` — monotonically ordered local state-change position;
- `checkpoint_id` — immutable durable normal-session checkpoint identity;
- schema and migration version fields.

Engine object references, WebView instances, process IDs, Android activity IDs, view handles, or memory addresses are transient implementation handles and must never be used as durable Browser identity.

### 18.3 Profile Isolation Model

Each tab and window must belong to exactly one `profile_id`. A Browser profile owns or scopes the applicable cookie/storage partition, history namespace, permission store, settings, Webspaces, Sync namespace, Vault delegation context, session journal, recovery material, and other profile-local state.

Cross-profile object movement must be an explicit Browser operation that creates or imports eligible state into the destination profile. Reparenting a live engine instance must not silently transfer cookies, storage, permissions, credentials, private state, or authority across profiles.

Deleting a profile must enter a controlled deletion lifecycle that closes its runtime objects, revokes or detaches applicable integrations, destroys profile-local state according to policy, records only the minimum permitted deletion evidence, and prevents stale checkpoints or Sync input from recreating the deleted profile without an explicit user-authorized restore/import path.

### 18.4 Tab State Machine

A Browser-owned tab state machine should distinguish at least:

```text
CREATED
  → INITIALIZING
  → ACTIVE | BACKGROUND
  → FROZEN
  → DISCARDED
  → RESTORING
  → ACTIVE | BACKGROUND
  → CLOSING
  → CLOSED
```

Exceptional states may include `CRASHED`, `RESTORE_FAILED`, and `ERROR` when needed for truthful recovery UX.

`tab_id` survives engine recreation, ordinary freeze/discard, and accepted process-death restoration. Closing a tab invalidates outstanding permission requests, transient authority handles, pending navigations that no longer belong to a live owner, and other tab-scoped work that cannot safely outlive it.

A discarded tab may retain only the minimum eligible Browser-owned restoration projection. Engine memory state is not assumed durable unless an accepted engine adapter provides a versioned compatible serialization that Browser treats as optional implementation data rather than sole session truth.

### 18.5 Window and Session State Model

Windows/tasks must have Browser-owned ordered tab membership, active-tab identity, profile identity, privacy-context ownership rules, and optional Webspace presentation state. A window cannot contain tabs from multiple profiles unless a future separately accepted architecture explicitly defines such a model.

A runtime session should distinguish:

```text
STARTING
RUNNING
BACKGROUNDING
CHECKPOINTING
RESTORING
CLEAN_SHUTDOWN
ABNORMAL_TERMINATION
```

Clean shutdown and abnormal termination must be distinguishable. Browser must not offer crash/process-death restoration based solely on the existence of stale files. Restoration eligibility must be determined from the last durable journal/checkpoint state, shutdown marker, profile/context policy, schema compatibility, and integrity validation.

### 18.6 Durable Normal-Session Journal

Eligible Normal-context runtime changes should be recorded through a versioned append-oriented journal or equivalent transactional event store before or atomically with the durable state they authorize.

Journal entries should include only the minimum state required to reconstruct Browser runtime truth, such as object identity, operation type, ordering/version information, bounded navigation/restoration metadata, Webspace assignment, and lifecycle transition. The journal must not persist reusable credentials, Vault secrets, authorization headers, private form data, or engine memory dumps merely for convenience.

Journal processing must be idempotent. Duplicate replay, interrupted compaction, reordered durable writes, or process death during a state transition must not create duplicate tabs, cross-profile ownership, impossible active-tab references, or resurrection of explicitly closed state.

### 18.7 Checkpoints, Compaction, and Integrity

Browser should periodically materialize immutable or transactionally replaceable normal-session checkpoints so restoration does not require replaying an unbounded journal.

A checkpoint should declare at least:

- `checkpoint_id`;
- `profile_id` or a clearly bounded multi-profile container when independently justified;
- `session_epoch`;
- schema version;
- journal high-water mark;
- window/tab ordering and active-tab state;
- eligible bounded navigation/restoration projection;
- Webspace assignment and Browser-owned presentation state needed for restoration;
- integrity checksum/MAC where required by the local threat model;
- creation time and compatibility metadata.

Compaction must preserve the meaning of committed close/delete operations. A checkpoint created after a tab close must not retain that tab as restorable state unless a separate recently-closed feature intentionally and explicitly owns such history.

Corrupt, partially written, unsupported-version, or internally inconsistent checkpoints must fail safely and must not be silently repaired by inventing Browser state.

### 18.8 Process-Death and Crash Restoration

At startup after abnormal termination, Browser should:

1. identify the most recent compatible valid checkpoint;
2. replay only journal entries after its high-water mark;
3. validate profile, privacy-context, window, tab, and Webspace relationships;
4. discard or quarantine invalid/inconsistent records rather than binding them across authority boundaries;
5. construct Browser-owned tab/window objects before attaching engine instances;
6. restore eligible Normal-context tabs according to resource limits and user policy;
7. surface bounded recovery information when material state could not be restored.

Restoration must be deterministic for the same validated durable input. It must not depend on network availability, Sync success, or Everkeep availability for ordinary local process-death recovery.

### 18.9 Private and Isolated Private Session Semantics

Private and Isolated Private tabs/windows must not enter the ordinary durable Normal session journal or checkpoint store.

If temporary crash-survival state is ever introduced for a private context, it requires a separately approved design with explicit threat model, encryption/key lifetime, automatic expiry, Close & Forget destruction, and no ordinary Sync/Everkeep participation. Until such a design is accepted, private runtime state is memory/ephemeral-storage scoped and non-restorable after process death.

`Close & Forget` must atomically or idempotently initiate destruction of the private context, cancel unresolved context-owned permissions/downloads/operations that cannot safely continue, destroy context storage and ephemeral session metadata, remove private tab/window objects, and prevent stale journal, engine, Sync, or callback input from recreating them.

### 18.10 Webspaces Runtime Contract

A Webspace is a Browser-owned organization and isolation-policy object within a profile. It may define user-visible name/icon/color, tab grouping, site/domain assignment rules, cookie/storage container mapping, permission defaults or references, Browser policy attributes, and eligible synchronization metadata.

Webspaces must not become identity, credential, Privacy Shield, Wardveil, DNS, Network, or Vault authority. A Webspace policy may impose a stricter Browser-local boundary, but it may not weaken profile or privacy-context isolation or convert an unavailable external authority into an allow decision.

Each tab must have either no Webspace or one explicit `webspace_id` under its owning profile. Moving a tab between Webspaces must use a defined transition. If storage/container isolation differs, Browser must create a new compatible browsing context or reload/reinstantiate through the destination container rather than silently retaining incompatible origin state.

### 18.11 Profile Switching and Context Transitions

Switching the visible profile must not mutate ownership of existing windows/tabs. Browser should activate a destination profile surface and suspend or background the previous profile according to policy.

Normal → Private, Private → Normal, profile A → profile B, or Isolated Private → any other context are not ordinary mutable property changes on a live tab. When user intent requires opening the same URL in another context, Browser should create a new destination-context tab with a purpose-limited navigation handoff rather than transferring cookies, history, storage, permission grants, Vault handles, or private session state.

### 18.12 Engine Adapter and Instance Recreation

The engine adapter must expose Browser-owned lifecycle hooks for create, attach, navigate, suspend, freeze, discard, recreate, restore projection, and destroy operations. An engine crash or Android WebView/process recreation must be mapped into Browser tab/session state rather than redefining it.

Browser should restore the minimum safe navigation projection first. Any engine-specific serialized state is optional and must be schema/version checked, bounded in size, scoped to the correct profile/context/tab, and rejected when incompatible or unsafe.

### 18.13 Browser Sync Boundary

Browser Sync must consume committed Browser projections; it must not observe half-applied local transitions or become the transaction coordinator for local runtime state.

Sync-eligible state must be explicitly classified per object/domain. Private and Isolated Private runtime objects are ineligible. Remote close/delete/tombstone semantics must be reconciled without resurrecting stale local objects. Incoming state must never move a tab across profiles or privacy contexts without an explicit supported mapping and local authorization.

Open-tabs/session-continuity synchronization should exchange bounded logical state, not raw engine memory, cookies, reusable credentials, private storage, or opaque local authority handles.

### 18.14 Everkeep and Recovery Boundary

Ordinary Browser session restoration remains a Browser-owned local runtime responsibility. Everkeep may provide broader recovery/version-history/continuity only through an explicit accepted contract.

Everkeep restore must target a Browser-supported import/recovery boundary and must not directly overwrite live Browser runtime databases while the Browser is active. Restored state must be schema compatible, integrity checked, profile scoped, and classified as recovered material before becoming active runtime state.

Private/Isolated Private session material remains excluded unless a separately governed privacy design explicitly authorizes preservation.

### 18.15 Resource Management and Tab Discard

Resource management may freeze or discard inactive tabs based on bounded signals such as recency, foreground state, memory pressure, media/activity state, user pinning, form-loss risk, accessibility needs, and platform constraints.

Discard policy must be Browser-owned and observable enough to explain user-visible restoration. Tabs performing security-sensitive, user-confirmation, active-download-control, media capture, WebAuthn/passkey, or other stateful workflows should not be discarded blindly.

Resource optimization must not weaken privacy isolation, drop required cleanup, or transform an unpersisted private tab into durable restorable state.

### 18.16 User Experience and Accessibility

The tab switcher, window/session recovery UI, profile switcher, Webspaces controls, recently closed/recovery surfaces, and error states must clearly communicate active profile/privacy context and avoid ambiguous cross-context actions.

GLAZE UI requirements include keyboard/input navigation where applicable, TalkBack/screen-reader semantics, large-text/reflow support, localization/RTL, reduced effects, contrast, focus order, state announcements, and representative-device behavior.

Destructive actions such as Close & Forget, profile deletion, clear-session operations, or recovery replacement must communicate their scope without falsely promising deletion from independent authorities that Browser does not control.

### 18.17 Privacy-Safe Diagnostics

Diagnostics may record opaque profile/window/tab/checkpoint identifiers, schema versions, lifecycle transitions, bounded restore failure categories, journal/checkpoint integrity outcomes, engine recreation counts, and timing needed to evaluate reliability.

Diagnostics must not contain raw private URLs/history, cookies, authorization headers, Vault secrets, form contents, reusable Privacy Shield capability material, or full engine serialized state. Normal browsing URLs should be minimized or reduced to purpose-limited derived categories unless exact values are explicitly required and authorized for a controlled debugging workflow.

### 18.18 Runtime Acceptance Matrix

Production acceptance requires deterministic automated tests, fault injection, migration tests, and representative-device evidence for at least:

1. Stable `tab_id` survives engine recreation while transient engine identity changes.
2. Tabs and windows cannot cross `profile_id` ownership accidentally.
3. Private and Isolated Private objects never enter the ordinary Normal session journal/checkpoint store.
4. Clean shutdown does not trigger crash-recovery behavior.
5. Process death during a journal write restores the last committed consistent state.
6. Process death during checkpoint replacement leaves at least one valid recovery source.
7. Duplicate journal replay is idempotent and does not duplicate tabs/windows.
8. Closed tabs do not resurrect after compaction or stale Sync input.
9. Active-tab/window ordering remains internally consistent after replay.
10. Corrupt or unsupported checkpoints fail safely without cross-profile rebinding.
11. Close & Forget cancels/destroys applicable private state and prevents stale callback resurrection.
12. Profile deletion prevents stale local or synchronized state from silently recreating the profile.
13. Profile switching does not transfer cookies, storage, permissions, Vault handles, or private state.
14. Webspace moves preserve profile/privacy boundaries and recreate storage contexts when isolation changes require it.
15. Engine crash/recreation does not redefine Browser authority or session identity.
16. Tab discard/reload behavior is deterministic and does not lose protected in-progress workflows without policy.
17. Browser Sync receives only committed, explicitly eligible logical projections.
18. Incoming Sync state cannot resurrect Private/Isolated Private state or violate profile isolation.
19. Everkeep import/recovery cannot overwrite an active live runtime store outside the accepted Browser recovery boundary.
20. Schema upgrade/downgrade/rollback and migration tests preserve or safely reject session state as designed.
21. Multi-window/multi-tab operation remains correct under rapid open/close/move/reorder/background/process-death sequences.
22. Accessibility/localization/RTL/large-text/reduced-effects/representative-device review passes for tab, profile, Webspace, and recovery surfaces.
23. Privacy-safe diagnostics reconstruct restoration failures without protected payloads or reusable credentials.
24. Sustained-use and memory-pressure testing does not create impossible state, unbounded journals, or privacy-context leakage.

Passing model/unit tests alone does not establish production runtime acceptance. Production qualification requires the exact implementation revision, Android/desktop platform evidence for supported targets, storage/migration integrity evidence, privacy/security review where applicable, and representative-device sustained-use validation.

### 18.19 Implementation Sequence

1. Define platform-neutral profile/privacy-context/window/tab/Webspace identifiers and immutable schema contracts.
2. Implement the Browser-owned in-memory state graph and deterministic tab/window lifecycle tests without changing production persistence behavior.
3. Add the versioned Normal-session journal with idempotent replay and corruption tests.
4. Add checkpoint creation/compaction and process-death restoration behind a Development-only gate.
5. Add profile isolation, profile-switch, profile-delete, and context-transition tests.
6. Add Webspace assignment/container-transition logic without allowing Webspaces to weaken profile/privacy boundaries.
7. Integrate engine recreation, freeze/discard, and resource-management adapters.
8. Add explicit Sync projections only after local commit semantics are stable; keep Private/Isolated Private excluded.
9. Add the accepted Browser↔Everkeep recovery/import boundary without allowing direct live-store overwrite.
10. Complete migration, rollback, accessibility, representative-device, sustained-use, memory-pressure, and fault-injection acceptance before enabling production claims.

The implementation principle is: **Browser-owned logical runtime identity and privacy boundaries survive engine recreation, process death, synchronization, and recovery; transient engine objects never become the source of truth.**

## 19. Documentation, Roadmap, and Evidence Synchronization

`SPECIFICATIONS.md`, `FEATURE-ROADMAP.md`, the corresponding Markdown roadmap record in `GoreeCloud/Feature Roadmap/GoreeCloud Browser`, PR/repository evidence, and GoreeCloud Tasks Management must remain materially consistent without becoming duplicate authority for the same purpose.

Documentation changes may define planned architecture, implementation contracts, and acceptance criteria. They do not establish runtime implementation. When source code is added, implementation claims must be tied to exact revisions and actual validation evidence.

The superseded legacy Browser DOCX is retained only as a historical migration source unless a future retention instruction requires a different disposition.
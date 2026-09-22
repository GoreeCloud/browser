# GoreeCloud Browser — Planned Features and Open Obligations

**Record type:** Repository planned/incomplete-feature inventory  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Authority:** Current `main` source, accepted repository evidence, and active GoreeCloud Tasks Management obligations  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## Interpretation

Items here are planned, incomplete, blocked, or acceptance-gated. Their presence does not imply implementation or release readiness. Partial foundations that already exist are also described in `IMPLEMENTED-FEATURES.md` for the verified portion only.

## Current stabilization obligations

- Migrate Browser presentation from its current V1.5.1 source mapping to current Official Stable GLAZE UI V1.6 / 1.6.0, then complete Browser-specific rendered, accessibility, large-text, localization/RTL, adaptive/form-factor/posture, reduced-effects, performance, rollback, Human Visual Excellence, and representative-device acceptance.
- Complete issue #33 representative physical-device usability, accessibility, performance, launcher rendering, and practical browser behavior against an exact candidate.
- Complete Browser-owned multi-tab/session/profile state and process-death/OEM/WebView/network recovery acceptance.
- Complete website-permission/geolocation UX plus Android runtime-permission mapping, live GoreeCloud Policy/Privacy Shield/Wardveil authority adapters, durable Normal-context decision storage, revocation/reset, private-context cleanup, and representative-device acceptance.
- Complete authenticated Browser-to-Wardveil download staging, scanning, verdict, quarantine/release, provenance, TOCTOU protection, recovery, and safe-failure behavior before enabling production downloads.
- Complete Private Browsing and Isolated Private runtime isolation, Close & Forget, deletion/cleanup evidence, and representative lifecycle acceptance.
- Complete applicable runtime integration/evaluation for all nine Integral Platform Systems: Manager, Privacy Shield, Wardveil Security, Everkeep, GLAZE UI, Mesh, Identity, Policy, and Observability. GoreeCloud Sync remains separately governed.
- Complete authenticated Browser-to-Search delegation with accepted Identity requester authentication, real Privacy Shield decisions/capabilities, Search-side verification, replay/denial handling, and production acceptance.
- Complete Browser Sync runtime integration on the separately governed GoreeCloud Sync substrate while excluding Private/Isolated Private state and preserving independent authority boundaries.
- Complete native first-party GoreeCloud Vault credential/passkey/autofill integration while keeping Vault as the credential authority.
- Complete production signing/key custody/recovery/rotation, upgrade/downgrade/rollback/data migration, controlled distribution/store publication, release evidence, Release Candidate, production approval, and Stable qualification.

## Product capability work still required

- Browser-owned tab strip/tab switcher and multi-tab lifecycle.
- Bookmarks, history, library, settings, downloads UI, Reader Mode, and Wayfinder mobile surfaces.
- Full private-browsing product surfaces and Close & Forget behavior.
- Complete navigation/address safety for malformed URLs, schemes/deep links, user-info disclosure, IDN/confusable/spoofing, origin/certificate presentation, and bidirectional text.
- Mature session restoration and Browser-owned durable recovery.
- Complete native extension installation, sandbox/process runtime, trusted developer-key authority, extension networking/storage/UI, privileged APIs, updates, developer tooling, and production trust/acceptance.
- Complete desktop render-capable runtime acceptance and preserve rendering-engine replaceability.
- Canonical artwork/package provenance and representative launcher/device rendering acceptance.
- Sustained device validation across supported Android versions, screen sizes, WebView versions, accessibility services, locales, large text, reduced effects, network transitions, background/process restoration, battery, and compatibility.

## Explicit non-claims

Until corresponding evidence exists, this file does not claim current V1.6 Browser acceptance, production website permissions/downloads, production Search/Vault/Sync authority integration, production extension execution/trust, production signing/distribution, Release Candidate, Production Acceptance, or Stable status.

## Legacy Drive roadmap migration ledger — 2026-09-22

The retired Drive roadmap contained 43 identifier-level records. The full obligation, legacy priority, and last Drive-side state are preserved below so the Drive source can be retired without losing feature, state, dependency, limitation, or verification context. These snapshots are **historical migration evidence**, not current lifecycle authority.

### FR-001

- **Legacy obligation:** Reconcile and maintain every current planned or recommended GoreeCloud Browser feature from the authoritative project record and verified repository evidence in this roadmap.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Current-state roadmap control is synchronized to repository `SPECIFICATIONS.md` v0.11, Browser Sync Phase 0 v0.1, Native Extension Platform v0.2, GCEX Package Format v1, Extension Permission Ledger Storage v1, GCEX Signature Scheme v1, and the September 16 PR #49 Development line. GCEX reader/decoder is exact-source verified at `80c577e…`; permission-ledger persistence is exact-source verified at `2026b06…`; signature verification is exact-source verified at `756e368…`. Runtime/production acceptance remains separate.
- **Migration disposition:** Governance control migrated into the repository-native interpretation and maintenance rules; Drive synchronization is retired.

### FR-002

- **Legacy obligation:** Move actionable feature obligations into GoreeCloud Tasks Management when required, preserving priority, dependency, and lifecycle disposition.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Ongoing control. Browser remains covered by the active GoreeCloud portfolio improvement task; do not create duplicate Browser task authority.
- **Migration disposition:** Tasks Management routing control preserved; actionable execution stays in GoreeCloud Tasks Management when required without duplicate Browser task authority.

### FR-003

- **Legacy obligation:** Do not mark features implemented, complete, cancelled, or superseded without authoritative evidence and synchronized repository/Drive roadmap updates.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Ongoing control; Development source/build evidence remains distinct from production/Stable acceptance.
- **Migration disposition:** Evidence/lifecycle rule preserved; the former repository/Drive synchronization clause is superseded.

### FR-010

- **Legacy obligation:** Preserve GoreeCloud Search as the sole approved Internet/current-information query authority while keeping structurally valid direct HTTP(S) URL navigation independent from search.
- **Legacy priority:** High
- **Legacy Drive state (historical):** PR #49 formalizes transport-neutral Search intent, capability preflight, operation-scoped Privacy Shield authorization preparation, and no silent third-party fallback. Live Search transport remains disabled.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-011

- **Legacy obligation:** Complete Browser-owned navigation and address safety including malformed URL handling, scheme/deep-link handoff, user-info disclosure, IDN/confusable/spoofing policy, origin/certificate presentation, and bidirectional address behavior without manufacturing DNS/origin trust.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Current source rejects malformed, credential-bearing, and unsupported navigation shapes and retains earlier canonical-host hardening. Full confusable/spoofing, origin/certificate, bidi, and representative-device acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-012

- **Legacy obligation:** Complete Browser-owned main-frame failure/recovery and session restoration behavior across Android recreation, process death, OEM/WebView variants, network transitions, and representative failure modes while keeping raw remote error content out of Browser-owned recovery UI.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Development source recovery boundaries exist; process-death/OEM/physical-device acceptance remains open. `SPECIFICATIONS.md` v0.11 Section 18 defines the target Browser-owned restoration state model and acceptance requirements.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-013

- **Legacy obligation:** Complete GLAZE UI V1.4.1 / 1.4.1 Browser migration and rendered/native-device visual, adaptive/form-factor, accessibility, large-text, contrast, localization/RTL, reduced-effects, rollback, performance, and Human Visual Excellence acceptance.
- **Legacy priority:** High
- **Legacy Drive state (historical):** PR #49 targets current Stable GLAZE UI V1.4.1 and records migration-required status. Browser-local rendered/native/device/product acceptance remains open.
- **Migration disposition:** Legacy V1.4.1 target retained as historical context; current open Browser presentation obligation is Official Stable GLAZE UI V1.6 / 1.6.0 plus Browser-specific acceptance.

### FR-014

- **Legacy obligation:** Complete Browser-owned website permission and geolocation UX with explicit Android runtime-permission mapping and applicable Privacy Shield/Wardveil policy evidence; keep requests denied while authority is unavailable.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Project Specification v0.11 Sections 13 and 16 define the Browser-owned permission broker, normalized permission/request data model, Android runtime-permission adapter, decision state machine, persistence/expiry rules, Privacy Shield/Wardveil boundaries, and acceptance matrix. No accepted runtime implementation exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-015

- **Legacy obligation:** Integrate authenticated Browser-to-Wardveil download verification, scanning, release/quarantine decisions, provenance, and safe failure behavior before enabling production downloads.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Project Specification v0.11 Sections 14 and 17 define quarantine-first staging, immutable object/digest binding, authenticated Wardveil verification, explicit verdict states, TOCTOU protection, atomic release, private-context handling, recovery, and acceptance requirements. Current Android runtime remains fail-closed and blocks downloads.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-016

- **Legacy obligation:** Complete Privacy Shield policy, consent, minimization, destination/retention enforcement, filtering, disclosure, and evidence surfaces for Browser-owned data flows and user controls.
- **Legacy priority:** High
- **Legacy Drive state (historical):** PR #49 adds operation-scoped Search authorization contracts but no accepted runtime decision acquisition. Project Specification v0.11 Sections 13 and 16 and Browser Sync Phase 0 §§8–10, 16–17 further define Privacy Shield authorization/minimization boundaries; broader runtime acceptance remains open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-017

- **Legacy obligation:** Complete Private Browsing and Isolated Private runtime request-context/storage isolation, Close & Forget behavior, deletion/cleanup evidence, and representative lifecycle acceptance.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Project Specification v0.11 Sections 13, 16, and 18 plus Browser Sync Phase 0 §16 define Normal/Private/Isolated Private separation, ephemeral private handling, Close & Forget cancellation/cleanup, non-durable private session semantics, and ordinary-Sync exclusion. Runtime isolation and representative-device acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-018

- **Legacy obligation:** Complete GoreeCloud Identity, Vault, GoreeCloud Sync, Everkeep, DNS, Network, Mesh, and applicable Manager integrations under independent authority boundaries with accepted runtime evidence.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Platform/product integration remains incomplete. PR #49 explicitly separates the seven Integral Platform Systems from other first-party services and keeps Sync-specific obligations separate.
- **Migration disposition:** Legacy integration wording retained as provenance; current platform authority evaluates all nine Integral Platform Systems, with GoreeCloud Sync separately governed.

### FR-019

- **Legacy obligation:** Complete Android multi-tab, session, history/state, Settings, profiles/account behavior, private-browsing surfaces, and other release-scope Browser product workflows while preserving local-first/fail-closed policy boundaries.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Project Specification v0.11 Sections 15 and 18 define tabs/sessions/profiles/Webspaces architecture plus implementation-facing identifiers, state machines, journal/checkpoint recovery, profile isolation, engine recreation, resource management, Sync/Everkeep boundaries, and acceptance tests. Complete runtime workflows remain partial/planned.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-020

- **Legacy obligation:** Maintain explicit rendering-engine dependency boundaries so Android System WebView/Chromium, desktop CEF/Chromium, and future mature engines remain replaceable implementation dependencies rather than GoreeCloud product/policy authority.
- **Legacy priority:** Medium
- **Legacy Drive state (historical):** Active architectural control; Section 18 additionally requires transient engine identity to remain subordinate to Browser-owned logical tab/session identity. Desktop render-capable runtime acceptance remains incomplete.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-021

- **Legacy obligation:** Complete canonical Browser artwork/package provenance and representative launcher/device rendering acceptance without treating packaged-resource presence as visual acceptance.
- **Legacy priority:** Medium
- **Legacy Drive state (historical):** Source/package identity validation exists; representative rendering acceptance remains open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-022

- **Legacy obligation:** Complete sustained real-device validation across supported Android versions, screen sizes, WebView versions, accessibility services, locale/RTL/large-text combinations, reduced-effects settings, network transitions, process/background restoration, performance, battery, and compatibility.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Open acceptance gate. Section 18 adds sustained-use, memory-pressure, process-death, multi-window/multi-tab, and runtime-state consistency requirements.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-023

- **Legacy obligation:** Complete controlled beta/production signing, key custody/recovery/rotation, signed upgrade/downgrade/rollback/data-migration testing, artifact provenance/checksums, release notes, controlled distribution, Release Candidate qualification, production approval, and Stable qualification.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Existing Android Beta APK evidence remains Development artifact evidence only; production/release gates remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-024

- **Legacy obligation:** Complete authenticated Browser → GoreeCloud Search delegation using accepted GoreeCloud Identity requester authentication plus real Privacy Shield decision/capability acquisition and Search-side capability-reference verification.
- **Legacy priority:** High
- **Legacy Drive state (historical):** PR #49 implements fail-closed capability/decision/reference contracts and keeps remote transmission disabled. Accepted runtime adapters, authenticated requester transport, Search required-mode enforcement, denial/replay evidence, and production acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-025

- **Legacy obligation:** Define, implement, and accept GoreeCloud Browser Sync as a Browser-specific synchronization service on the shared GoreeCloud Sync substrate while excluding Private/Isolated Private state and preserving independent Identity, Privacy Shield, Vault, Wardveil, Mesh, Everkeep, Bookmarks, and Browser authority.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Proposed Browser Sync architecture remains documented in Project Specification v0.11 Section 11. `docs/BROWSER_SYNC_PHASE_0.md` v0.1 defines the Phase 0 Browser-side runtime contract against the verified shared Sync substrate. No accepted runtime Browser Sync implementation exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-026

- **Legacy obligation:** Keep the authoritative Project Specification — Browser reconciled to verified current Development reality without overwriting historical evidence or claiming unverified implementation.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Authoritative project-specification control remains repository `SPECIFICATIONS.md` v0.11. Sections 16–18 define current implementation contracts; the Browser Sync Phase 0 supporting contract elaborates existing §11 without replacing it. The superseded legacy DOCX/Git evidence preserves historical detail without making it current authority.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-027

- **Legacy obligation:** Implement the Browser Sync core data plane: canonical dataset/schema consumption, stable object identities, durable Browser-local outbound change journal, idempotent submission, bounded retrieval/cursors, acknowledgements/observation tracking, retry, compaction, and offline resume.
- **Legacy priority:** High
- **Legacy Drive state (historical):** The shared Sync substrate already provides the canonical Browser dataset registry, record envelope, generic replay/high-water, observation/tombstone, and bounded retrieval foundations. Browser-local projection/journal/apply integration remains unimplemented and is specified in Browser Sync Phase 0 §§2–14.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-028

- **Legacy obligation:** Implement domain-specific Browser Sync convergence and lifecycle semantics for Browser-owned datasets while preserving separate Bookmarks authority, conflicts, deletion/tombstones, stale-device return, and Close & Forget exclusions.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Browser Sync Phase 0 constrains initial scope to canonical `browser.tabs`, `browser.history`, and portable `browser.preferences`; Bookmarks remains `bookmarks.*`. Shared generic record ordering/tombstone mechanics exist in Sync, but complete Browser application semantics and runtime convergence remain unaccepted.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-029

- **Legacy obligation:** Implement Browser Sync trust and privacy boundaries: account/profile/device isolation, trusted-device/revocation handling, Privacy Shield purpose/minimization/consent enforcement, Wardveil peer-trust acceptance, secret exclusion, and independent Vault/Everkeep/Bookmarks authority preservation.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Browser Sync Phase 0 §§10, 15–17 consume the current shared Sync acceptance boundary and add Browser-local namespace/privacy checks. No accepted production Browser Sync authority adapters exist.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-030

- **Legacy obligation:** Implement the Browser Sync adapter, truthful status/privacy controls, privacy-safe diagnostics, failure/recovery behavior, resource bounds, schema migration, and production acceptance program.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Browser Sync Phase 0 §§18–25 define the Browser-side adapter/status/failure/diagnostic/acceptance contract. Hosted/self-hosted/hybrid transport remains governed by GoreeCloud Sync rather than redefined by Browser. No production acceptance exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-031

- **Legacy obligation:** Build GoreeCloud Vault directly into GoreeCloud Browser as a native first-party credential capability, including secure username/password autofill, inline suggestions, password/passphrase generation, save/update prompts, passkeys/WebAuthn, TOTP, payment/identity autofill where permitted, Vault quick access, lock/reauthentication, origin-aware matching, private-context isolation, and user controls while preserving GoreeCloud Vault as the sole credential authority.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Project Specification v0.11 Section 12 defines native first-party Vault delivery, credential workflows, authority/data boundaries, private-context rules, platform/security integration, and acceptance gates. No native Vault credential runtime adapter, UX acceptance, production approval, or Stable qualification is claimed.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-032

- **Legacy obligation:** Implement a platform-neutral Browser PermissionBroker with normalized request IDs, profile/privacy-context binding, origin canonicalization, typed permission resources, deterministic state transitions, timeout/cancellation handling, final revalidation, and unit/fault tests.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 Section 16. Current Android callbacks still deny permission/geolocation requests directly; no accepted broker runtime exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-033

- **Legacy obligation:** Implement Browser privacy-context permission lifecycle: Normal persistent decisions where allowed, Private/Isolated Private ephemeral decisions, profile/site scoping, expiry/revocation/reset, Close & Forget cancellation and cleanup, OS-permission separation, and privacy-safe diagnostics.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 §§16.2–16.13. Runtime persistence/isolation and representative-device evidence remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-034

- **Legacy obligation:** Implement the Wardveil-gated download data plane with quarantine-first staging, stable download/attempt identities, immutable finalized objects, cryptographic digest binding, authenticated versioned Wardveil verification, explicit verdict states, and fail-closed timeout/unavailability behavior.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 §§17.1–17.8. Current Android runtime intentionally blocks downloads; no accepted scanner/release runtime exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-035

- **Legacy obligation:** Implement secure download release and recovery: TOCTOU-resistant atomic release, filename/destination safety, private-context semantics, retry/resume/process-death recovery, File Manager/Drive/external handoff after release only, privacy-safe provenance, and production acceptance tests.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 §§17.9–17.15. No accepted release/handoff path or production evidence exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-036

- **Legacy obligation:** Implement the Browser-owned runtime object model for profiles, privacy contexts, windows, tabs, Webspaces, session epochs, journals, and checkpoints with stable logical identifiers independent from transient engine objects.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 §§18.1–18.5. No accepted complete runtime model exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-037

- **Legacy obligation:** Implement durable Normal-session journaling/checkpoints, idempotent replay, corruption handling, process-death/crash restoration, schema migration, and explicit Private/Isolated Private non-durability with Close & Forget destruction semantics.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 §§18.6–18.9 and §§18.17–18.19. No accepted production restoration implementation or representative-device evidence exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-038

- **Legacy obligation:** Implement profile/Webspace isolation and runtime transitions, engine recreation/discard adapters, committed-state-only Sync projections, Browser-owned Everkeep recovery/import boundaries, resource-management policy, and the Section 18 acceptance matrix.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in Project Specification v0.11 §§18.10–18.19. Complete multi-profile/Webspace/Sync/Everkeep/runtime acceptance remains open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-039

- **Legacy obligation:** Implement canonical Browser Sync Phase 0 dataset adapters for `browser.tabs`, `browser.history`, and portable `browser.preferences`, consuming the shared Sync capability registry/envelope and preserving separate `bookmarks.*` authority.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in `docs/BROWSER_SYNC_PHASE_0.md` §§2–6. Shared Sync substrate capability definitions exist; Browser payload schemas, projection/apply adapters, and runtime integration remain unaccepted.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-040

- **Legacy obligation:** Implement the durable Browser Sync outbound journal and deterministic outbound/inbound pipelines with stable operation IDs, commit-before-project ordering, idempotent retry, profile/privacy eligibility, staged remote application, and process-death recovery.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in `docs/BROWSER_SYNC_PHASE_0.md` §§7–9 and §§15–17, 21. No accepted Browser-local Sync journal or production apply pipeline exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-041

- **Legacy obligation:** Integrate the shared Sync acceptance/replay/tombstone/receipt/retrieval foundations into Browser and pass the Phase 0 acceptance matrix, including capability/schema denial, cross-profile isolation, private-state exclusion, deletion convergence, long-offline recovery, resource bounds, diagnostics, and exact-candidate evidence.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in `docs/BROWSER_SYNC_PHASE_0.md` §§10–14 and §§18–25. Shared Sync primitives exist at Development source level; Browser integration and production acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-042

- **Legacy obligation:** Define and implement the GoreeCloud Browser native extension platform without external extension-ecosystem dependencies or compatibility layers, using the Browser-owned extension runtime, `.gcex` package format, native manifest, independently versioned Extension API, and secure decentralized installation model.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Development source includes manifest/API v1 contracts, closed semantic manifest/package validation, the bounded GCEX v1 byte reader/strict native manifest decoder, and a standalone package-local Ed25519/SHA-256 signature-verification layer. Reader source `80c577e…` and signature source `756e368…` have exact-source Development evidence. Installation, trusted developer identity, runtime/API dispatch, sandboxed execution, and production acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-043

- **Legacy obligation:** Implement least-privilege extension authorization with human-readable permissions, per-site access, temporary grants, capability-scoped authorization, profile-specific installation/permissions, private-browsing opt-in, and strong isolation from Browser internals, other extensions, unrelated websites, services, credentials, and system files.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Development source has the closed permission vocabulary, exact declared-website checks, profile-bound grants, private-browsing opt-in, explicit scopes, and once/tab/site/browser/one-hour/always lifetimes. `ExtensionPermissionLedger` provides stateful consumption/expiry/revocation. `extension_permission_ledger_storage.hpp` adds bounded per-profile persistence/recovery for only `always` and still-valid `one_hour` leases, excludes volatile lifetimes, preserves original one-hour expiry, rejects profile mismatch/clock regression/corruption, and can recover a previous valid backup. Platform Contract #72, Android Beta #250, and Core CI #491 passed on exact implementation source `2026b06…`. Production runtime integration, authenticated-at-rest storage, process isolation, and production acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-044

- **Legacy obligation:** Implement extension transparency and containment controls including process/resource visibility, network activity reporting, privacy dashboards, local permission-activity history, extension-specific firewall policy, storage quotas, advanced resource limits, and local safety checks.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Specified in `docs/NATIVE_EXTENSION_PLATFORM.md` §§11–15, 31, 37–38. No accepted runtime telemetry, firewall, quota, or safety-check implementation exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-045

- **Legacy obligation:** Implement bounded Glaze UI extension surfaces while protecting Browser-controlled security UI from silent imitation, including clear separation of extension-controlled and Browser-controlled prompts, warnings, privacy indicators, settings, and installation surfaces.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Specified in `docs/NATIVE_EXTENSION_PLATFORM.md` §§18–19. No accepted extension UI surface or anti-spoofing implementation exists.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-046

- **Legacy obligation:** Implement decentralized extension provenance, signing/trust states, user-approved update sources, permission-delta review, rollback, crash-triggered disable, browser-wide emergency disable, and compatibility reporting without requiring a GoreeCloud extension store.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** Exact Development source `756e368…` implements package-local `ed25519-sha256-v1` verification through a GoreeCloud adapter over pinned libsodium 1.0.22, with repository provenance/license metadata and a dedicated exact-source verification lane. Successful verification can establish only `signed_package`; `verified_signature` still requires an independent trusted developer-key/organization authority. Signature enforcement is not wired into the ordinary Browser runtime. Update trust, rollback, emergency-disable runtime, compatibility engine, production trust decisions, and centralized-store-free update acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-047

- **Legacy obligation:** Deliver Developer Mode, declarative APIs where practical, structured API deprecation, the GoreeCloud Extension SDK, and CLI tooling for create/validate/test/run/package/sign/inspect/permissions without developer registration.
- **Legacy priority:** High
- **Legacy Drive state (historical):** Specified in `docs/NATIVE_EXTENSION_PLATFORM.md` §§4, 27–28, and 34–35. SDK, CLI, developer runtime, API-version compatibility, and migration tooling remain planned.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-048

- **Legacy obligation:** Implement profile-isolated extension storage, quotas, private-context semantics, optional configuration backup/synchronization, organization-managed deployment, and trusted-signing policy without synchronizing executable packages as ordinary Browser state.
- **Legacy priority:** High
- **Legacy Drive state (historical):** The permission-ledger snapshot is a bounded Browser-owned permission-state primitive, not general extension storage or Sync. Dedicated extension data storage classes, quotas, configuration Sync/backup integration, and organization deployment remain planned.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

### FR-049

- **Legacy obligation:** Complete native extension adversarial/fault testing, package-corruption/signature/permission-escalation/site/private/profile/network/quota/crash/update/rollback/API/UI-spoofing/accessibility/resource-abuse tests and representative-device/sustained-use acceptance before any production claim.
- **Legacy priority:** Critical
- **Legacy Drive state (historical):** GCEX reader/decoder smoke coverage is exact-source verified at `80c577e…`; permission-ledger storage smoke coverage is exact-source verified at `2026b06…`. The dedicated signature lane #6 passed on exact source `756e368…` with assertions enabled and covers unsigned behavior, deterministic digesting, valid Ed25519 verification, `signed_package`-only promotion, fingerprint generation, covered-file tampering, incomplete signature material, and unknown signature-namespace rejection. Broader sandbox/runtime, developer-key trust/revocation, fuzz/adversarial, accessibility, representative-device, production, and Stable acceptance remain open.
- **Migration disposition:** Migrated into the repository-native feature authority. The legacy Drive state below is historical evidence only; current lifecycle state is controlled by the current sections of PLANNED-FEATURES.md, IMPLEMENTED-FEATURES.md, specifications, and verified repository evidence.

This ledger does not restore FEATURE-ROADMAP.md or Drive synchronization. Current state must be determined from the repository-native feature records and verified evidence above.

## Maintenance rule

Move an item to `IMPLEMENTED-FEATURES.md` only after the authoritative implementation and required verification are integrated. Record material lifecycle changes in `CHANGELOGS.md`. Keep actionable execution work in GoreeCloud Tasks Management without creating duplicate task authority.
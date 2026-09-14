# GoreeCloud Browser — Feature Roadmap

**Status:** Active roadmap control  
**As of:** 2026-09-14  
**Authoritative project record:** Project Specification — Browser  
**Canonical repository:** GoreeCloud/goreecloud-browser  
**Drive control:** `GoreeCloud/Feature Roadmap/GoreeCloud Browser/FEATURE-ROADMAP.docx`

## Purpose

This file is the repository-side feature roadmap control for GoreeCloud Browser. It records current planned and recommended feature work without replacing the authoritative Project Specification, verified repository implementation evidence, release gates, or GoreeCloud Tasks Management.

## Current Development checkpoint

The authoritative Project Specification still describes the older V1.3 Development stack, so that document requires separate reconciliation through the approved Google Docs preservation workflow. Current repository reality has advanced beyond that historical checkpoint.

Active PR #49 is a Development/beta candidate at verified implementation head `32728ad53219b738eeff613906316692746b215f`. That exact revision passed Platform Contract #22, Browser Core CI #439, and Android Beta APK #200, including Android test/lint/assembly, signature/package identity verification, SHA-256 recording, and installable APK artifact upload.

The PR #49 candidate now:

- targets current Stable Glaze UI V1.4 / `1.4.0` while retaining migration-required status pending Browser-local rendered/native, accessibility, localization/RTL, large-text, reduced-effects, representative-device, performance, and production acceptance;
- declares the seven authoritative Integral Platform Systems only: GoreeCloud Manager, Privacy Shield, Wardveil Security, Everkeep, Glaze UI, GoreeCloud Mesh, and GoreeCloud Identity;
- keeps GoreeCloud Search, GoreeCloud Index, GoreeCloud Sync, Vault, DNS, Network, Bookmarks, and other first-party products/services as separate capability/authority relationships rather than misclassifying them as Platform Systems;
- preserves explicit omnibox classification among direct Navigation, Search intent, Home, and Blocked input;
- keeps free-text Search transport-neutral instead of manufacturing a query-bearing `?q=` navigation URL;
- requires production Search capability evidence for POST + `json_body`, JSON media types, result/request bounds, Privacy Shield opaque capability-reference transport, server enforcement `required`, and authenticated requester identity;
- creates unique Privacy Shield request IDs and requires exact decision correlation before a production Search handoff can be constructed;
- validates canonical opaque `psc_*` references at both authorization and final request construction;
- keeps actual remote Search transmission disabled until accepted runtime Privacy Shield acquisition, GoreeCloud Identity requester authentication, and Search-side capability-reference verification are available;
- rejects malformed, credential-bearing, or unsupported navigation inputs rather than silently reinterpreting them as Search;
- redacts typed queries, navigation URLs, blocked omnibox input, and authorization references from affected debug rendering;
- preserves the fail-closed Wardveil download-release model, current Everkeep recovery boundaries, rendering-engine dependency separation, and Development/beta release status.

The current Browser roadmap must therefore treat the older V1.3 specification text as stale documentation, not current implementation authority. No current PR evidence upgrades Browser to production or Stable.

## Roadmap

| ID | Feature / obligation | Priority | Current state |
| --- | --- | --- | --- |
| FR-001 | Reconcile and maintain every current planned or recommended GoreeCloud Browser feature from the authoritative project record and verified repository evidence in this roadmap. | High | Ongoing control; synchronized to the verified September 14 PR #49 checkpoint. Project Specification reconciliation remains open. |
| FR-002 | Move actionable feature obligations into GoreeCloud Tasks Management when required, preserving priority, dependency, and lifecycle disposition. | High | Ongoing control; GOR-24 tracks the active Browser modernization work and exact checkpoints. |
| FR-003 | Do not mark features implemented, complete, cancelled, or superseded without authoritative evidence and synchronized repository/Drive roadmap updates. | High | Ongoing control; Development source/build evidence remains distinct from production/Stable acceptance. |
| FR-010 | Preserve GoreeCloud Search as the sole approved Internet/current-information query authority while keeping structurally valid direct HTTP(S) URL navigation independent from search. | High | PR #49 formalizes transport-neutral Search intent, capability preflight, operation-scoped Privacy Shield authorization preparation, and no silent third-party fallback. Live Search transport remains disabled. |
| FR-011 | Complete Browser-owned navigation and address safety including malformed URL handling, scheme/deep-link handoff, user-info disclosure, IDN/confusable/spoofing policy, origin/certificate presentation, and bidirectional address behavior without manufacturing DNS/origin trust. | High | Current source rejects malformed/credential-bearing unsupported navigation shapes and retains earlier canonical-host hardening. Full confusable/spoofing, origin/certificate, bidi, and representative-device acceptance remain open. |
| FR-012 | Complete Browser-owned main-frame failure/recovery and session restoration behavior across Android recreation, process death, OEM/WebView variants, network transitions, and representative failure modes while keeping raw remote error content out of Browser-owned recovery UI. | High | Development source recovery boundaries exist; process-death/OEM/physical-device acceptance remains open. |
| FR-013 | Complete Glaze UI V1.4 / `1.4.0` Browser migration and rendered/native-device visual, adaptive/form-factor, accessibility, large-text, contrast, localization/RTL, reduced-effects, rollback, performance, and Human Visual Excellence acceptance. | High | PR #49 targets current Stable V1.4 and records migration-required status. Browser-local rendered/native/device/product acceptance remains open. |
| FR-014 | Complete Browser-owned website permission and geolocation UX with explicit Android runtime-permission mapping and applicable Privacy Shield/Wardveil policy evidence; keep requests denied while authority is unavailable. | High | Planned / fail-closed by default. |
| FR-015 | Integrate authenticated Browser-to-Wardveil download verification, scanning, release/quarantine decisions, provenance, and safe failure behavior before enabling production downloads. | High | Source-level fail-closed release contract exists; authenticated runtime scanner health, quarantine evidence, concurrency-safe staging/release, and production acceptance remain open. |
| FR-016 | Complete Privacy Shield policy, consent, minimization, destination/retention enforcement, filtering, disclosure, and evidence surfaces for Browser-owned data flows and user controls. | High | PR #49 adds operation-scoped Search authorization request/decision/reference validation but no accepted runtime decision acquisition. Broader Browser Privacy Shield acceptance remains open. |
| FR-017 | Complete Private Browsing and Isolated Private runtime request-context/storage isolation, Close & Forget behavior, deletion/cleanup evidence, and representative lifecycle acceptance. | High | Planned / incomplete; private state remains excluded from ordinary persistence/synchronization claims. |
| FR-018 | Complete GoreeCloud Identity, Vault, GoreeCloud Sync, Everkeep, DNS, Network, Mesh, and applicable Manager integrations under independent authority boundaries with accepted runtime evidence. | High | Platform/product integration remains incomplete. PR #49 explicitly separates the seven Platform Systems from other first-party services and keeps Sync-specific obligations separate. |
| FR-019 | Complete Android multi-tab, session, history/state, Settings, profiles/account behavior, private-browsing surfaces, and other release-scope Browser product workflows while preserving local-first/fail-closed policy boundaries. | High | Partial / planned. |
| FR-020 | Maintain explicit rendering-engine dependency boundaries so Android System WebView/Chromium, desktop CEF/Chromium, and future mature engines remain replaceable implementation dependencies rather than GoreeCloud product/policy authority. | Medium | Active architectural control; desktop render-capable runtime acceptance remains incomplete. |
| FR-021 | Complete canonical Browser artwork/package provenance and representative launcher/device rendering acceptance without treating packaged-resource presence as visual acceptance. | Medium | Source/package identity validation exists; representative rendering acceptance remains open. |
| FR-022 | Complete sustained real-device validation across supported Android versions, screen sizes, WebView versions, accessibility services, locale/RTL/large-text combinations, reduced-effects settings, network transitions, process/background restoration, performance, battery, and compatibility. | High | Open acceptance gate. |
| FR-023 | Complete controlled beta/production signing, key custody/recovery/rotation, signed upgrade/downgrade/rollback/data-migration testing, artifact provenance/checksums, release notes, controlled distribution, Release Candidate qualification, production approval, and Stable qualification. | High | Android Beta APK #200 is Development artifact evidence only; production/release gates remain open. |
| FR-024 | Complete authenticated Browser → GoreeCloud Search delegation using accepted GoreeCloud Identity requester authentication plus real Privacy Shield decision/capability acquisition and Search-side capability-reference verification. | High | PR #49 implements fail-closed capability/decision/reference contracts and keeps remote transmission disabled. Accepted runtime adapters, authenticated requester transport, Search required-mode enforcement, denial/replay evidence, and production acceptance remain open. |
| FR-025 | Define and accept GoreeCloud Sync datasets only for approved normal durable Browser-owned state while excluding Private/Isolated Private state, credentials, passkeys, reusable secrets, unrelated protected evidence, and other prohibited material. | High | Privacy exclusions are documented/source-modeled; accepted dataset contracts, runtime registration, reconciliation, conflict/deletion behavior, and cross-device acceptance remain open. |
| FR-026 | Reconcile the authoritative Project Specification — Browser from its historical V1.3 stack to verified current V1.4 Development reality without overwriting historical evidence or claiming unverified implementation. | High | Required documentation reconciliation remains open; roadmap and PR records now explicitly flag the specification lag. |

## Current sequencing recommendation

1. Establish accepted GoreeCloud Identity requester authentication for Browser → Search and the corresponding Search → Privacy Shield direct-service authentication path.
2. Implement runtime Privacy Shield decision/capability acquisition and only then connect bounded Search discovery/POST transport after Search proves required server-side enforcement and authenticated requester resolution.
3. Continue Wardveil download-release runtime integration and complete Browser-owned permission/privacy/private-browsing isolation work without weakening fail-closed behavior.
4. Complete Glaze UI V1.4 rendered/native accessibility, localization/RTL, large-text, reduced-effects, representative-device, and performance acceptance.
5. Complete product-surface workflows, Sync/Everkeep/other service integrations, desktop engine acceptance, production signing/provenance, recovery/rollback, Release Candidate, production, and Stable gates.
6. Reconcile the authoritative Browser Project Specification through the approved native Google Docs preservation workflow so canonical documentation catches up to verified implementation reality.

## Maintenance and synchronization

This roadmap and the corresponding Drive `FEATURE-ROADMAP.docx` must remain materially synchronized with one another and with the authoritative Project Specification. Update both copies whenever feature scope, priority, dependency, implementation status, cancellation, supersession, recommendation, or verification state materially changes.

No feature may be represented as complete, Release Candidate, production-approved, or Stable solely because it appears in this roadmap. Lifecycle claims require applicable authoritative implementation, validation, review, release, and production evidence.

## Reconciliation rule

At each material Browser change, reconcile this roadmap against the authoritative Project Specification, verified repository state, GoreeCloud Search authority, applicable Platform Systems, rendering-engine dependency boundaries, and GoreeCloud Tasks Management. Missing obligations, stale status, duplicated work, roadmap drift, or undocumented disposition changes are defects to correct.

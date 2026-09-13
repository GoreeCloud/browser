# GoreeCloud Browser — Feature Roadmap

**Status:** Active roadmap control — as of September 12, 2026  
**Authoritative project record:** `GoreeCloud/Projects/Project Specification — Browser`  
**Canonical repository:** `GoreeCloud/goreecloud-browser`  
**Drive counterpart:** `GoreeCloud/Feature Roadmap/GoreeCloud Browser/FEATURE-ROADMAP.docx`

This repository roadmap mirrors the Drive-side Browser roadmap and records current planned and recommended obligations without replacing the authoritative Project Specification, repository implementation evidence, release gates, or GoreeCloud Tasks Management. A roadmap status is not production, Release Candidate, or Stable evidence by itself.

Current Development checkpoint: the active GLAZE UI V1.3 Android stack includes Draft PR #45 (`governance/browser-feature-roadmap-control`) exact head `72f6535ca04a810ea587f05e0e44ab022c099098`, which passed path-applicable Browser Core CI `34545259063` / #382, and stacked Draft PR #46 (`hardening/android-idn-a-label-20260912`). PR #46 exact source head `fec4d4cc6ef9e0eda7a04d034dca1faffa1f93e0` passed Browser Core CI `34733687604` / #385 and Android Beta APK `34733687633` / #148. That source boundary canonicalizes valid internationalized HTTP(S) host identity to ASCII A-labels for Browser navigation/presentation, rejects malformed STD3 host labels fail closed, preserves explicit ports/path/query/fragment, and does not claim UTS #39 confusable detection, DNS/certificate trust, reputation, or representative-device acceptance.

## Roadmap

| ID | Feature / obligation | Priority | Current state |
| --- | --- | --- | --- |
| FR-001 | Reconcile and maintain every current planned or recommended GoreeCloud Browser feature from the authoritative project record and verified repository evidence in this roadmap. | High | Ongoing control |
| FR-002 | Move actionable feature obligations into GoreeCloud Tasks Management when required, preserving priority, dependency, and lifecycle disposition. | High | Ongoing control |
| FR-003 | Do not mark features implemented, complete, cancelled, or superseded without authoritative evidence and synchronized repository/Drive roadmap updates. | High | Ongoing control |
| FR-010 | Preserve GoreeCloud Search as the sole approved query/search authority while keeping structurally valid direct HTTP(S) URL navigation independent from search. | High | Active Development boundary; current V1.3 stack has fail-closed navigation-shape, user-info handling, and canonical internationalized-host routing; broader acceptance pending |
| FR-011 | Complete Browser-owned navigation and address safety including malformed URL handling, scheme/deep-link handoff, user-info disclosure, IDN/confusable/spoofing policy, origin/certificate presentation, and bidirectional address behavior without manufacturing DNS/origin trust. | High | Development hardening through PR #46 adds STD3/A-label internationalized-host canonicalization and safer canonical address presentation while preserving ports/path/query/fragment. Full UTS #39 confusable/spoofing analysis, origin/certificate presentation, bidirectional-address acceptance, and representative-device acceptance remain open. |
| FR-012 | Complete Browser-owned main-frame failure/recovery and session restoration behavior across Android recreation, process death, OEM/WebView variants, network transitions, and representative failure modes while keeping raw remote error content out of Browser-owned recovery UI. | High | Development source boundary exists; process-death/OEM/physical-device acceptance pending |
| FR-013 | Complete GLAZE UI V1.3 / `1.3.0` Browser application migration and rendered/native-device visual, adaptive/form-factor, accessibility, large-text, contrast, localization/RTL, reduced-motion, Touch Assistance where applicable, rollback, performance, and Human Visual Excellence acceptance. | High | `applicable-migration-required`; source-level V1.3 mapping exists, application acceptance pending |
| FR-014 | Complete Browser-owned website permission and geolocation UX with explicit Android runtime-permission mapping and applicable Privacy Shield/Wardveil policy evidence; keep requests denied while authority is unavailable. | High | Planned / fail-closed by default |
| FR-015 | Integrate authenticated Browser-to-Wardveil download verification, scanning, release/quarantine decisions, provenance, and safe failure behavior before enabling production downloads. | High | Downloads remain blocked pending accepted integration |
| FR-016 | Complete Privacy Shield policy, consent, minimization, filtering, disclosure, retention, and evidence surfaces for Browser-owned data flows and user controls. | High | Planned — platform-system acceptance pending |
| FR-017 | Complete Private Browsing runtime request-context/storage isolation, Close & Forget behavior, deletion/cleanup evidence, and representative lifecycle acceptance. | High | Planned / incomplete |
| FR-018 | Complete GoreeCloud Identity, Vault, Sync, Everkeep, DNS, Network, Mesh, and applicable Manager integrations under independent authority boundaries with accepted runtime evidence. | High | Planned — platform-system acceptance pending |
| FR-019 | Complete Android multi-tab, session, history/state, Settings, profiles/account behavior, private-browsing surfaces, and other release-scope Browser product workflows while preserving local-first/fail-closed policy boundaries. | High | Partial / planned |
| FR-020 | Maintain explicit rendering-engine dependency boundaries so Android System WebView/Chromium and future mature engines remain replaceable implementation dependencies rather than GoreeCloud product/policy authority. | Medium | Active architectural control |
| FR-021 | Complete canonical Browser artwork/package provenance and representative launcher/device rendering acceptance without treating packaged-resource presence as visual acceptance. | Medium | Source/package provenance exists; representative rendering acceptance pending |
| FR-022 | Complete sustained real-device validation across supported Android versions, screen sizes, WebView versions, accessibility services, locale/RTL/large-text combinations, network transitions, process/background restoration, performance, battery, and compatibility. | High | Open acceptance gate |
| FR-023 | Complete controlled beta/production signing, key custody/recovery/rotation, signed upgrade/downgrade/rollback/data-migration testing, artifact provenance/checksums, release notes, controlled distribution, Release Candidate qualification, production approval, and Stable qualification. | High | Open release gate |

## Recommended sequencing

1. Keep the current V1.3 navigation/recovery and canonical-IDN stack as Development evidence only.
2. Close remaining UTS #39/confusable/spoofing, origin/certificate presentation, bidirectional-address, and process/OEM restoration safety gaps before broadening Browser permission or external-handoff authority.
3. Integrate Browser-owned permission UX, Wardveil download verification, Privacy Shield, Private Browsing isolation, and required Identity/Vault/Sync/Everkeep/DNS/Network/Mesh/Manager boundaries with producer-authoritative evidence.
4. Complete GLAZE UI V1.3 rendered/accessibility/localization/RTL/large-text/adaptive/performance acceptance and sustained representative-device validation.
5. Finish product-surface completeness, controlled production signing, upgrade/rollback/migration, release provenance, and explicit Release Candidate/production/Stable gates.

## Maintenance and synchronization

Update this file and the Drive `FEATURE-ROADMAP.docx` together whenever feature scope, priority, dependency, implementation status, cancellation, supersession, recommendation, or verification state materially changes. Historical checkpoints remain historical evidence and must not be silently rewritten into current acceptance claims.

## Reconciliation rule

At each material Browser change, reconcile this roadmap against the authoritative Project Specification, verified repository state, GoreeCloud Search authority, the applicable platform systems, rendering-engine dependency boundaries, and GoreeCloud Tasks Management. Missing obligations, stale status, duplicated work, roadmap drift, or undocumented disposition changes are defects to correct.

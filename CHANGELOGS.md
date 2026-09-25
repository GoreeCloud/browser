# GoreeCloud Browser — Changelogs

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## 2026-09-23 — Current-main internationalized host canonicalization

### Changed

- Replayed only the still-unintegrated IDN host-identity logic from historical PR #46 onto current authoritative Android architecture.
- Unicode DNS host names are converted to ASCII A-label form before direct HTTP(S) navigation and Browser-owned unfocused address presentation.
- STD3-invalid labels, invalid port syntax/ranges, credential-bearing authorities, bracketed hosts that are not valid IPv6 literals, and ambiguous numeric dotted host forms fail closed; only canonical four-octet decimal IPv4 is accepted as numeric direct-navigation identity.
- Preserved path, query, fragment, explicit port, Search-versus-navigation classification, and existing fail-closed unsupported-scheme behavior.
- Added current-main regression coverage for direct and scheme-less Unicode domains, A-label presentation, invalid STD3 labels, explicit ports, and bracketed IPv6 validation.

### Lifecycle boundary

This is Development source hardening. It does not establish complete UTS #39/confusable/spoofing analysis, DNS/certificate/reputation trust, origin presentation acceptance, representative-device acceptance, Release Candidate, or Stable qualification.

## 2026-09-22 — Repository feature/changelog governance migration

### Added

- `IMPLEMENTED-FEATURES.md` as the authoritative implemented-feature inventory.
- `PLANNED-FEATURES.md` as the authoritative planned/incomplete-feature inventory.
- `CHANGELOGS.md` as the authoritative repository change-history record.

### Changed

- Retired the repository `FEATURE-ROADMAP.md` control model.
- Removed the obsolete requirement to synchronize feature-roadmap authority with Google Drive.
- Preserved `FEATURES.md`, `SPECIFICATIONS.md`, supporting contracts, Git history, pull requests, workflow evidence, and `NOTES.md` as product/specification/evidence sources rather than duplicate feature-lifecycle authority.
- Recorded the current V1.5.1 Browser source mapping as migration-required against current Official Stable GLAZE UI V1.6 / 1.6.0 rather than silently rebinding earlier evidence.

### Lifecycle boundary

This migration changes documentation/control-plane authority only. Browser remains Development/non-Stable. Representative-device usability/accessibility/performance, current V1.6 application acceptance, platform integrations, recovery, production signing/deployment, Release Candidate, and Stable qualification remain open.

## Historical change evidence

Historical implementation and validation evidence remains preserved by Git history, merged pull requests, repository documentation, workflow records, exact-source evidence, and product-specific governed evidence. Future material integrated changes must be recorded here.

## Maintenance rule

Record material integrated changes here with enough exact repository evidence to distinguish authoritative `main` state from draft/unmerged work. Do not convert source presence, an installable beta artifact, or green CI into production or Stable claims.
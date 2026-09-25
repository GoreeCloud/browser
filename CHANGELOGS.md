# GoreeCloud Browser — Changelogs

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## 2026-09-23 — Privacy-safe blocked deep-link feedback

### Changed

- Web-initiated unsupported main-frame schemes remain fail-closed but now show a Browser-owned local explanation instead of failing silently.
- The local surface receives only a syntactically bounded scheme label such as `mailto: link`; phone numbers, email addresses, intent extras, tokens, and other target payload data are not rendered.
- Unsupported subframe/custom-scheme attempts remain blocked without creating a user-facing payload surface.
- The blocked top-level safety-surface marker survives Android instance-state recreation without persisting the blocked target, preventing the synthetic local rendering origin from becoming Browser-visible/copyable page state after rotation or recreation.
- Added focused unit coverage for scheme-only labeling and the generic local safety page.

### Lifecycle boundary

This Development hardening does not enable external-app handoff, deep-link execution, new intent permissions, Search delegation, production website permissions, representative-device acceptance, Release Candidate, or Stable qualification.

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
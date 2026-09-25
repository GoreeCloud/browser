# GoreeCloud Browser — Changelogs

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## 2026-09-25 — Glaze UI V1.6 source adoption and GTK chrome stabilization

- Migrated Browser source/build pins from GLAZE UI V1.5.1 to consumer-eligible GLAZE UI V1.6 / 1.6.0 using accepted release source `a7180679ea851389e0f3004515f9a25f420e716d`, qualification source `c7509c79256b04b0aa67cb9dd0737d7588e0ae4a`, and qualification evidence integration `354f5759385c28596fcfec26a3ad525e89fb1c35`.
- Expanded Browser-local V1.6 capability and acceptance contracts for component-state completeness, large-text reflow, non-color semantic meaning, responsive continuity, localization, provenance, bounded complexity, and accessibility/performance fallback behavior.
- Migrated the Android-native Glaze contract to V1.6 and added explicit disabled control presentation while preserving authority boundaries and deterministic effects-free fallback.
- Reworked the Linux GTK beta shell from a flat text-heavy toolbar into grouped primary navigation, a dominant omnibox, neutral privacy/security entry points, accessible secondary-tool overflow, local symbolic-icon fallbacks, structured tabs, and structured Browser-owned internal/tool surfaces.
- Corrected stale Browser Glaze documentation, including the obsolete "Glaze UI 2.0" surface-model reference and the outdated stabilization-base revision.
- Added exact-source smoke gates for the V1.6 version, release source, qualification source, rollback baseline, and required Browser presentation capabilities.

This tranche is implementation/source-adoption evidence only. Browser remains Development and non-production-approved; exact-revision rendered/native visual, accessibility, large-text, localization/RTL, representative-device/form-factor, performance, rollback, and production acceptance remain open.

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
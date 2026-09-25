# GoreeCloud Browser — Changelogs

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## 2026-09-25 — Linux desktop Glaze V1.6 source migration tranche

### Changed

- Mapped Browser source guards from historical GLAZE UI V1.5.1 to exact current Stable GLAZE UI V1.6 / 1.6.0 release source `a7180679ea851389e0f3004515f9a25f420e716d`.
- Reworked the GTK/X11 Development chrome around a compact active-tab surface, primary navigation capsule, bounded secondary tools popover, branded first-party surfaces, and explicit Development state.
- Added friendly first-party tab titles for New Tab, Home, Settings, and Private Browsing.
- Reordered initial GTK presentation so the visible first-party surface is selected after the window is shown.
- Lowered the project CMake floor to 3.22 and moved the dedicated GTK beta-shell CI lane to Ubuntu 22.04 for Zorin/Ubuntu 22.04-class compatibility.
- Corrected stale Browser Glaze documentation that referenced an obsolete Glaze 2.0 baseline.

### Lifecycle boundary

This tranche is Development source implementation only. It does not establish rendered/accessibility/device/performance acceptance, render-capable CEF/Chromium desktop acceptance, Release Candidate, production approval, or Stable status.

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
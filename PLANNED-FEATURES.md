# GoreeCloud Browser — Planned Features and Open Obligations

**Record type:** Repository planned/incomplete-feature inventory  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Authority:** Current `main` source, accepted repository evidence, and active GoreeCloud Tasks Management obligations  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## Interpretation

Items here are planned, incomplete, blocked, or acceptance-gated. Their presence does not imply implementation or release readiness. Partial foundations that already exist are also described in `IMPLEMENTED-FEATURES.md` for the verified portion only.

## Current stabilization obligations

- Complete Browser-specific acceptance for the current GLAZE UI V1.6 / 1.6.0 source mapping and Linux desktop presentation tranche: rendered review, accessibility, large text, localization/RTL, adaptive/form-factor/posture behavior, reduced effects, performance, rollback, Human Visual Excellence, and representative-device acceptance.
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

- Complete cross-platform tab strip/tab switcher and multi-tab lifecycle beyond the current Linux GTK Development create/activate/close controls, including ordering, pinned/grouped tabs, overflow/search, session persistence, crash/process recovery, accessibility, and representative-device acceptance.
- Complete production-grade Bookmarks, history, library, settings, downloads, Reader Mode, and Wayfinder mobile surfaces beyond the current Linux Development panels/start-surface scaffolding; add real persistence, synchronization/provider adapters, commands, empty/error/loading states, accessibility, and representative-device acceptance.
- Full private-browsing product surfaces and Close & Forget behavior.
- Complete navigation/address safety for malformed URLs, schemes/deep links, user-info disclosure, IDN/confusable/spoofing, origin/certificate presentation, and bidirectional text.
- Mature session restoration and Browser-owned durable recovery.
- Complete native extension installation, sandbox/process runtime, trusted developer-key authority, extension networking/storage/UI, privileged APIs, updates, developer tooling, and production trust/acceptance.
- Complete the pinned Linux CEF render candidate through real CEF compilation and representative-device runtime validation: HTTPS rendering/navigation, engine-owned multi-tab behavior including reordering, TLS/certificate state, sandbox/site-isolation preservation, renderer crash handling, private-context isolation/cleanup, accessibility, performance, packaging, and the remaining `docs/BETA_0_1.md` render-capable gates while preserving rendering-engine replaceability.
- Canonical artwork/package provenance and representative launcher/device rendering acceptance.
- Sustained device validation across supported Android versions, screen sizes, WebView versions, accessibility services, locales, large text, reduced effects, network transitions, background/process restoration, battery, and compatibility.

## Explicit non-claims

Until corresponding evidence exists, this file does not claim current V1.6 Browser acceptance, production website permissions/downloads, production Search/Vault/Sync authority integration, production extension execution/trust, production signing/distribution, Release Candidate, Production Acceptance, or Stable status.

## Maintenance rule

Move an item to `IMPLEMENTED-FEATURES.md` only after the authoritative implementation and required verification are integrated. Record material lifecycle changes in `CHANGELOGS.md`. Keep actionable execution work in GoreeCloud Tasks Management without creating duplicate task authority.
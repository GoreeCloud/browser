# GoreeCloud Browser 0.1.0-beta.1

**Release channel:** Beta  
**Production approved:** No  
**Purpose:** Establish the first testable GoreeCloud Browser source milestone with GoreeCloud-owned product architecture, build/test gates, and a concrete render-path integration boundary.

## Beta scope

Beta 0.1 is a development/testing milestone. It does not claim that every planned GoreeCloud Browser subsystem is feature-complete or production-ready.

The beta source milestone requires the following Browser-owned contracts to be present and build-tested:

- GoreeCloud Browser product identity and first-party internal-page routing.
- Latest approved Stable Glaze UI baseline enforcement.
- Fixed 12-control desktop toolbar contract.
- Unified Search Bar with Advanced Reader Mode, Unified Bookmarks, and Wardveil Security controls.
- GoreeCloud Search as the sole search authority, with direct URL navigation remaining independent.
- New Tab, Home, and Settings first-party routes.
- Advanced Tab Manager core state and protection behavior.
- Unified Bookmarks dual authority: Local Bookmarks and GoreeCloud Bookmarks.
- Unified Vault dual authority: Local Vault and GoreeCloud Vault.
- Private Browsing session, cleanup, permission, lock, and persistence-boundary contracts.
- Browser Engine Layer abstraction.
- Development engine fallback.
- Chromium adapter build path.
- Optional CEF runtime path with subprocess dispatch and request-context separation.
- Runtime smoke tests through CTest.
- Core Linux CI builds in Release and Debug configurations.

## Current pinned desktop renderer candidate

The Linux Development render path pins CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` for the current render-capable milestone. The repository bootstrap uses the official CEF binary-distribution service, verifies the official checksum before extraction, records a local SHA-256 provenance value, and CMake rejects a CEF root whose reported version differs from the repository pin.

This pin establishes a reproducible candidate dependency only. It does **not** by itself satisfy any render-capable Beta acceptance gate.

## Render-capable beta gate

A desktop artifact must not be described as render-capable Beta until all of the following are demonstrated in an actual build using the selected CEF/Chromium runtime:

1. GoreeCloud Browser opens a visible native desktop window.
2. The top-level window and Browser chrome are GoreeCloud-owned Glaze UI surfaces.
3. A CEF/Chromium child web-content view is embedded only in the content region.
4. HTTPS web pages render and navigate successfully.
5. Back, Forward, Refresh/Stop, Home, and direct URL navigation operate against the real renderer.
6. The unified search bar sends search queries only to GoreeCloud Search.
7. New Tab, Home, and Settings display first-party GoreeCloud surfaces.
8. At least two tabs can be created, activated, reordered, and closed without losing engine-view ownership.
9. Private and normal windows use separate request contexts and do not share private cookies/site storage.
10. Close & Forget destroys the applicable private runtime context and temporary Browser-owned state.
11. TLS/certificate validation and Chromium sandbox/site-isolation requirements remain enabled.
12. A real Linux runtime smoke test passes outside the engine-independent test scaffold.

Until those gates pass, the repository may carry the Beta source version while documentation must identify the desktop renderer as under integration.

## Deferred from Beta 0.1 acceptance

The following planned systems may remain incomplete in Beta 0.1 provided their product boundaries are preserved and they are not represented as complete:

- Full accelerated segmented Download Manager implementation and torrent engine.
- Complete Privacy Shield filtering engine.
- Full Wardveil threat-classification backend.
- Production GoreeCloud Sync service.
- Production GoreeCloud Vault service and migration.
- GoreeCloud DNS and GoreeCloud Network service adapters.
- Full Advanced Tab Manager Workspaces/Split View persistence and UI.
- Complete Reader Mode implementation.
- Full Web Clipper destination adapters.
- Complete Notes, Memos, Tasks, Calendar, Contacts, and AI service-write adapters.
- Windows installer production acceptance.
- Android APK production acceptance.
- Flatpak production acceptance.
- Signing, update, rollback, and Stable release evidence.

## Beta safety and privacy rules

Beta status does not relax Browser security or privacy requirements. The Browser must not disable sandboxing, site isolation, TLS validation, or required certificate/security mechanisms merely to make the beta run. Private Browsing must remain outside ordinary Sync and ordinary session recovery. First-party protection claims must remain evidence-backed.

## Promotion beyond Beta

Promotion from Beta requires real runtime evidence rather than source presence alone. CI establishes build/test health only. Stable promotion requires the wider production validation, packaging, signing, recovery, accessibility, security, compatibility, and sustained-use evidence defined elsewhere in the repository.

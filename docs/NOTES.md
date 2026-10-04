# GoreeCloud Browser — Development Notes

## Current stabilization context

- Repository lifecycle remains Development and is not Release Candidate, production accepted, Stable, Seal, or Anchor-qualified as a product.
- Live protected `main` and exact-revision workflow evidence are authoritative for current implementation state; this file does not pin a volatile main SHA.
- Browser source now maps the current bounded Glaze V1.7 / `1.7.0` Anchor identity while inheriting the accepted V1.6.0 runtime behavior. Fresh Browser-local V1.7 rendered/native, accessibility, representative-device, performance, rollback, release, and production acceptance remain open.
- Android has an installable GoreeCloud-owned browser shell with direct HTTP(S) navigation, Browser-owned chrome, local start/error/search-blocked surfaces, fail-closed free-text Search delegation, adaptive/round/monochrome launcher resources, and exact-source APK CI.
- Source presence and green CI do not replace representative physical-device, accessibility, performance, production-signing, platform-system, recovery, security/privacy, or Stable acceptance evidence.

## Active stabilization observations

- GitHub issue #33 remains the Android representative-device usability/acceptance gate.
- Browser platform conformance remains nonconformant; accepted runtime Manager, Privacy Shield, Wardveil Security, Everkeep, Mesh, Identity, Policy, Observability, and Browser-local Glaze acceptance remain incomplete.
- Downloads and website permissions intentionally remain fail-closed where required authority has not been accepted.
- Canonical full-color and monochrome Browser branding and fail-closed Android launcher provenance checks are integrated.
- Android 15 managed-emulator runtime smoke covers Browser-owned chrome, conservative WebView defaults, and fail-closed free-text Search; physical-device acceptance remains separate.
- The Linux CEF Development path has progressed through Wayland software rendering, pointer/cursor/context-menu/popup/keyboard/clipboard/IME interaction foundations and keyboard tab reordering; representative native-Wayland and production acceptance remain separate.
- Bounded external-drop support is under active Development qualification in a draft branch/PR. File URIs and file contents remain outside the accepted Browser-owned drop contract.

## Maintenance notes

Keep Browser-owned behavior separate from the replaceable rendering-engine dependency. Preserve safe direct navigation, minimized presentation, third-party-cookie restrictions, cleartext/mixed-content protections, TLS fail-closed behavior, and exact-revision build evidence when changing the Android shell.

Do not promote Browser lifecycle state based only on source implementation, a merged pull request, or green CI. Record representative-device findings against the exact application revision and reconcile issue #33, feature records, changelog, and release evidence when those findings change.

## Security stabilization provenance

- Browser exact-source security evidence includes full-history Gitleaks, HIGH/CRITICAL Trivy dependency scanning, CycloneDX SBOM generation, evidence checksums/artifacts, immutable external workflow-action enforcement, and a repository-owned source-security audit.
- Synthetic test fixtures receive only narrow documented handling; broad secret-scanner bypasses are not accepted.
- Android Beta workflow supply-chain tooling/actions are pinned by explicit version or immutable action SHA where required.
- These controls are Development evidence only and do not satisfy representative-device, accessibility, performance, production-signing, runtime platform-system, recovery, Release Candidate, or Stable acceptance.

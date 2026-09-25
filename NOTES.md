# GoreeCloud Browser — Development Notes

## Current stabilization context

- Repository lifecycle remains Development/Beta and is not Stable or production accepted.
- Verified current stabilization base: authoritative `main` at `ae1ee34e317d79c6bcc8998d00f4263f88f6d32f` (merged PR #73).
- Android has an installable GoreeCloud-owned browser shell with direct HTTP(S) navigation, Browser-owned chrome, local start/error/search-blocked surfaces, fail-closed free-text Search delegation, adaptive/round/monochrome launcher-icon resources, and exact-source APK CI.
- Source presence and CI do not replace representative physical-device, accessibility, performance, production-signing, platform-system, recovery, or Stable acceptance evidence.

## Active stabilization observations

- GitHub issue #33 remains the Android usability/device-acceptance gate.
- The original icon-resource and basic-browser-chrome gaps are now source-addressed on current `main`, but launcher rendering and practical usability still require fresh representative-device validation.
- Browser platform conformance remains nonconformant; accepted runtime Manager, Privacy Shield, Wardveil Security, Everkeep, Mesh, Identity, Policy, Observability, and Browser-local Glaze acceptance remain incomplete.
- Downloads and website permissions intentionally remain fail-closed where required authority has not been accepted.

## Maintenance notes

Keep Browser-owned behavior separate from the replaceable rendering-engine dependency. Preserve safe direct navigation, minimized presentation, third-party-cookie restrictions, cleartext/mixed-content protections, TLS fail-closed behavior, and exact-revision build evidence when changing the Android shell.

Do not promote Android Browser lifecycle state based only on source implementation or green CI. Record physical-device findings against the exact application revision and reconcile issue #33 and release evidence when those findings change.


## September 18 security stabilization

- Browser exact-source security evidence is integrated: full-history Gitleaks, HIGH/CRITICAL Trivy dependency scanning, CycloneDX SBOM generation, evidence checksums/artifacts, and immutable external workflow-action enforcement.
- Browser also has a repository-owned exact-source source-security audit for high-confidence committed-secret signatures in the current tree/reachable history and immutable external GitHub Actions references.
- The one synthetic Privacy Shield capability-reference unit-test fixture is handled narrowly; no broad secret-scanner bypass is accepted.
- The Android Beta workflow supply chain is pinned to explicit runner/tool versions and immutable action SHAs. The current source-security and security-evidence lanes are green on their merged revisions.
- These controls are Development evidence only and do not satisfy issue #33 representative-device, accessibility, performance, production-signing, runtime platform-system, recovery, Release Candidate, or Stable acceptance.

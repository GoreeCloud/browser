# GoreeCloud Browser — Development Notes

## Current stabilization context

- Repository lifecycle remains Development/Beta and is not Stable or production accepted.
- Verified baseline for this notes change: `main` at `5c70a7f39a826e841f6f0a537eeea5f16e8415e5`.
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

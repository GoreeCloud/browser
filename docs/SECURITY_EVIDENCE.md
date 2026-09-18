# GoreeCloud Browser — Security Evidence and Supply-Chain Qualification

**Status:** Development control / exact-head validation required  
**Lifecycle impact:** None by itself  
**Stable authority:** Not established by this record

## Purpose

This record defines the repository-local security evidence generated for GoreeCloud Browser during stabilization. It supplements, but does not replace, the GoreeCloud Stable Release Security Blockers and release procedure.

The canonical automated lane is:

`.github/workflows/security-evidence.yml`

## Exact-source controls

For each evaluated pull-request head or `main` revision, the workflow:

- checks out and verifies the exact evaluated source revision;
- fetches full Git history without persisting checkout credentials;
- rejects third-party and reusable workflow `uses:` references that are not full 40-character commit SHAs;
- runs full-history Gitleaks secret scanning with findings redacted;
- verifies the downloaded Gitleaks binary against the recorded SHA-256;
- verifies the Trivy release checksum file against its recorded SHA-256 and then verifies the Trivy archive from that checksum file;
- scans repository-resolvable dependencies for HIGH and CRITICAL known vulnerabilities;
- generates a CycloneDX SBOM;
- records source revision, evidence generation time, scanner versions, and evidence-file SHA-256 values;
- retains the evidence bundle as an exact-revision workflow artifact.

A finding or tooling failure blocks that workflow result. Evidence from an older revision must not be silently transferred to a materially changed candidate.

The repository carries one narrowly scoped Gitleaks false-positive exception for a synthetic `capabilityTokenReference` unit-test fixture. The exception requires the default `generic-api-key` rule, the exact Privacy Shield authorization test path, the capability-reference assignment shape, and an explicit test marker in the fixture value. It does not allowlist the whole test directory, a whole commit, or arbitrary token/key assignments.

## CI supply-chain controls

Browser build workflows use immutable action revisions. The Android beta build additionally pins:

- Eclipse Temurin `17.0.20.1+1`;
- Android command-line-tools build `15859902`;
- Android API 35 and Build Tools `35.0.0`;
- Gradle `8.9`;
- the open-source Gradle basic cache provider;
- the artifact-upload action revision.

Core and extension-signature jobs use `ubuntu-24.04` rather than a moving `ubuntu-latest` label. Build jobs that execute repository source do not retain checkout credentials where that credential is unnecessary.

## Evidence boundary

A green security-evidence workflow is repository-local Development evidence only. It does **not** prove that every Browser dependency or runtime component is reproducible, vulnerability-free, deployed, production-accepted, or Stable.

Separate verification remains required where applicable for:

- Ubuntu/runner image contents and updates;
- APT-resolved build packages such as GTK, X11, curl, compiler, and related transitive dependencies;
- Android SDK platform/package payload provenance beyond the explicitly selected versions;
- Android System WebView / Chromium runtime versions and update state;
- optional CEF / Chromium desktop runtime packages;
- production signing services and key custody;
- release/distribution infrastructure;
- runtime first-party service dependencies and their deployed revisions;
- production artifacts, upgrades, rollback, migration, and deployed-environment security controls.

## Promotion rule

No Release Candidate, production, or Stable claim may rely on this repository workflow alone. The exact proposed candidate must pass all applicable GoreeCloud security, privacy, platform, Glaze UI, functionality, accessibility, recovery, dependency, artifact, signing, deployment, and production-acceptance gates.

---
title: "GoreeCloud Browser — GCEX Signature Scheme v1"
version: "v0.2"
status: "Development"
classification: "Internal"
document_type: "Native Extension Package Signature Implementation Contract"
project: "GoreeCloud Browser"
repository: "GoreeCloud/goreecloud-browser"
canonical_path: "docs/GCEX_SIGNATURE_V1.md"
parent_specification: "docs/NATIVE_EXTENSION_PLATFORM.md"
package_contract: "docs/GCEX_PACKAGE_FORMAT_V1.md"
created: "2026-09-16"
last_updated: "2026-09-16"
---

# GoreeCloud Browser — GCEX Signature Scheme v1

This document defines the first Development cryptographic-signature contract for GoreeCloud Browser `.gcex` packages. It is subordinate to `docs/NATIVE_EXTENSION_PLATFORM.md` and `docs/GCEX_PACKAGE_FORMAT_V1.md`.

The scheme verifies package integrity and possession of the private key corresponding to the package-local public key. It does **not** by itself prove developer identity, extension safety, privacy quality, installation authorization, runtime permission, or production acceptance.

## 1. Cryptographic dependency

The first verifier uses **libsodium 1.0.22** through a narrow GoreeCloud-owned adapter.

- Upstream: `jedisct1/libsodium`
- Upstream release: `1.0.22`
- Upstream annotated tag object: `fe3fa3dd583d9487e1cc32b0f5ef0f516949c836`
- Upstream release commit: `77e1ce5d6dee871c49ef211222ba18ef0c486bda`
- License: ISC
- Browser integration: optional Development verification dependency used only by the standalone signature verification harness in `cmake/extension-signature/`
- Browser does not vendor or fork libsodium source in this milestone.
- Ordinary Browser builds do not acquire a mandatory libsodium dependency from this milestone.

Repository provenance, license notice, and machine-readable dependency metadata are recorded under `third_party/`.

## 2. Scheme identity

The implemented scheme identifier is:

`ed25519-sha256-v1`

It uses:

- SHA-256 for the deterministic GCEX signed-payload digest.
- Ed25519 detached signatures over the resulting 32-byte digest.
- A raw 32-byte Ed25519 public key.
- A raw 64-byte Ed25519 detached signature.

The explicit scheme name prevents this construction from being confused with direct Ed25519 signing of the full package or with Ed25519ph.

## 3. Signature entries

A signed GCEX v1 package contains exactly these signature-namespace files:

- `signatures/ed25519-sha256-v1.public-key`
- `signatures/ed25519-sha256-v1.signature`

The public-key entry is exactly 32 bytes. The signature entry is exactly 64 bytes.

For this Development scheme, any other entry under `signatures/` is rejected by the verifier rather than treated as unbound metadata. Future signature schemes require an explicitly versioned contract.

An unsigned package contains no `signatures/` entries and remains `unsigned_package`.

## 4. Canonical signed payload

GCEX v1 archive order is not used as cryptographic meaning. The verifier derives one deterministic SHA-256 digest from decoded package files.

Signature-namespace files are excluded. Every other package file is covered.

Covered files are sorted by their exact package-relative UTF-8 path byte sequence. The SHA-256 input is then constructed as:

1. Unsigned 16-bit little-endian byte length of the ASCII domain string.
2. ASCII domain string: `GoreeCloud GCEX signature payload v1`.
3. GCEX package-format version byte (`1`).
4. Unsigned 32-bit little-endian number of covered files.
5. For each covered file in sorted path order:
   - unsigned 16-bit little-endian path length;
   - exact path bytes;
   - unsigned 64-bit little-endian content length;
   - exact content bytes.

The final SHA-256 output is 32 bytes and is the message supplied to Ed25519 detached signing or verification.

This binds the manifest, scripts, pages, styles, assets, locales, and every other non-signature file. Adding, removing, renaming, or changing a covered file changes the digest.

## 5. Verification pipeline

Verification operates only after bounded GCEX v1 decoding and semantic manifest/package validation succeed.

The verifier then:

1. Requires the decoded package trust state to still be `unsigned_package`.
2. Detects whether signature material is present.
3. Rejects partial, malformed, unknown, or size-invalid signature material.
4. Recomputes the canonical signed-payload SHA-256 digest.
5. Verifies the detached Ed25519 signature with the package-local public key.
6. Computes a lowercase SHA-256 fingerprint of the raw public key for display/audit correlation.
7. On successful cryptographic verification only, promotes the package trust state from `unsigned_package` to `signed_package`.

Failure leaves package trust as `unsigned_package` and returns a fail-closed verification issue.

## 6. Trust-state boundary

A valid package-local signature establishes only the `signed_package` state:

**Signed**  
The package integrity signature is valid for the package-local public key.

It does not establish:

**Verified Signature**  
A separate trusted developer-key or organizational trust authority must independently establish that the signing key belongs to the claimed developer or approved publisher before `verified_signature` can be produced.

The current verifier therefore never returns or assigns `verified_signature`.

This prevents a package author from becoming “verified” simply by embedding a new key and signing the package with the matching private key.

## 7. Decentralized distribution

The signature scheme does not require:

- A GoreeCloud extension store.
- Mandatory publication.
- Mandatory developer registration.
- A centralized signing service.
- A GoreeCloud-hosted catalog or discovery hub.

Users, developers, and organizations may distribute signed packages independently. Trust in a developer key remains a separate policy/user/organization decision.

## 8. Dependency and provenance controls

The libsodium integration is deliberately isolated behind a GoreeCloud interface so the cryptographic provider remains replaceable.

The dedicated `GoreeCloud Browser Extension Signature` workflow:

1. Checks out the exact Browser candidate revision.
2. Checks out libsodium at exact upstream commit `77e1ce5d6dee871c49ef211222ba18ef0c486bda`.
3. Verifies the repository provenance and ISC license records.
4. Builds that exact libsodium source into a workflow-local prefix.
5. Configures `cmake/extension-signature/` against that prefix.
6. Builds the verifier with strict warnings-as-errors.
7. Runs the CTest signature smoke test.

The integration must not silently float to a newer dependency revision.

A future dependency update must independently review:

- Upstream release/provenance.
- License state.
- Security advisories.
- API/ABI compatibility.
- Browser signature compatibility.
- Build and platform compatibility.
- Required notices and SBOM/dependency metadata.

## 9. Current deterministic coverage

`goreecloud_browser_extension_signature_smoke` covers:

- Unsigned package remains unsigned.
- Deterministic digest generation.
- Valid Ed25519 signature acceptance.
- Promotion only to `signed_package`.
- Public-key fingerprint generation.
- Covered-file tampering rejection.
- Partial signature-material rejection.
- Unknown signature-namespace entry rejection.

The test private key is deterministic test-only material and is zeroed after use. It is not a production signing identity.

## 10. Current build/runtime boundary

This milestone provides a standalone CMake verification harness for the cryptographic adapter and tests. It does **not** yet wire libsodium or signature verification into the ordinary Browser executable, Android application, installer, extension manager runtime, or production build graph.

Runtime integration must be a separate change that preserves the fail-closed trust transition, remains optional where required, keeps the dependency provenance controls above, and receives its own exact-source and representative-platform evidence.

## 11. Open acceptance gates

This Development milestone does not establish:

- Trusted developer identity or key enrollment.
- `verified_signature` production authority.
- Key rotation, revocation, expiry, compromise response, or transparency logs.
- Ordinary Browser runtime integration of the verifier.
- Package installation authorization.
- Update-source trust or update signatures.
- Rollback authorization.
- Extension execution or sandboxing.
- Privileged Extension API dispatch.
- OS-backed key protection.
- Hardware-backed signing.
- Production release signing.
- Representative-device acceptance.
- Release Candidate or Stable qualification.

These remain separate obligations under the Browser roadmap and native-extension specification.

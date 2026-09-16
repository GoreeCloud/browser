---
title: "GoreeCloud Browser — GCEX Package Format v1"
version: "v0.1"
status: "Development"
classification: "Internal"
document_type: "Native Extension Package Implementation Contract"
project: "GoreeCloud Browser"
repository: "GoreeCloud/goreecloud-browser"
canonical_path: "docs/GCEX_PACKAGE_FORMAT_V1.md"
parent_specification: "docs/NATIVE_EXTENSION_PLATFORM.md"
created: "2026-09-16"
last_updated: "2026-09-16"
---

# GoreeCloud Browser — GCEX Package Format v1

This document defines the first bounded **Development** encoding for GoreeCloud Browser `.gcex` packages. It is subordinate to `docs/NATIVE_EXTENSION_PLATFORM.md` and exists so the source reader, tests, future packaging tools, and independent implementations have one explicit byte-level contract.

This contract does **not** enable extension installation or execution. It does not establish package signatures, developer identity, trust, sandboxing, privileged Extension API dispatch, update authority, or production acceptance.

## 1. Design goals

GCEX v1 is intentionally small, deterministic to parse, dependency-light, and fail closed. The first reader must be able to reject malformed or resource-abusive packages before an extension runtime exists.

Version 1 therefore uses a GoreeCloud-owned binary container with raw entry payloads. It is not ZIP, TAR, CRX, XPI, or a compatibility wrapper around another browser-extension package format.

Compression and encryption are not defined in GCEX v1. A future package-format version may add bounded compression or other features only with explicit versioning and corresponding resource-abuse protections.

## 2. Package identity

A packaged extension uses the `.gcex` suffix.

The file begins with this fixed 12-byte header:

| Offset | Size | Field | Encoding |
| ---: | ---: | --- | --- |
| 0 | 4 | Magic | ASCII `GCEX` |
| 4 | 1 | Package-format version | Unsigned byte; v1 is `1` |
| 5 | 3 | Reserved | Must be zero in v1 |
| 8 | 4 | Entry count | Unsigned 32-bit little-endian |

Unknown package-format versions and non-zero reserved bytes are rejected by the v1 reader.

## 3. Entry encoding

Immediately after the package header, exactly `entry_count` entries appear. Each entry is encoded as:

| Field | Size | Encoding |
| --- | ---: | --- |
| Path length | 2 bytes | Unsigned 16-bit little-endian |
| Content length | 8 bytes | Unsigned 64-bit little-endian |
| Path | `path_length` bytes | UTF-8 package-relative path |
| Content | `content_length` bytes | Raw payload |

No bytes may remain after the declared final entry. Trailing bytes are rejected.

Directory records are unnecessary in v1. Entries represent files. Paths ending in `/` are rejected by the source reader.

## 4. Resource bounds

The initial Development reader enforces these hard v1 limits:

| Limit | Maximum |
| --- | ---: |
| Package file size | 64 MiB |
| Entry count | 2,048 |
| One entry payload | 16 MiB |
| Sum of entry payloads | 64 MiB |
| Manifest payload | 256 KiB |
| Manifest line | 16 KiB |
| Package path | 1,024 bytes |

These are security/resource-governance limits for package-format v1, not product marketing limits. Increasing them requires an explicit source and documentation change with renewed abuse/resource validation.

## 5. Package path rules

Every entry path must be valid UTF-8 and must also satisfy the Browser native-extension package-path contract.

The v1 reader rejects at least:

- Empty paths.
- Absolute paths.
- Backslashes.
- Drive/scheme-like `:` characters.
- Control or whitespace characters in package paths.
- Empty path segments.
- `.` or `..` path segments.
- Paths longer than the bound above.
- Paths ending in `/`.
- Duplicate paths.

These rules are intended to prevent traversal and ambiguous extraction semantics before any installer exists. Reading a package never authorizes writing package entries to arbitrary filesystem locations.

## 6. Required manifest entry

Every GCEX v1 package contains exactly one file at the package-relative path:

`manifest`

The archive reader requires this entry before combined package decoding can succeed. The existing native package validator also requires manifest-declared entry points to exist in the package inventory.

## 7. Native manifest text encoding

The `manifest` payload is UTF-8 text using a GoreeCloud-native line-oriented schema.

Each non-empty line has this form:

`key=value`

LF and CRLF line endings are accepted. The first `=` separates key from value, so later `=` characters remain part of the value. Comments and implicit type coercion are not defined in v1.

Manifest keys use lowercase ASCII letters and underscores. Unknown keys fail closed. Duplicate scalar keys fail closed. Control characters inside values fail closed.

### Required scalar keys

- `manifest_version`
- `api_version`
- `id`
- `name`
- `version`

### Optional scalar keys currently decoded

- `description`
- `author`
- `homepage`
- `minimum_browser_version`

### Repeated list keys

- `entry_point`
- `permission`
- `allowed_website`

Repeated list values are subsequently passed through the existing semantic manifest validator, which rejects duplicate entry points, unknown or duplicate permissions, invalid/duplicate site declarations, unsupported manifest/API versions, missing entry points, invalid IDs/versions, and entry points absent from the package inventory.

A minimal manifest therefore resembles:

```text
manifest_version=1
api_version=1
id=org.example.reader-helper
name=Reader Helper
version=1.0.0
entry_point=scripts/background.js
permission=page.read
allowed_website=https://example.org
```

## 8. Validation pipeline

The bounded source pipeline is deliberately layered:

1. Read the `.gcex` file under the package-size limit.
2. Validate GCEX magic, package-format version, reserved header, entry count, entry lengths, resource limits, UTF-8 paths, path safety, duplicate paths, manifest presence, and exact end-of-file.
3. Decode the `manifest` payload under manifest/line limits and strict key rules.
4. Apply the existing semantic native-manifest validator.
5. Apply the existing package-inventory/entry-point validator.

Only a package that passes every layer is returned as an accepted decoded package.

## 9. Trust and execution boundary

Successful GCEX decoding proves only that bytes conform to the bounded package and manifest contracts checked by the current source.

It does **not** mean:

- The extension is safe.
- The developer is known or trusted.
- A signature is valid.
- The package is authorized for installation.
- Requested permissions are granted.
- Extension code may execute.
- Browser APIs are available.
- The package is production-approved.

The reader accepts a caller-supplied trust-state data value only as metadata. Cryptographic signature verification and provenance remain a later independent milestone.

## 10. Current verification target

`goreecloud_browser_extension_package_smoke` is the deterministic source test for this contract. It covers an accepted in-memory package, a real temporary on-disk `.gcex` read, suffix rejection, invalid magic, traversal rejection, duplicate entries, missing manifest, unknown manifest keys, duplicate scalar keys, missing required manifest keys, unsupported manifest versions, unknown permissions, and oversized declared entries.

Broader fuzzing, installer behavior, signature verification, process isolation, runtime execution, update/rollback, representative-device evidence, production acceptance, and Stable qualification remain separate open gates.

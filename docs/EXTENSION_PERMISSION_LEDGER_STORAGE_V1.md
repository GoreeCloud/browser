---
title: "GoreeCloud Browser — Extension Permission Ledger Storage v1"
version: "v0.1"
status: "Development"
classification: "Internal"
document_type: "Native Extension Permission Persistence Implementation Contract"
project: "GoreeCloud Browser"
repository: "GoreeCloud/goreecloud-browser"
canonical_path: "docs/EXTENSION_PERMISSION_LEDGER_STORAGE_V1.md"
parent_specification: "docs/NATIVE_EXTENSION_PLATFORM.md"
created: "2026-09-16"
last_updated: "2026-09-16"
---

# GoreeCloud Browser — Extension Permission Ledger Storage v1

This document defines the first bounded Development persistence and recovery contract for Browser-owned native-extension permission leases. It is subordinate to `docs/NATIVE_EXTENSION_PLATFORM.md` and the in-memory authorization contract in `include/goreecloud/browser/extension_permission_ledger.hpp`.

This storage primitive does **not** install or execute extensions, grant undeclared capabilities, validate package signatures, establish developer identity, create extension-process authority, or establish production acceptance.

## 1. Persistence purpose

The Browser needs to preserve user-approved permission state that is intentionally durable while preventing restart or crash recovery from extending temporary authorization beyond its declared lifetime.

The governing rule is:

**Recovery may restore only authorization that was already durable and still valid. Recovery must never manufacture, broaden, or resurrect authorization.**

## 2. Durable and volatile grant lifetimes

Storage v1 persists only active, non-revoked, non-consumed grants with these lifetimes:

- `always`
- `one_hour`, only while the exact stored expiration remains in the future

The following volatile lifetimes are intentionally **never serialized**:

- `once`
- `until_tab_closes`
- `until_website_closes`
- `until_browser_closes`

Tab identifiers and browser-session identifiers are therefore not part of the durable snapshot format. Restarting the Browser cannot revive those volatile grants from disk.

A durable record may retain the explicit `private_browsing_allowed` user preference attached to the grant, but it does not persist private tabs, private navigation state, private session identity, or private page data.

## 3. Profile isolation

Every snapshot is bound to exactly one Browser profile identifier.

Restore requires an expected profile identifier. A snapshot whose embedded profile does not exactly match the requested profile fails closed with `profile_mismatch`.

Leases belonging to another profile are not serialized into the target profile snapshot.

## 4. Snapshot format and bounds

Storage v1 uses a Browser-owned binary snapshot with:

- Magic: `GCPL`
- Snapshot version: `1`
- Three reserved zero bytes
- Snapshot timestamp in milliseconds
- Profile identifier
- Lease count
- Bounded lease records
- CRC32 checksum over the preceding snapshot payload

Hard Development limits include:

- Maximum snapshot size: 4 MiB
- Maximum persisted leases: 4,096
- Maximum profile identifier length: 256 bytes
- Maximum website declarations per lease: 256
- Maximum website declaration length: 2,048 bytes

Unknown snapshot versions, non-zero reserved bytes, malformed or truncated fields, duplicate lease IDs, invalid enum codes, unknown flag bits, invalid leases, invalid or duplicate websites, profile mismatch, trailing bytes, and checksum mismatch fail closed.

## 5. Stored lease fields

Each persisted lease records only the authorization state needed to reconstruct an eligible durable lease:

- Lease ID
- Permission identity
- Grant scope
- Durable lifetime identity
- Private-browsing opt-in flag
- Issue timestamp
- Expiration timestamp
- Website declarations

Profile identity is stored once at snapshot level and applied to every reconstructed lease after exact profile matching.

`revoked`, `consumed`, inactive, cross-profile, and volatile grants are not serialized as active durable grants.

## 6. Time and expiration semantics

A one-hour lease must retain the exact one-hour relationship already required by the in-memory permission ledger.

During snapshot creation:

- Already-expired one-hour leases are omitted.
- A clock value earlier than the lease issue timestamp fails closed rather than extending the lease.

During restore:

- A restored one-hour lease whose expiration has been reached is dropped and counted as expired.
- A restore clock earlier than the stored issue timestamp fails closed with `clock_regression`.
- Recovery never gives a one-hour lease a fresh one-hour window.

## 7. Corruption detection boundary

Storage v1 uses CRC32 to detect accidental snapshot corruption and incomplete or unintended byte changes.

CRC32 is **not** a cryptographic authenticity mechanism and must not be represented as tamper resistance, package signing, user authentication, or trust evidence. An attacker capable of modifying the snapshot and recomputing its checksum is outside what CRC32 can prove.

Stronger authenticated-at-rest storage may be added later under the appropriate Browser/profile/key-management authority. It must remain separate from extension package-signature trust.

## 8. Save and recovery behavior

The Development file helper uses a primary snapshot plus temporary and backup paths:

1. Encode a complete bounded snapshot.
2. Write the new bytes to a temporary sibling path.
3. Preserve the previous primary as a backup only when the previous primary can be decoded as a valid snapshot for the same profile and current recovery context.
4. Promote the temporary file to the primary path.
5. If the primary cannot be restored later, attempt the previous valid backup.

Auxiliary paths are derived with native `std::filesystem::path` operations so path encoding is not round-tripped through a narrow string representation.

If neither a primary nor backup snapshot exists, loading returns an empty valid ledger. First-run absence is not treated as corruption and does not create any authorization.

If snapshot files exist but no valid primary or backup can be restored, loading fails closed.

## 9. Recovery limitations

The current helper provides bounded primary/backup recovery semantics but does not yet claim production-grade crash consistency across every operating system or storage failure mode.

In particular, this Development milestone does not claim:

- Filesystem or directory `fsync` durability guarantees.
- Atomic replacement semantics proven across all supported filesystems.
- Cryptographic authentication of stored permission state.
- Multi-process writer coordination.
- Durable integration with a production profile database.
- OS-backed protected storage.
- Device-level fault-injection acceptance.
- Production migration or rollback guarantees.

Those remain independent acceptance obligations before production use.

## 10. Current deterministic coverage

`goreecloud_browser_extension_permission_ledger_storage_smoke` covers:

- Durable `always` and still-valid `one_hour` round trips.
- Exclusion of `once` and tab-lifetime grants.
- Cross-profile exclusion.
- Preservation of explicit private-browsing opt-in on an eligible durable grant.
- Exact profile mismatch rejection.
- Checksum corruption rejection.
- Expired one-hour lease removal during restore.
- Clock-regression rejection.
- First-run missing-snapshot recovery as an empty valid ledger.
- Primary save/load.
- Previous-valid-snapshot backup creation.
- Backup recovery after deliberate primary corruption.

Broader fuzzing, authenticated-at-rest storage, power-loss testing, multi-process coordination, runtime integration, package signing, process isolation, representative-device evidence, production acceptance, and Stable qualification remain open.

## 11. Authority boundary

Permission persistence remains subordinate to all of the following:

- The extension manifest must declare the requested permission.
- The Browser-owned permission model must authorize the exact profile, site or object scope, privacy context, lifetime, and operation.
- A restored lease is only candidate authorization state; runtime use must still pass the normal authorization path.
- Package trust and developer identity remain separate from permission state.
- Extension execution remains disabled until its own runtime and security gates are independently implemented and accepted.

Persisting a permission lease never means an extension is safe, trusted, installed, running, or production-approved.

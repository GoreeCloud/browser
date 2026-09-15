---
title: "GoreeCloud Browser Sync — Phase 0 Runtime Contract"
document_owner: "LaDamian Goree"
version: "v0.1"
status: "Proposed / implementation contract"
classification: "Internal"
document_type: "Browser Sync Phase 0 Implementation Contract"
project: "GoreeCloud Browser"
repository: "GoreeCloud/goreecloud-browser"
authoritative_parent: "SPECIFICATIONS.md §11"
last_updated: "2026-09-15"
---

# GoreeCloud Browser Sync — Phase 0 Runtime Contract

## 1. Purpose and Status

This document turns the Browser Sync Phase 0 obligations already defined by `SPECIFICATIONS.md` §11 into an implementation-ready Browser-side contract.

It does **not** establish an accepted Browser Sync runtime, production authorization, production encryption, production transport, Release Candidate status, production approval, or Stable qualification.

The contract is intentionally limited to the Browser side of the shared synchronization boundary. GoreeCloud Sync remains the shared synchronization/state-coordination substrate. GoreeCloud Browser remains the authority for Browser semantics, local Browser state, privacy-context eligibility, payload meaning, Browser user controls, and safe application of synchronized Browser records.

## 2. Verified Shared-Sync Baseline

This contract was reconciled on September 15, 2026 against the current `GoreeCloud/goreecloud-sync` source and specification available to the Browser project.

The verified shared Sync foundations relevant to Browser include:

- first-party dataset capability negotiation;
- transport-neutral application record envelopes;
- deterministic record conflict selection;
- payload-free tombstones;
- Browser-specific persistence handlers for `browser.tabs` and `browser.history`;
- Privacy Shield and Wardveil acceptance evidence at the Sync record-acceptance boundary;
- persistent replay/high-water foundations;
- observation receipts and tombstone-convergence foundations;
- bounded authenticated retrieval ordered by record ID;
- authenticated peer/device trust foundations and explicit current-trust revalidation primitives.

The current shared dataset registry defines these Browser-owned capabilities at schema version 1:

| Dataset | Application authority | Read | Write | Delete |
| --- | --- | ---: | ---: | ---: |
| `browser.tabs` | Browser | Yes | Yes | Yes |
| `browser.history` | Browser | Yes | Yes | Yes |
| `browser.preferences` | Browser | Yes | Yes | No |

The same registry defines `bookmarks.items`, `bookmarks.collections`, and `bookmarks.assignments` under the separate `bookmarks` application authority. Browser must not invent a competing `browser.bookmarks` dataset merely because bookmarks are visible in the Browser UI.

This baseline is a dependency snapshot, not a permanent wire-protocol promise. Before implementation or release, Browser must revalidate the current Sync capability registry and accepted Sync interface rather than assuming this document froze Sync forever.

## 3. Governing Boundary

The Phase 0 rule is:

> **Browser owns the meaning of Browser data; Sync owns the shared replication substrate.**

Browser owns:

- which local Browser state is eligible for synchronization;
- Browser record identity strategy;
- Browser payload schemas and field semantics;
- profile and privacy-context scoping;
- Browser-local revisions before projection;
- local change observation and durable outbound intent;
- Browser-specific validation before projection and after remote delivery;
- local application transactions;
- user category controls and Browser sync UX;
- retention behavior that belongs to Browser;
- Browser-specific conflict semantics when the generic Sync winner is insufficient to produce a safe application result;
- mapping between local Browser state and shared Sync record envelopes.

GoreeCloud Sync owns:

- canonical first-party dataset identifiers in the shared registry;
- capability negotiation and negotiated schema version;
- shared `RecordEnvelope` transport shape;
- generic record validation against negotiated capability;
- authenticated peer/session binding;
- record-acceptance enforcement at the shared Sync boundary;
- generic replay/high-water controls;
- generic deterministic record conflict selection;
- payload-free tombstones;
- observation receipts and shared tombstone-convergence foundations;
- bounded retrieval/paging and transport-independent replication mechanics;
- shared transport, retry, peer trust, and synchronization infrastructure as those facilities mature.

Browser must not fork these shared mechanics into an incompatible Browser-only wire protocol.

## 4. Shared Record Envelope Contract

Browser Phase 0 must project eligible application objects into the shared Sync record-envelope model rather than defining a second transport envelope.

The verified shared envelope currently contains:

```text
dataset
schemaVersion
recordId
revision
updatedAt
originDevice
deleted
payload
```

Browser must treat those fields as Sync transport/state-coordination fields, not as a substitute for Browser application semantics.

### 4.1 Record identity

`recordId` must be stable for the lifetime of one logical synchronized Browser object and remain within the current shared bound. Browser must not use transient engine handles, Android activity IDs, process IDs, WebView object identity, array positions, or ephemeral memory addresses as synchronized record identity.

Where a synchronized Browser object already has a stable Browser-owned ID, such as a logical `tab_id`, the adapter should derive or directly use a deterministic bounded representation of that ID.

### 4.2 Revision

The shared envelope requires a non-zero monotonically meaningful `revision` for one record identity. Browser must define how its local application transaction advances that revision and must not increment it merely because a record was retransmitted.

A retry of the same committed logical change should preserve the same logical record revision where practical so duplicate transport attempts remain distinguishable from a new application mutation.

### 4.3 Updated time

`updatedAt` is part of the shared deterministic conflict ordering but must not become Browser's sole ordering or authorization source. Browser clocks can skew. Higher-level Browser invariants must not depend only on wall-clock time when stable revision/application sequencing is available.

### 4.4 Origin device

`originDevice` identifies the device that produced the envelope revision. It does not grant that device authority by itself. Sync acceptance still requires authenticated peer identity and applicable current trust/authorization evidence.

### 4.5 Tombstones

Deleted Sync records carry no application payload. Browser must therefore persist enough local deletion meaning outside the removed payload to safely apply and converge deletion.

A Browser deletion must never require retaining the deleted private payload solely to communicate that deletion.

## 5. Capability Negotiation

Browser must begin a Sync relationship by consuming the capabilities actually negotiated by GoreeCloud Sync.

The current shared negotiation model intersects the local and remote capability set by dataset and uses the lower mutually understood positive schema version. Read/write/delete rights are intersected rather than expanded.

Browser rules:

1. Do not send a dataset that was not negotiated.
2. Do not parse an incoming Browser dataset at a schema version Browser does not support.
3. Do not treat recognition of a dataset name as authority to read, write, or delete it.
4. Do not infer delete support for `browser.preferences`; the current shared registry does not advertise deletion for that dataset.
5. Do not silently alias a foreign or future dataset name to a Browser dataset.
6. Do not downgrade security, privacy, profile isolation, or payload meaning merely to reach an older schema.
7. If a mutually safe schema does not exist, disable that dataset for the peer and expose the bounded incompatibility to Browser sync status.

## 6. Phase 0 Dataset Scope

Phase 0 is deliberately bounded.

### 6.1 `browser.tabs`

This dataset is the initial Browser live-state replication surface for explicitly Sync-eligible **Normal-context** tab projections.

The Browser payload schema should remain bounded to logical state needed for cross-device tab continuity. It may include versioned fields such as:

- stable Browser `tab_id` or a dataset record ID derived from it;
- owning Browser profile namespace/reference suitable for Sync scoping;
- bounded normalized URL required for tab continuity;
- user-visible title where allowed;
- pinned state where supported;
- Webspace reference only when that Webspace is itself eligible and the receiving profile can resolve it safely;
- last meaningful Browser commit/update marker needed by the Browser application;
- bounded presentation metadata needed for continuity.

It must not include:

- cookies;
- authorization headers;
- reusable credentials;
- Vault secrets or secure-autofill payloads;
- private form contents;
- raw engine memory/session blobs;
- Private or Isolated Private tabs;
- local permission capability handles;
- Privacy Shield or Wardveil reusable authority material.

### 6.2 `browser.history`

This dataset is the initial Browser history replication surface for explicitly eligible Normal-context history records.

The Browser payload schema should include only the minimum fields required to represent the accepted Browser history model, such as a stable Browser history identity, bounded URL/origin representation, visit time or Browser ordering data, and user-visible title where permitted.

Private and Isolated Private history must never be projected into `browser.history`.

History retention remains subject to Browser and Privacy Shield rules. Sync availability must not convert a locally expired or user-deleted history record into indefinite retention.

### 6.3 `browser.preferences`

The shared registry currently advertises Browser preferences at schema version 1 with read/write and no delete capability.

Browser Phase 0 may prepare a preferences adapter, but it must not enable production preference synchronization until the Browser defines:

- which preferences are portable;
- which preferences are device-local;
- which preferences are profile-scoped;
- which preferences are policy-controlled;
- how unknown/future fields are preserved or rejected;
- how a reset/default is represented without relying on an unsupported delete operation;
- how unsafe device-specific values are excluded.

### 6.4 Bookmarks

Bookmarks shown in Browser are **not** a Browser-owned shared Sync dataset under the verified registry. The current shared namespace belongs to the `bookmarks` application:

- `bookmarks.items`;
- `bookmarks.collections`;
- `bookmarks.assignments`.

Browser must integrate with GoreeCloud Bookmarks through its accepted first-party boundary. Browser Sync must not create duplicate bookmark authority or a second bookmark conflict model.

### 6.5 Deferred Browser domains

Sessions, windows, Reading List, site permissions, extension state, Webspace definitions, download metadata, and other Browser domains remain outside Phase 0 unless and until a canonical dataset and application contract is accepted in the shared Sync registry or another approved first-party authority owns that state.

Phase 0 must not mint speculative wire identifiers for those domains.

## 7. Browser-Side Local Change Journal

Browser Phase 0 requires a durable Browser-owned outbound synchronization journal distinct from the Browser runtime/session journal defined for local crash restoration.

The sync journal records committed synchronization intent, not arbitrary engine events.

Each entry should bind at least:

- local journal sequence/operation ID;
- Browser profile namespace;
- dataset;
- record ID;
- record revision;
- operation type such as upsert or delete;
- committed local state/version reference;
- eligibility/privacy classification;
- creation time;
- current delivery state;
- retry/acknowledgement bookkeeping that does not contain secret authority material.

The journal must be append-oriented or transactionally equivalent and restart-safe.

### 7.1 Required delivery states

A Browser-owned outbound operation should distinguish states equivalent to:

```text
PENDING
READY
SUBMITTING
SUBMITTED
OBSERVED_OR_ACKNOWLEDGED
COMPACTABLE
BLOCKED_AUTHORIZATION
BLOCKED_CAPABILITY
BLOCKED_SCHEMA
RETRYABLE_ERROR
TERMINAL_ERROR
CANCELLED_BY_LOCAL_STATE
```

Transport retry must not become a new Browser mutation.

### 7.2 Local commit ordering

Browser must not enqueue a Sync upsert before the corresponding Browser application transaction is committed.

For deletion, Browser must commit the Browser deletion/tombstone intent locally before or atomically with enqueueing the Sync tombstone. This prevents a crash from publishing a deletion that Browser itself did not commit or from committing a local deletion without retaining enough intent to converge it later.

## 8. Outbound Projection Pipeline

The outbound Browser Phase 0 flow should be equivalent to:

```text
Committed Browser state
  → eligibility classifier
  → dataset-specific projection builder
  → Browser payload-schema validator
  → Browser sync journal
  → negotiated-capability check
  → shared RecordEnvelope construction
  → Privacy Shield authorization acquisition/reference
  → authenticated Sync submission
  → shared acceptance / replication
  → observation/acknowledgement tracking
  → bounded compaction
```

No step may convert a failed privacy, trust, capability, schema, or profile check into a successful send.

### 8.1 Eligibility must occur before projection

Private and Isolated Private data must be rejected at the Browser source classification boundary before a Sync envelope is constructed.

Filtering private data after serialization is insufficient because serialization itself can create durable/loggable copies of data that should never enter ordinary Sync.

### 8.2 Minimal payload

Projection builders must omit fields not required by the receiving Browser use case. Browser should prefer purpose-limited logical state over full local database rows or engine serialization.

## 9. Inbound Apply Pipeline

The inbound Browser flow must keep shared Sync acceptance separate from Browser application acceptance.

A record accepted by Sync means that the shared Sync boundary accepted its negotiated dataset/schema, peer/trust evidence, and required privacy decision. It does **not** automatically mean the Browser can safely apply the payload to its current local state.

The Browser inbound path should be equivalent to:

```text
Accepted shared Sync record
  → Browser dataset dispatch
  → Browser payload-schema validation
  → profile namespace / local mapping validation
  → privacy-context exclusion check
  → local invariant check
  → stage application mutation
  → deterministic Browser transaction
  → commit local Browser state
  → record local observation/application result
```

Browser must fail closed or quarantine the record for review/diagnostics when the Browser payload is malformed, references an unknown/deleted profile, attempts a forbidden privacy-context transition, violates a Browser invariant, or cannot be safely migrated.

## 10. Shared Acceptance and Browser Application Authorization

The verified shared Sync acceptance path currently requires:

- the record dataset to be present in the authenticated peer's negotiated capabilities;
- the record to validate against that negotiated capability;
- an explicit Privacy Shield decision with non-empty purpose, allowed purpose, granted consent, and decision time;
- explicit Wardveil trust evidence bound to the authenticated peer device ID and key fingerprint;
- accepted evidence recorded without embedding the synchronized application payload.

Browser must consume this boundary rather than recreating a weaker local substitute.

Browser-specific application checks remain additional. Shared acceptance can never authorize Browser to:

- bind a record to the wrong profile;
- import Private/Isolated Private state;
- install or execute code;
- persist a credential;
- override a deleted profile;
- weaken a local permission/privacy decision;
- write into a Vault authority boundary;
- bypass a Browser schema/invariant check.

## 11. Conflict Semantics

The current shared generic record resolver deterministically prefers:

1. higher record revision;
2. then newer `updatedAt`;
3. then tombstone over non-tombstone;
4. then lexicographically greater origin device as a stable final tie-breaker;
5. identical ordering inputs are treated as converged.

Browser must understand this ordering because it affects which shared envelope reaches persistence.

However, the generic winner is not permission to discard Browser-domain invariants. If a domain requires structured merge rather than whole-record replacement, Browser must define that merge at the Browser payload/object layer or work with GoreeCloud Sync to introduce an accepted shared contract. Browser must not silently bolt an incompatible merge algorithm onto the same record identity and pretend the peers will converge.

Phase 0 should keep record granularity small enough that deterministic whole-record resolution is safe for the selected datasets.

## 12. Replay, Freshness, and Idempotency

The shared Sync replay foundation tracks a high-water revision per authenticated peer device, dataset, and record ID and rejects an already-observed or older revision.

Browser Phase 0 must complement that behavior:

- local operation IDs must be unique and restart-safe;
- retries of one operation must be idempotent;
- a remote duplicate must not create duplicate Browser objects;
- an older remote revision must not roll Browser state backward;
- stale callbacks arriving after profile deletion, Close & Forget, or local tombstone application must be rejected;
- Browser must not infer global freshness merely from one peer's high-water mark;
- authorization/trust freshness remains separately governed and must be revalidated where required by the current Sync/Identity/Wardveil contract.

## 13. Deletion and Tombstone Convergence

Browser deletions eligible for Sync must use the shared payload-free tombstone mechanism when the negotiated capability supports delete.

The Browser must retain enough local metadata to know that a synchronized object was intentionally deleted and to avoid stale resurrection until the applicable convergence/retention policy permits compaction.

The shared observation-receipt model can record that authenticated peers observed a specific record revision without including application payload. Shared tombstone convergence can be confirmed when the required peer set has exact receipts for the tombstone revision/digest.

Browser Phase 0 should use that facility where exposed by the accepted Sync adapter rather than inventing a second acknowledgement protocol.

Compaction rules must distinguish:

- local journal acknowledgement;
- peer observation;
- tombstone convergence;
- policy retention expiry;
- device revocation/retirement;
- user-requested deletion completion.

Those states are related but are not interchangeable.

## 14. Retrieval and Cursor Contract

The current shared persistence layer supports bounded record retrieval ordered by `recordId` with an exclusive cursor and validates the entire persisted store before returning a page so corruption outside the requested page fails closed.

Browser must treat a retrieval cursor as Sync transport progress, not as Browser semantic ordering.

Rules:

- cursors must not be used as record identity;
- a missing cursor must not imply a full logical reset unless the accepted Sync API defines that behavior;
- page replay must be idempotent;
- Browser must commit applied progress only after the corresponding local application transaction succeeds;
- interruption between pages must resume without duplicating Browser objects or skipping records;
- a corrupt/invalid page must stop application rather than partially applying unverified trailing records.

## 15. Profile, Account, and Device Namespace

Browser Sync Phase 0 must make namespace boundaries explicit.

At minimum, the local adapter must bind every synchronized Browser record flow to:

- the authenticated GoreeCloud account/user context where applicable;
- the Browser profile;
- the local device identity;
- the remote authenticated device identity supplied through Sync;
- the negotiated dataset;
- the Browser privacy context eligibility classification.

A valid Sync record for account/profile A must never be applied to profile B merely because the record ID collides.

Profile deletion or account sign-out must pause/terminate applicable Browser Sync flows and invalidate stale local adapter work. Resuming requires fresh valid local authority and current Sync/Identity trust state.

## 16. Privacy Context Exclusion

Phase 0 has a non-negotiable invariant:

> **Private and Isolated Private Browser state is never ordinary Browser Sync input.**

This includes tabs, history, private permission/session material, private source URLs, private Webspace/container state, private downloads still tied to private runtime metadata, and any derived payload that would reveal private activity.

Close & Forget must invalidate queued ordinary Sync work derived from the destroyed private context. Ideally such work never exists because exclusion occurs before projection; defensive cancellation still remains required for race safety.

Normal state that was derived from an explicit user action in a private context, such as intentionally retaining a released downloaded file, must be evaluated under the destination product's own authority and must not carry private Browser history into Sync by implication.

## 17. Encryption and Secret Boundaries

Phase 0 must prepare for cryptographic separation without inventing Browser-local cryptography that conflicts with Sync, Identity, or Vault.

Browser rules:

- no reusable credential or Vault payload in ordinary Browser Sync records;
- no private keys or bearer secrets in the Browser sync journal;
- no Sync transport key material persisted in Browser application payloads;
- Browser may store only opaque, bounded adapter references required by the accepted Sync/Identity interface;
- any future end-to-end encryption domain must be versioned and jointly aligned with the accepted shared Sync and Identity/Vault authority boundaries;
- inability to establish the required protected Sync path must fail closed for that dataset rather than fall back to an unprotected transport.

## 18. Browser Sync Adapter Surface

The Browser should expose a platform-neutral adapter boundary equivalent to:

```text
BrowserSyncAdapter
  capabilities(profile)
  start(profile)
  pause(profile, reason)
  submit(operation)
  retrieve(dataset, cursor, limit)
  record_observation(record)
  revoke_device(device)
  status(profile)
  close_profile(profile)
```

The exact accepted Sync API may differ. The Browser adapter should translate Browser requirements onto the real Sync interface rather than forcing Sync to mirror these method names.

The adapter must not manufacture successful authority states. It should surface typed results/failures such as:

- capability unavailable;
- schema incompatible;
- authentication required;
- device untrusted/revoked;
- Privacy Shield authorization denied/expired/unavailable;
- Wardveil trust denied/unavailable;
- transport unavailable;
- retryable remote error;
- storage/persistence error;
- malformed remote record;
- local Browser application rejection.

## 19. State Model and Status UX

Browser Sync user state should distinguish at least:

```text
DISABLED
ENROLLMENT_OR_AUTH_REQUIRED
INITIALIZING
SYNCING
UP_TO_DATE
OFFLINE_WITH_PENDING_CHANGES
PAUSED_BY_USER
PAUSED_BY_POLICY
BLOCKED_AUTHORIZATION
BLOCKED_TRUST
BLOCKED_SCHEMA
RECOVERY_REQUIRED
ERROR
```

A green or success state must not be shown solely because the network connection exists.

Browser should expose, without leaking sensitive payloads:

- enabled categories;
- participating trusted devices;
- last successful local/remote synchronization time where meaningful;
- pending operation count or bounded status;
- paused/blocked reason;
- schema/capability incompatibility;
- device revocation state;
- recovery or reconciliation requirement;
- whether a category is unavailable because its authority is not accepted.

## 20. Diagnostics and Evidence

Diagnostics may include:

- opaque operation IDs;
- dataset and schema version;
- high-level journal state;
- bounded retry count;
- cursor/page status;
- local application result category;
- Sync acceptance/result category;
- device ID only where operationally required and permitted;
- privacy/trust evidence identifiers only when they are non-secret opaque references approved for logging;
- timing and resource measurements required for reliability validation.

Diagnostics must not include raw private history, cookies, authorization headers, Vault material, user-entered form contents, private tabs, reusable Privacy Shield capabilities, private keys, or full sensitive synchronized payloads.

## 21. Failure and Recovery Semantics

Phase 0 must survive:

- Browser process death before enqueue;
- process death after enqueue but before submission;
- process death during submission;
- lost acknowledgement/receipt;
- duplicate delivery;
- out-of-order delivery;
- network loss;
- peer restart;
- local device restart;
- corrupt local journal;
- incompatible schema;
- revoked device;
- expired/denied privacy authorization;
- Sync persistence corruption/error;
- local Browser database transaction failure;
- profile deletion/sign-out while work is pending;
- long-offline device return.

Recovery must prefer deterministic retry or explicit reconciliation over silent full-state overwrite.

A corrupt local journal or irreconcilable state must not be silently discarded if doing so could resurrect deleted data or omit pending deletion. Browser should enter a bounded recovery-required state and preserve privacy-safe diagnostic evidence.

## 22. Phase 0 Acceptance Matrix

Phase 0 is implementation-accepted only when exact-revision automated/fault tests and applicable representative-device evidence prove at least:

1. Browser advertises/uses only Sync datasets actually negotiated for Browser.
2. `browser.tabs`, `browser.history`, and `browser.preferences` schema handling matches the accepted shared registry.
3. Browser does not create a competing `browser.bookmarks` namespace.
4. Unknown datasets fail closed.
5. Unsupported schema versions fail closed without payload application.
6. Private and Isolated Private tabs/history never enter projection, journal, envelope, diagnostics, or ordinary Sync storage.
7. Stable record IDs survive restart and do not depend on engine instance identity.
8. Local retries preserve idempotency and do not create new logical revisions merely because transport retried.
9. Shared Sync acceptance denial prevents Browser application.
10. Browser local invariant rejection prevents local application even after shared Sync acceptance.
11. Cross-profile/account record application is rejected.
12. Revoked/untrusted peer state cannot continue accepted record application at required trust checkpoints.
13. Privacy Shield denied/unavailable/invalid decision fails closed where authorization is required.
14. Wardveil trust denial/mismatch fails closed where shared acceptance requires it.
15. Replay/older revision does not roll back local Browser state.
16. Duplicate incoming delivery does not duplicate tabs/history/preferences.
17. Generic conflict ordering is deterministic for the same shared record inputs.
18. Tombstones contain no application payload and stale records cannot resurrect a converged deletion.
19. Observation/acknowledgement loss is safely retryable.
20. Cursor/page interruption resumes without skipping or duplicating Browser records.
21. Store/page corruption stops application rather than returning partially trusted state.
22. Browser process death across each journal/submission/apply boundary restores a consistent state.
23. Profile deletion/sign-out cancels or blocks stale queued work and prevents silent recreation.
24. Preferences reset semantics do not rely on unsupported delete capability.
25. Sync unavailability leaves local Browser operation functional and records bounded pending state rather than corrupting local state.
26. Pending journal size, retry behavior, retrieval limits, and backoff remain bounded under sustained offline operation.
27. Diagnostics reconstruct failures without protected payloads or reusable authority material.
28. User status truthfully distinguishes offline, blocked, incompatible, pending, syncing, and up-to-date states.
29. Mock-only tests are not represented as production Sync/Identity/Privacy Shield/Wardveil acceptance.
30. Exact candidate evidence identifies Browser revision, accepted Sync interface/source baseline, supported datasets/schemas, and test environment.

## 23. Implementation Sequence

1. Implement Browser-local dataset constants by consuming the accepted Sync capability contract; do not duplicate the registry as an independent authority.
2. Define Browser payload schemas for `browser.tabs`, `browser.history`, and portable `browser.preferences` with explicit private-state exclusions.
3. Implement the durable Browser sync journal and deterministic local operation IDs.
4. Implement outbound eligibility/projection builders and unit tests with all authority adapters stubbed fail closed by default.
5. Implement the Browser Sync adapter against the accepted shared capability negotiation and record-envelope interface.
6. Implement inbound Browser payload validation and staged transactional apply.
7. Integrate shared acceptance results, replay/high-water behavior, bounded retrieval/cursors, and observation/tombstone convergence where exposed by the accepted Sync API.
8. Integrate authenticated Identity/device trust and production Privacy Shield/Wardveil decision acquisition without changing Browser payload semantics.
9. Add profile/account namespace handling, device revocation response, pause/resume, and status UX.
10. Complete process-death, long-offline, corruption, duplicate, out-of-order, schema-migration, deletion, sustained-offline, resource-bound, accessibility, and representative-device acceptance.
11. Only after Phase 0 passes should Browser add additional datasets such as richer session/Webspace/permission state, and only through an accepted canonical registry/interface.

## 24. Non-Goals for Phase 0

Phase 0 does not implement or claim:

- production Sync transport enablement by itself;
- Browser credential/passkey synchronization;
- Private or Isolated Private synchronization;
- raw Browser engine-session synchronization;
- arbitrary filesystem synchronization;
- backup/recovery replacement for Everkeep;
- a Browser-specific device-trust system separate from Identity/Sync/Wardveil;
- new bookmark authority;
- speculative datasets not accepted by the shared Sync registry;
- production Stable qualification.

## 25. Core Invariants

1. Browser semantics remain Browser-owned.
2. Shared Sync registry/envelope/acceptance/replay/retrieval mechanics are consumed, not forked.
3. `browser.tabs`, `browser.history`, and `browser.preferences` are the current Browser dataset namespace; Bookmarks remains `bookmarks.*` authority.
4. Private and Isolated Private state never becomes ordinary Sync input.
5. Shared Sync acceptance and Browser local application acceptance are separate required gates.
6. Sign-in, network reachability, or peer authentication alone does not authorize a dataset operation.
7. Local Browser transactions commit before their Sync projection is published.
8. Retries are idempotent; transport retries are not new Browser mutations.
9. Tombstones carry no deleted application payload.
10. Stale/replayed state cannot undo a newer local/accepted state or converged deletion.
11. Profile/account/device namespaces cannot be crossed implicitly.
12. Browser Sync does not become Vault, Privacy Shield, Wardveil, Identity, Everkeep, Bookmarks, DNS, Network, or Browser authority.
13. Sync availability does not imply backup or recoverability.
14. A documentation contract or mock test does not prove production integration.

The implementation principle is: **project the minimum committed Browser state into the accepted shared Sync contract, preserve all independent authority boundaries, and fail closed whenever capability, schema, privacy, trust, profile, or application invariants are uncertain.**

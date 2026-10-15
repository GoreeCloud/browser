# GoreeCloud Browser — Unified Bookmarks contract

**Status:** Development-only, portable domain layer implemented; no Gecko chrome, Android, iOS or Windows UI integration has shipped. This specification is not release evidence.

## User-facing structure

Exactly two storage roots exist in the **Unified Bookmarks** interface, in this order:

1. **Local Bookmarks** — device/profile-local and available without any account or connection.
2. **GoreeCloud Bookmarks** — account-owned and optionally synchronized, only when an approved authenticated GoreeCloud Bookmarks adapter is available.

There are **no** separate top-level **Other Bookmarks**, **Bookmarks Toolbar**, **Bookmarks Menu**, or **Mobile Bookmarks** locations. Folders may exist *inside* either destination. The toolbar is a presentation of chosen bookmarks, not a third storage root.

## Interaction contract

- Saving and adding a bookmark must show a visible **Save to** picker with **Local Bookmarks** and **GoreeCloud Bookmarks**. Always display the selected destination in the confirmation and edit surface. A first-run, logged-out browser chooses Local Bookmarks but allows users to change the destination before saving.
- A cloud destination without an authenticated/authorized account remains visible but unavailable, with a clear sign-in/connect explanation. **Never silently reroute a cloud save to local.** Cloud writes are not presented as saved to GoreeCloud until the approved account-backed adapter confirms success; pending offline sync needs a distinct, truthful status.
- All CRUD, import/export, folders, searches and location filters preserve destination identity. Cross-location copy/move must be an explicit, consented, recoverable operation with conflict and rollback rules; it is **not** enabled by this domain module.
- Local bookmark management must function with the network disabled. Signing out must not delete local bookmarks. Remote account data must never leak between local profiles or cloud accounts.
- Legacy Firefox roots are an **import-source detail**, not destination categories. A read-only preview combines their contents under Local Bookmarks, preserves child folders, and records per-source counts for reconciliation. The preview alone performs no writes. The eventual importer must take source backups, resolve duplicate IDs and destinations, show a complete inventory, transactionally merge, reconcile counts, and provide rollback.

## Cross-platform navigation contract

| Platform | UI composition | Local authority | Cloud adapter | Verification status |
| --- | --- | --- | --- | --- |
| Linux | Unified Bookmarks in sidebar/library, toolbar save dialog and manager | Gecko privileged profile storage under review | GoreeCloud Bookmarks API, opt-in | Not integrated |
| Windows | Same locations, labels, picker and data format | Gecko privileged profile storage under review | Same reviewed protocol | Not integrated |
| Android | Same two-location bookmark screens and add/edit sheet | Application-private database, offline | Same reviewed protocol | Not integrated |
| iOS | Same two-location bookmark screens and add/edit sheet | App-private native storage using permitted iOS browser engine/API | Same reviewed protocol | Not integrated |

Cross-platform **experience and interoperable data semantics** are required; this does not imply all platforms use Gecko internally. Platform browser-engine restrictions, app sandboxing, secure storage, and accessibility conventions remain applicable.

## Implementation currently in this branch

- `unified.mjs`: portable controller dispatching between two explicitly selected stores. Enforces missing/invalid destination failures and cloud-connection gating at the UI/domain boundary; cloud adapters **must still enforce authentication and authorization per operation**. Neither network persistence nor account synchronization has been implemented here. The controller treats exceptions from the session gate as cloud-unavailable and binds returned location labels to the user's explicit selection, not to untrusted adapter response fields.
- `portable.mjs` and `store.mjs`: local-only interchange and transactional domain interfaces. Portable import and export reject credential-bearing URLs and ambiguous control-character or padded URLs; they do not silently rewrite invalid sources. The store driver must implement crash-safe atomic replacement before it is connected to a real profile.
- `gecko-profile-driver.mjs`: **unregistered, disabled-by-default development prototype** for privileged Gecko profile storage using `IOUtils` atomic temporary-file replacement and backup. Its driver validates persisted JSON before permitting writes, rejects existing bookmark files whose reported permissions are not exactly `0600` **before reading**, rechecks the file mode before replacement, rejects already-visible on-disk changes, and **requires a privileged host-supplied profile security guard** for each read/write. The guard must confirm exclusive profile ownership, private directory and safe paths; passing a callback does **not** itself implement a lock or verify filesystem security. Driver-local reads and writes are serialized, with profile authorization rechecked **after asynchronous file metadata checks and directly before the next bookmark-content read**, again after validation and immediately before disk writes. Deterministic Node tests revoke a synthetic guard while `stat` is pending and prove no unauthorized read/replacement occurs. These checks do not establish an actual exclusive profile lock. This preflight is **not** atomic cross-process concurrency control. The draft secure-temp follow-up precreates `0600` unique temporary files with `IOUtils.createUniqueFile`, verifies `IOUtils.stat(...).permissions` is exactly `0600` before writing bookmark contents, and inventories orphaned temp paths through `IOUtils.getChildren`, preserving both legacy `.tmp` and unique interrupted candidates for explicit recovery review. Missing or permissive permission metadata fails closed. Gecko reports generic Unix-style `0666` on Windows and does not establish owner-only ACLs; the feature therefore stays disabled there. A privileged Gecko chrome-mochitest is registered under `xpcom/ioutils/tests/test_goreecloud_bookmark_private_temp.html` but **has not yet run** (no compiled Firefox test runtime available). Host-owned profile locking, symlink/path security, actual privacy at first creation in Gecko, real crash recovery, and xpcshell/browser runtime coverage remain pending; do not connect it to users' Places data or production profiles.
- `gecko-recovery-inspector.mjs`: **unregistered development-only read-only preview** of primary, backup, legacy `.tmp`, unique `.json.tmp-` and Gecko collision `.json-N.tmp-` bookmark artifacts. Requires the same host security guard; reports only bounded status labels (no bookmark contents, URLs, or filesystem paths), requires the existing file's reported permissions to be exactly `0600` **before reading** (otherwise reports `private-mode-unverified` and requires manual review), imposes file/directory limits, rejects directory inventories containing paths outside the profile or nested children, and never restores or deletes files. The driver also rejects inconsistent inventory paths before writes and validates existing backup metadata, exact private `0600` mode and portable bookmark JSON before permitting backup rotation, preserving corrupt/nonprivate backups for manual review. The source-level checks do not establish native host symlink or race safety. `gecko-profile-recovery-collision.test.mjs`, `gecko-recovery-inspector.test.mjs`, and `gecko-recovery-filesystem.test.mjs` check source contracts and disposable Node filesystem behavior. This is **not** native Gecko, symlink-safe profile-lock, or crash-durability acceptance.
- `gecko-profile-driver.test.mjs`: synthetic source-level regression tests for first run, persistent reopen, backup and failure recovery, corrupt data, disappearing/arriving files, and externally modified data. These are not Gecko browser runtime tests.
- `gecko-profile-filesystem.test.mjs`: temporary, isolated Node filesystem integration tests with an IOUtils-shaped emulator for persistence, backup movement, orphan `.tmp` preservation, simulated interrupted writes, corrupt files, guard refusal, and pre-chmod permissions. Firefox 158 `IOUtils::WriteSync` moves the existing primary to backup and requests creation mode `0666`, modified by umask; therefore later `setPermissions(0600)` is **not** proof of private-at-creation. These tests cannot certify native Gecko file operations, locking, symlink protection, or crash durability.
- `previewFirefoxLegacyBookmarks`: read-only conversion from validated source-root contents to one portable Local Bookmarks collection, with no new top-level root folders and no implicit cloud upload.
- `unified.test.mjs` and `portable.test.mjs`: pure Node tests for location choice, routing isolation, offline behavior, session-gate failure, response spoofing, logged-out refusal, migration previews, URL safety and malformed inputs. These are not native runtime or user-interface tests.

## Acceptance gates before disabling Firefox legacy bookmark UI

1. Real crash-safe storage and backup/restore for Local Bookmarks on each platform.
2. Reviewed authenticated GoreeCloud Bookmarks account adapter, device/session isolation, synchronization error handling and remote-write confirmations.
3. Native interface at all relevant entry points with accessible labels, keyboard/touch interaction, narrow and wide viewports, and consistent empty/error states.
4. Non-destructive Firefox and CEF migration preview, approval, reconciliation, idempotence, rollback and lossless export.
5. Disabled-network end-to-end tests; account connect/disconnect, cloud errors, duplicate conflicts, failed writes, and cross-account tests.
6. Full Gecko compile and exact-head runtime evidence; no merging into the existing production CEF Browser solely on the basis of these module tests.

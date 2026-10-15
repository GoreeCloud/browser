# GoreeCloud Browser — native ecosystem migration plan

**Status:** development-only contract; no runtime integration or release acceptance yet.  
**Source baseline:** signed Firefox 158.0 source; staged above `feat/gecko-linux-branding-first-pass`.  
**Current desktop Browser:** the CEF/Chromium implementation remains untouched.

## Non-negotiable invariants

1. A fresh profile browses locally without login or internet access. Local bookmarks, credentials, browser settings, and installed extension packages must not depend on a server.
2. The four replacement features are **privileged, built-in Browser services** rather than optional WebExtensions. The extension runtime remains standards-compatible; the manager, distribution, trust, and policy surfaces become GoreeCloud-owned.
3. Cloud features are opt-in. A failed or disabled sync service never disables local storage or basic navigation; server-side sync never takes ownership of offline data without authenticated reconciliation.
4. Gecko security boundaries, origin isolation, permission checks, certificate validation, and WebExtensions compartmentalization must not be weakened.
5. Firefox profile and current CEF profile data are not automatically deleted. Every importer is previewable, bounded, reversible, idempotent, and verifies backup/rollback. Migration from Firefox and CEF are distinct.
6. No cloud telemetry, extensions catalog network access, or account requirement by default. Native integrations remain disabled until exact-version runtime tests pass.
7. Mozilla/third-party licensing remains in place. Forked files comply with MPL-2.0, even if GoreeCloud product identity is independently owned.
8. Never invent cryptography for Vault or Sync. Protected credentials stay behind a vetted encrypted local-store API and master-key lifecycle. No plaintext secrets in sync queues, logs, crash data, or generic JSON settings.

## Service boundaries and delivery sequence

| Service | Offline authority | Online adapter (optional) | Migration gate | Ship gate |
| --- | --- | --- | --- | --- |
| Browser Extensions | Gecko-compatible extension engine and GoreeCloud-owned installed-package catalog and manager | GoreeCloud-signed catalog and update service; never silently trust arbitrary remote code | Read installed Firefox add-on list, keep per-site permissions, import signed or explicitly trusted packages | Package-signature verification, least-privilege prompts, CSP/sandbox, updates, offline ZIP/XPI installation, uninstall rollback |
| Bookmarks | First-party Browser profile store with transactional folders/bookmarks and portable local import/export | Existing GoreeCloud Bookmarks service using separately reviewed account/device API | Explicit Firefox Places and CEF bookmark import with counts/preview/rollback | Crash-safe persistence, conflict resolution, export round-trip, recovery, full offline CRUD |
| Browser Sync | Encrypted device-specific change queue for *approved* fields; disabled if never connected | GoreeCloud Browser Sync protocol, with device trust and per-type versioned contracts; GoreeCloud Sync general file-transfer transport is **not** a database replica | Opt-in pairing only, reversible sign-out and server detach | Device authorization/revocation, end-to-end encryption, conflict preservation, retry/replay protections |
| Vault | Encrypted local vault independent from browser-sync or account lifecycle | Optional GoreeCloud Vault Server encrypted-data protocol (not raw browser sync) | Explicit import from Firefox Login Manager, verification and non-destructive copy | Origin-bound autofill, locked-by-default states, reviewed crypto/key storage, reauth, passkey/WebAuthn, recovery and adversarial tests |

## Implementation milestones

- **M0** Verified upstream lineage, independent draft branches, signed source, full-source configure, no binary or production release.
- **M1** Shared schema/versioning and offline bookmarks portable data contract, with source-level unit tests. The code under `goreecloud/native/bookmarks/` is a **pure domain module only**; it does not yet replace Gecko's Places backend or write persistent data.
- **M1.5 (development prototype only)** An opt-in Gecko profile bookmark driver exists in `goreecloud/native/bookmarks/gecko-profile-driver.mjs`, with source-level tests, but is **not installed, registered, or enabled** in a built browser. Its IOUtils replacement/backup, validated-read preflight and cache recovery are not evidence of crash-safe or private-at-creation behavior on real filesystems. Exclusive profile locking, symlink/path auditing, safe permissions, backup restore, xpcshell and browser runtime tests remain prerequisites to M2.
- **M2** Package a privileged Gecko Browser service through a minimal reviewed moz.build overlay; implement local durable profile database and UI entry point, gate behind development pref; run xpcshell/browser tests and validate profile recovery.
- **M3** Extensions management with compatibility and offline packages; Firefox-facing UI replaced only after install/update/removal test evidence.
- **M4** Vault integration using approved encryption/credential lifecycle; do not suppress existing Firefox password saving until local Vault acceptance.
- **M5** Browser Sync opt-in with verified Bookmarks/Extensions/Settings adapter; Vault Sync is a separately authorized encrypted data flow.
- **M6** Controlled Firefox/CEF imports, hardening, performance and accessibility; runtime browser tests, migrations, packaging, and explicit release gates.

## Responsibility boundaries

Browser owns chrome UI, local profile data and client adapters. GoreeCloud Bookmarks owns its remote data APIs and storage rules. GoreeCloud Vault Server owns the server-side opaque ciphertext authority. GoreeCloud Sync's general transfer service is not automatically Browser Sync, and cross-product names do not imply compatible protocols.

No phase may be presented as completed merely because its interface, schema, documentation or tests exist.

# GoreeCloud Browser — Gecko Fork-to-Native track

This repository includes Mozilla Firefox **158.0**, acquired from Mozilla's signed stable source archive on 2026-10-13. It is a **development candidate**, not a production browser or released replacement for the existing GoreeCloud Browser desktop CEF/Chromium implementation.

## Provenance and protected upstream boundary

* `goreecloud/upstream/BASELINE.json` pins the exact source archive, SHA-256, release tag/commit and verified Mozilla release-signing subkey.
* `LICENSE`, Mozilla and third-party notices, and all applicable attribution remain in the upstream tree. Rebranding must **not** delete license notices or Mozilla credit.
* All GoreeCloud integration adapters, design contracts, tests, and release documentation belong in `goreecloud/`. A source-level change outside this directory must have a narrowly scoped patch plus regression evidence.
* The canonical target for Glaze is Stable-compatible **1.7.0**. Glaze in this repository is **not integrated or accepted** yet.
* The existing CEF runtime and its user data must not be overwritten. Gecko requires explicit profile, bookmarks, extensions, login, DNS, TLS, privacy and rollback migration evidence.

## Upstream releases

Firefox moved to a two-week major release cadence with emergency patch releases still possible. The scheduled `firefox-upstream-watch.yml` workflow checks Mozilla's product-details endpoint twice a day, validates the reported stable version, compares it against this pin, and opens one issue for an observed newer release. It does **not** download unverified binaries, automatically overwrite source, merge unreviewed code, or publish a browser.

To update this fork:

1. Identify the latest Mozilla **stable** source release (including any dot release) and capture the exact release tag/commit.
2. Download Mozilla's source archive and detached signature into an **isolated staging directory**.
3. Verify the full archive with the current official Mozilla release key and independently record the SHA-256.
4. Import the verified source snapshot to an upstream-only candidate commit, retaining the `goreecloud/` overlay separately.
5. Compare source and overlay integration points, inspect breaking changes and Mozilla security advisories, reconcile licenses, and test.
6. Promote **only** after the release-gates checklist and independent runtime evidence pass. Keep rollback to the previous validated release.

`goreecloud/tools/upstream_watch.py` tracks release availability, **not** security advisory completeness.

## Mandatory release gates

`goreecloud/RELEASE-GATES.md` is the acceptance checklist. All gates are pending until explicitly evidenced. A successful source import, signature check or green source-level CI must never be reported as a production-qualified browser.

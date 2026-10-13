# Source divergence register

## Baseline

Mozilla Firefox 158.0 stable source is preserved as the first committed upstream snapshot (separate from the GoreeCloud overlay). Archive origin and cryptographic checks are in `BASELINE.json`.

## Downstream changes

- `goreecloud/`: candidate-only GoreeCloud branding strings, integration inventory, provenance checks and release gates; no Gecko engine alterations yet.
- `.github/workflows/pr-handler.yml`: relocated without modifying contents to `.github/upstream-workflows/pr-handler.yml`, because this Mozilla-owned workflow targets Mozilla-only team membership, Lando credentials and PR infrastructure. It must not execute automatically for GoreeCloud pull requests.
- Root `README.md`: downstream project description; original Mozilla project documentation remains in `docs/` and source tree.
- `.github/workflows/firefox-upstream-watch.yml`: advisory version-check issue creation; does not merge source.
- `.github/workflows/gecko-source-structural.yml`: source metadata and unit tests only; not a substitute for Firefox builds.

## Future upstream merges

Apply upstream changes to a new isolated snapshot commit. Re-apply narrow patches and GoreeCloud-specific overlay separately, comparing this register with each release. Do not remove required licenses or assume that a previously validated Firefox build proves current Gecko runtime acceptance.

# GoreeCloud Browser Artwork — Current Authority

## Current canonical artwork

The authoritative Browser product source is `GoreeCloud/branding-assets/products/browser/app-icon.svg` (Git blob `3f502e5a6dae8ff3cb86dfdc3ae5d2eb2e8b54e1`, verified on September 23, 2026). The central asset was updated September 8, 2026 to the blue compass-style icon as part of the unified GoreeCloud mobile family. The central repository and its current machine-readable catalog control approved product identity.

The earlier local Tabmark approval and aqua identity narrative in this document predated that central update. It is **historical, superseded provenance**, not current Browser identity authority; Git history retains the older narrative without keeping competing approved masters in the current tree.

## Consumer derivatives

- `assets/branding/goreecloud-browser.svg` must remain an exact copy of the current pinned central SVG. The Android source guard validates its complete Git blob identity.
- `assets/branding/goreecloud-browser-monochrome.svg` simplifies only paint to a monochrome ring/needle while preserving the canonical compass-ring, needle path and center coordinates. This derivative is pending human/theme/device visual review.
- `apps/android/app/src/main/res/drawable/ic_launcher_*.xml` and `mipmap-anydpi-v26`/`mipmap-anydpi-v33` are Android packaging derivatives. Source checks and APK resource enumeration do **not** prove exact-rendered identity, mask contrast or real launcher visibility.

The original official central blue/white compass artwork is not recreated by a consumer. Any future redesign or approval belongs in `GoreeCloud/branding-assets` first, followed by validated propagation to Browser and its platforms.

## Remaining acceptance

Browser issue #33 requires inspection of the **exact intended APK** on GoreeCloud Launcher and Lineage/Trebuchet, with adaptive, round and themed icon configurations on light and dark backgrounds and small-size contexts. Check icon identity, optical geometry, legibility, launch intent, actual navigation/search usability and rollback. A passing source/provenance check and green emulator CI do not establish the required representative physical-device, accessibility, privacy/security, Release Candidate, production or Stable gates.

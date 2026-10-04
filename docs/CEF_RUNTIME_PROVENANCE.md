# GoreeCloud Browser — Linux CEF Runtime Provenance

**Status:** Development dependency record  
**Applies to:** Linux x86_64 render-capable Browser milestone  
**Production acceptance:** No

## Selected runtime candidate

GoreeCloud Browser currently pins:

- CEF: `152.0.6+g708dc14+chromium-152.0.7977.83`
- Chromium: `152.0.7977.83`
- Platform: `linux64`
- Distribution: `minimal`
- Release family: CEF Stable / preferred build at the time of selection
- Upstream binary service: `https://cef-builds.spotifycdn.com/`
- Minimal archive SHA-1: `9711b86c105fb590da576fe5a829802f1a79d520`
- Minimal archive SHA-256: `daf8c2b6e63787d6a91d666205a8a4521419937eabaf47723738c86aea7135bd`

The minimal distribution is intentional for this Release-mode Development milestone. It contains the CEF headers, CMake configuration, C++ wrapper source, and Release runtime files required by the existing Browser CEF integration without adding Debug binaries or sample-application source.

## Integrity chain

`scripts/bootstrap_cef_linux.py` constructs the exact pinned archive name and source URL. It then:

1. fetches the matching `.sha1` sidecar from the same official CEF build service;
2. requires that response to equal the repository-pinned SHA-1;
3. downloads the exact archive;
4. computes SHA-1 and SHA-256 while streaming and rejects the archive unless both equal the repository pins;
5. requires cached archives and extracted provenance records to carry those same immutable identities;
6. validates archive member paths before extraction;
7. rejects device/FIFO archive entries and traversal-capable link targets;
8. validates required CEF headers, CMake files, `Release/libcef.so`, and resource files;
9. writes `.goreecloud-cef-provenance.json` inside the extracted root.

CEF's official project download helper uses the same upstream service and official SHA-1 sidecar model for binary-distribution verification. GoreeCloud additionally pins the observed SHA-1 and SHA-256 from the verified minimal archive so an upstream sidecar or archive change cannot silently change Browser renderer bytes.

## Build guard

When `GOREECLOUD_ENABLE_CEF=ON`, Browser CMake reads `include/cef_version.h` from `GOREECLOUD_CEF_ROOT` and fails configuration unless `CEF_VERSION` exactly matches the repository pin.

A locally supplied CEF root therefore cannot silently substitute another runtime version.

## Runtime safety boundary

The Browser CEF integration refuses to initialize when its sandbox option is disabled. The Linux launcher does not add `--no-sandbox`. If the host requires the CEF SUID sandbox helper, the helper must be configured according to the CEF Linux sandbox instructions rather than bypassing sandboxing.

TLS validation and site-isolation requirements remain mandatory Browser acceptance boundaries.

## Rotation procedure

Changing the CEF pin requires a deliberate Browser dependency update:

1. verify the new release on the official CEF build source;
2. update the bootstrap and CMake pin together;
3. run the offline bootstrap-contract CI gate;
4. build the exact CEF-enabled Browser candidate;
5. run the governed render-capable Beta acceptance suite on representative Linux hardware;
6. record the exact runtime/archive provenance and any compatibility changes;
7. merge only after the exact Browser source candidate passes applicable checks.

## Current owner-device Development evidence

Exact Browser source `27e1b8a9ecea3feb184425a72cdf8fd6eb9ea89b` built successfully on the owner's Zorin OS 17.3 laptop using this exact pinned CEF archive and provenance record. All 13 configured CTest targets passed.

In the active native Wayland session, normal GTK backend detection selected the Browser windowless CEF path without the Development-only force-windowless override. The Browser created a 1280×723 windowless page surface, completed `https://example.com/` with HTTP 200, and GTK reported a materially non-uniform software frame.

The generated local `chrome-sandbox` helper was not configured root-owned/setuid on that device, so packaged Linux sandbox acceptance remains open. The Browser did not add `--no-sandbox` or weaken its sandbox requirement to obtain the runtime result. See `docs/LINUX_WAYLAND_OWNER_DEVICE_EVIDENCE_2026-10-03.md` for the exact source/environment boundary and open device diagnostics.

An upstream version being newer does not automatically authorize a Browser dependency change.

## Non-claims

The current pinned CEF archive has exact-source Development build and native-Wayland page-render evidence on the owner's laptop as recorded above. This record still does not establish full render-capable Browser Beta acceptance, production readiness, packaged sandbox/site-isolation acceptance, release signing, representative interaction/accessibility/performance acceptance, or Stable/Seal/Anchor product maturity.

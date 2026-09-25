# Chromium Embedded Framework dependency provenance

**Component:** Chromium Embedded Framework (CEF)  
**Purpose:** Replaceable Chromium/Blink implementation behind the GoreeCloud Browser Engine Layer  
**Current reviewed channel:** Stable  
**Pinned CEF version:** `152.0.6+g708dc14+chromium-152.0.7977.83`  
**Chromium version:** `152.0.7977.83`  
**Initial reviewed platform:** Linux x86_64 / `linux64`  
**Official binary source:** `https://cef-builds.spotifycdn.com`  
**Acquisition:** Explicit opt-in only through `GOREECLOUD_CEF_AUTO_DOWNLOAD=ON`

## Integrity model

The Development acquisition helper follows the official CEF sample-project distribution convention:

1. Select the exact source-controlled Stable CEF version and platform.
2. Fetch the matching official `.sha1` sidecar over HTTPS.
3. Reject malformed sidecars.
4. Verify the downloaded archive against that upstream-published hash before extraction.
5. Validate the expected CEF header and CMake entry points after extraction.
6. Emit the observed SHA-256 into the configure log for evidence and follow-up pinning.

The upstream SHA-1 sidecar is transport/integrity evidence, not independent production trust or a substitute for GoreeCloud release certification. Production dependency acceptance still requires the applicable vulnerability, provenance, licensing, update, packaging, rollback, runtime, and release evidence.

## Security boundary

CEF is an implementation dependency, not the GoreeCloud Browser product architecture. The Browser-owned engine abstraction remains authoritative for product behavior and replaceability.

The CEF path must not disable Chromium sandboxing or site isolation to make integration easier. Private contexts must remain ephemeral. Missing, stale, malformed, or unverifiable dependency evidence fails closed.

## Update rule

Changing the pinned CEF version requires a reviewed source change, fresh exact-head CI, dependency/security review, and fresh Browser runtime acceptance. Build callers cannot silently select an arbitrary CEF version through a cache variable.

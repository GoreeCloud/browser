# GoreeCloud Browser Android Beta APK

**Target:** Android installable Development beta APK  
**Package:** `io.goreecloud.browser.beta`  
**Candidate version:** `0.1.0-beta.1+android.7`  
**Candidate versionCode:** `10007`  
**Minimum Android:** 8.0 / API 26  
**Target Android API:** 35  
**Current Glaze UI target:** V1.6 / `1.6.0` Stable (product-local migration pending)  
**Current Platform Contract:** `0.4` / nine Integral Platform Systems  
**Production approved:** No

## Purpose

This target is a real installable GoreeCloud Browser Development beta. Android System WebView/Chromium is a bounded, replaceable rendering-engine dependency; GoreeCloud retains Browser product identity, chrome, navigation/search policy, privacy/security boundaries, state, and integration authority.

The `+android.7` candidate adds a Browser-owned page-title presentation boundary so raw URL/resource-path/control-character engine titles cannot become chrome identity. It retains the historical V1.5.1-era native mapping and exact-head artifact provenance; migration to current Stable Glaze UI V1.6.0 remains pending. These changes do not establish rendered/native-device acceptance or production readiness.

## Implemented beta behavior

The current Development line includes:

- GoreeCloud Browser Beta application identity and launcher activity.
- HTTP/HTTPS browser intent handling.
- Direct URL navigation independent from search.
- Bare-host HTTPS upgrade.
- GoreeCloud Search as the sole intended remote query authority for non-URL input, with transmission still fail-closed until authorization/authentication gates are accepted.
- JavaScript and DOM storage for web compatibility.
- Cleartext application traffic disabled.
- Mixed content blocked.
- Android Safe Browsing enabled with return-to-safety behavior.
- TLS/certificate errors cancelled rather than bypassed.
- Third-party cookies disabled by default.
- WebView file/content access disabled.
- Website permission/geolocation requests denied until the Browser permission broker and required authorities are accepted.
- Downloads blocked until the Wardveil-gated download release path is accepted.
- Browser-owned navigation, address-presentation, safe page-title presentation, Search/Privacy Shield, and Glaze contract tests.

## Browser chrome

The Android beta uses Browser-owned native chrome with a compact top omnibox, web-content region, 56dp bottom toolbar, vector controls, scroll-aware top-chrome collapse/restore, Browser-owned menu sheet, and effects-free accessibility fallbacks.

The current source preserves the historical V1.5.1-era Browser shell rather than claiming a completed V1.6.0 migration. Glaze UI remains presentation-only and cannot create authorization, consent, security/privacy truth, provider precedence, navigation authority, or automatic consequential execution.

## Glaze UI migration status and historical V1.5.1 mapping

The current shared Stable target is **V1.6.0**. The source evidence below is historical V1.5.1 provenance only; product-local V1.6.0 migration and rendered, accessibility, device, performance, and rollback acceptance are still required.

Historical source baseline (not the current Stable target):

- Stable version: `1.5.1`.
- Stable promotion revision: `98da57064ede0f334627b632bc16801f580331af`.
- Reviewed V1.5 implementation anchor: `ee1032a0822ab8e103f8afe48e5c1859fde65cc9`.
- V1.5.1 qualification source anchor: `5b59d0e36950d737dba35b58ae58058684e0831b`.
- V1.5.1 qualification integration revision: `f7ef915f0aabea6cf92748018f2220a99e3a9c92`.
- Immediate Stable rollback baseline: `1.5.0`.
- Inherited optical baseline: `1.4.1`.

Browser source maps current capability-aware presentation while retaining fail-closed provider conflict/absence handling, accessibility precedence, privacy-safe diagnostics, bounded optical behavior, and no automatic consequential/fallback execution.

Shared Glaze V1.5.1 qualification does **not** certify Browser. Browser-specific rendered/native visual, TalkBack/screen-reader, 200% text, RTL/localization, Reduced Motion, Reduced Transparency, Increased Contrast, representative device/posture, sustained performance, workflow, rollback, and production acceptance remain open.

## Platform Contract boundary

Browser now declares Platform Contract `0.4` and explicitly evaluates all nine Integral Platform Systems:

1. GoreeCloud Manager
2. Privacy Shield
3. Wardveil Security
4. Everkeep
5. Glaze UI
6. GoreeCloud Mesh
7. GoreeCloud Identity
8. GoreeCloud Policy
9. GoreeCloud Observability

GoreeCloud Sync remains a separately governed application/service capability. The Browser manifest remains `development` and `nonconformant`; adding Policy and Observability declarations does not manufacture acceptance.

## APK build and exact-source evidence

Workflow: `.github/workflows/android-beta.yml`

The workflow uses Java 17, Android API 35, Android Build Tools 35.0.0, and Gradle 8.9. The separately gated Android 15 managed-emulator job exercises a bounded native launcher/chrome smoke test, requires the Back, Forward, Start page, Reload/Stop, Browser menu, and Go controls with initial history controls disabled, and verifies the free-text Search authorization-required local page; it does not substitute for representative physical-device acceptance. It runs:

```text
gradle --no-daemon clean testDebugUnitTest lintDebug assembleDebug assembleDebugAndroidTest
```

For pull requests, the workflow explicitly checks out `github.event.pull_request.head.sha`, verifies `git rev-parse HEAD` matches that revision, validates APK signature/package/version/label, records `SOURCE_REVISION` and `SHA256SUMS.txt`, and names the retained artifact for the exact evaluated revision.

Expected APK path:

```text
apps/android/app/build/outputs/apk/debug/app-debug.apk
```

A green run proves only the checks performed against that exact Development revision. It does not establish production signing, security/privacy acceptance, real-device behavior, Stable qualification, or deployment acceptance.

## Signing boundary

CI uses Android debug signing for installability. Debug signing is not GoreeCloud production signing authority, and independently produced debug APKs are not guaranteed to upgrade one another. Production signing material must not be stored in source.

## Security and privacy boundaries

The beta continues to fail closed where required authority is unavailable:

- downloads remain blocked pending accepted Wardveil verification/release;
- website permission/geolocation requests are denied pending accepted Browser/OS/Privacy Shield/Wardveil decision paths;
- TLS errors cannot be bypassed by this shell;
- mixed content remains blocked;
- third-party cookies remain disabled by default;
- WebView file/content access remains disabled.

These source defaults are useful Development controls, not complete Wardveil or Privacy Shield acceptance.

## Promotion gates

Before Android Browser may be represented as production-approved or Stable, current GoreeCloud Stable qualification still requires exact-release evidence for applicable functionality, security, privacy, accessibility, recovery, Glaze UI, all nine Integral Platform Systems, supported platforms, dependencies, artifact provenance, signing/distribution, upgrade/rollback/migration, representative devices, and production acceptance.

Current known Browser-specific blockers include accepted runtime Manager/Privacy Shield/Wardveil/Everkeep/Mesh/Identity/Policy/Observability integration, Browser-local Glaze V1.6.0 migration and acceptance, production signing, representative Android validation, complete permission/private-context/download/session/profile workflows, and published Stable release evidence.


## Launcher identity verification

Android Beta CI now validates the repository launcher-identity source boundary and inspects the built APK resource table for the adaptive launcher, round launcher, foreground, and Android 13+ monochrome resources. This proves source/package presence only.

Correct rendering in GoreeCloud Launcher, Lineage/Trebuchet, OEM launchers, themed-icon modes, supported densities, and representative physical devices remains a separate human/device acceptance requirement and is not inferred from APK packaging.

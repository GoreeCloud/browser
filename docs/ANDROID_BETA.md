# GoreeCloud Browser Android Beta APK

**Target:** Android installable Development beta APK  
**Package:** `io.goreecloud.browser.beta`  
**Candidate version:** `0.1.0-beta.1+android.12`  
**Candidate versionCode:** `10012`  
**Minimum Android:** 8.0 / API 26  
**Target Android API:** 35  
**Current Glaze target:** V1.7 / `1.7.0`  
**Canonical Glaze lifecycle:** Anchor  
**Current Platform Contract:** `0.4` / nine Integral Platform Systems  
**Production approved:** No

## Purpose

This target is a real installable GoreeCloud Browser Development beta. Android System WebView/Chromium is a bounded, replaceable rendering-engine dependency; GoreeCloud retains Browser product identity, chrome, navigation/search policy, privacy/security boundaries, state, and integration authority.

The current Android source targets bounded Glaze V1.7 / 1.7.0, inheriting the accepted V1.6.0 runtime behavior, and keeps free-text remote Search fail-closed until the required authorization and authentication evidence exists. Source implementation, emulator execution, or a green APK workflow does not establish representative physical-device acceptance, production approval, or release maturity.

## Implemented beta behavior

The current Development line includes:

- GoreeCloud Browser Beta application identity and launcher activity.
- Canonical GoreeCloud Browser compass branding with source-provenance guards for full-color, monochrome, and Android launcher derivatives.
- HTTP/HTTPS browser intent handling.
- Direct URL navigation independent from Search.
- Bare-host HTTPS upgrade.
- GoreeCloud Search as the sole intended remote query authority for non-URL input, with transmission fail-closed until authorization/authentication gates are accepted.
- Browser-owned navigation, address, title, blocked-link, and local recovery presentation.
- Main-frame callback correlation that rejects stale page completion/failure callbacks.
- JavaScript and DOM storage for web compatibility.
- Cleartext application traffic disabled.
- Mixed content blocked.
- Android Safe Browsing enabled with return-to-safety behavior.
- TLS/certificate errors cancelled rather than bypassed.
- Third-party cookies disabled by default.
- WebView file/content access disabled.
- Website permission and geolocation requests denied until Browser-owned policy surfaces and required authorities are accepted.
- Downloads blocked until the Wardveil-gated download release path is accepted.
- Browser-owned navigation, address-presentation, safe page-title presentation, Search/Privacy Shield, and Glaze contract tests.

## Browser chrome

The Android beta uses Browser-owned native chrome with a compact top omnibox, web-content region, 56dp bottom toolbar, vector controls, scroll-aware top-chrome collapse/restore, Browser-owned menu sheet, and effects-free accessibility fallbacks.

Glaze remains presentation-only. It cannot create authorization, consent, security/privacy truth, provider precedence, navigation authority, service availability, or automatic consequential execution.

## Normal-session recovery boundary

The Android Development beta has two distinct restoration paths:

- **Same-process Activity recreation** keeps the existing Android saved-state path for the live regular-tab graph and transient WebView bundles, guarded by the process-instance token.
- **Abnormal app-process termination** uses the Browser-owned C++ Normal-session journal/checkpoint store through a thin JNI adapter. Android accepts only the validated logical projection, rechecks tab IDs/order/active selection plus bounded safe HTTP(S)/Start locations and normalized titles, then creates fresh WebViews.

The durable path does not restore raw WebView memory/history bundles, form contents, reusable credentials, blocked-deep-link payloads, remote failure details, JavaScript/Images/Desktop-site session choices, or Private/Isolated Private state. Clean shutdown suppresses abnormal-session restoration. Corrupt, unsupported, or unrepresentable durable state fails fresh when Browser can safely reset the store; Browser does not invent recovery from invalid state.

This is Development Normal-context behavior. Released-version schema migration, representative physical-device/OEM/WebView fault acceptance, Private/Isolated Private crash-survival design, profiles/Webspaces, production recovery approval, Stable, Seal, and Anchor remain open.

## Glaze V1.7 source mapping

Current Browser source mapping:

- Consumer release: Glaze V1.7 / `1.7.0`.
- Canonical lifecycle: Anchor.
- Release integration revision: `1a5756daed2294155be2e9972b24f580f6222b7b`.
- V1.7 Stable-contract qualification source: `7c4ded83d7a8725165bb6a55dfb175667cc9589e`.
- Inherited accepted runtime source: GLAZE UI V1.6 / `1.6.0` at `a7180679ea851389e0f3004515f9a25f420e716d`.
- Immediate rollback baseline: `1.6.0`.
- Inherited optical baseline: `1.4.1`.

The Android-native Glaze contract targets V1.7.0 and intentionally retains the inherited accepted V1.6.0 behavior: fail-closed provider conflict/absence handling, accessibility precedence, privacy-safe diagnostics, explicit disabled-state presentation, bounded visual complexity, non-color semantic meaning, large-text reflow requirements, and no automatic consequential/fallback execution.

Central Glaze qualification does **not** certify Browser. Browser-specific rendered/native visual, TalkBack/screen-reader, large-text, RTL/localization, Reduced Motion, Reduced Transparency, Increased Contrast, representative device/posture, sustained performance, workflow, rollback, renderer-integrated, and production acceptance remain open.

## Platform Contract boundary

Browser declares Platform Contract `0.4` and evaluates all nine Integral Platform Systems:

1. GoreeCloud Manager
2. Privacy Shield
3. Wardveil Security
4. Everkeep
5. Glaze
6. GoreeCloud Mesh
7. GoreeCloud Identity
8. GoreeCloud Policy
9. GoreeCloud Observability

GoreeCloud Sync remains a separately governed application/service capability. The Browser manifest remains Development and nonconformant; declarations and green validators do not manufacture system acceptance.

## APK and managed-emulator evidence

Workflow: `.github/workflows/android-beta.yml`

The workflow uses Java 17, Android API 35, Android Build Tools 35.0.0, Gradle 8.9, pinned Android NDK 27.2.12479018, and pinned CMake 3.22.1. The build job runs:

```text
gradle --no-daemon clean testDebugUnitTest lintDebug assembleDebug assembleDebugAndroidTest
```

A dependent Android 15 managed-emulator lane then launches the Browser-owned activity and verifies a bounded runtime contract:

- Browser-owned title and Search/address field are present.
- Go, Back, Forward, Start page, Browser menu, and Reload/Stop controls are present.
- Back and Forward begin disabled on the local start surface.
- WebView file/content access remains disabled.
- Mixed content remains blocked.
- Third-party cookies remain disabled.
- The exact Development package identity is installed.
- Free-text Search remains on the local authorization-required surface and is not transmitted to GoreeCloud Search.
- The native recovery bridge loads in the managed Android runtime and regular-tab recovery preserves logical order/active selection while creating fresh transient WebViews.
- A dedicated shell-driven scenario seeds an abnormal Normal-session fixture through the real JNI bridge, terminates/force-stops the target app process, relaunches the Browser activity, and requires the recovered second-tab selection plus safe recovered omnibox location through UIAutomator.

For pull requests, the workflow checks out the exact pull-request head, verifies `git rev-parse HEAD`, validates launcher identity source provenance, validates APK signature/package/version/label/resources, requires the recovery JNI library in both `arm64-v8a` and `x86_64` APK payloads, records `SOURCE_REVISION` and `SHA256SUMS.txt`, and names the retained artifact for the exact evaluated revision.

Expected APK path:

```text
apps/android/app/build/outputs/apk/debug/app-debug.apk
```

A green run proves only those checks on that exact Development revision. The managed emulator can establish the bounded process-recovery scenario described above, but it does not establish representative physical-device launcher rendering, real-network browsing quality, sustained performance, accessibility, OEM/WebView recovery variance, released-version migration, production signing/recovery acceptance, or release qualification.

## Launcher identity verification

Android Beta CI validates that Browser-local full-color and monochrome branding files match the pinned canonical GoreeCloud branding assets and that the reviewed Android launcher background, foreground, and monochrome derivatives have not silently changed. It also inspects the built APK resource table for adaptive launcher, round launcher, foreground, and Android 13+ monochrome resources.

Correct rendering in GoreeCloud Launcher, Lineage/Trebuchet, OEM launchers, themed-icon modes, supported densities, and representative physical devices remains a separate device/human acceptance requirement.

## Signing boundary

CI uses Android debug signing for installability. Debug signing is not GoreeCloud production signing authority, and independently produced debug APKs are not guaranteed to upgrade one another. Production signing material must not be stored in source.

## Security and privacy boundaries

The beta continues to fail closed where required authority is unavailable:

- downloads remain blocked pending accepted Wardveil verification/release;
- website permission and geolocation requests are denied pending accepted Browser/OS/Privacy Shield/Wardveil decision paths;
- TLS errors cannot be bypassed by this shell;
- mixed content remains blocked;
- third-party cookies remain disabled by default;
- WebView file/content access remains disabled;
- remote Search remains blocked without accepted authorization/authentication.

These Development controls do not constitute complete Wardveil, Privacy Shield, Search, Identity, Policy, or production acceptance.

## Promotion gates

Browser remains Development. Before Android Browser can advance through later lifecycle stages, exact-release evidence must cover applicable functionality, security, privacy, accessibility, recovery, current Glaze acceptance, all nine Integral Platform Systems, supported platforms, dependencies, artifact provenance, signing/distribution, upgrade/rollback/migration, representative devices, and production acceptance.

Browser issue #33 remains the primary Android acceptance gate for representative physical-device launcher rendering, practical navigation/Search/error usability, accessibility and performance, current Glaze rendered/device acceptance, and release evidence.

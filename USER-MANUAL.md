# GoreeCloud Browser User Manual

## Current availability

GoreeCloud Browser is in active beta development. The Android target is a real installable test APK, but it is not production-approved or Stable.

Current Android beta identity for this source revision:

- Application: **GoreeCloud Browser Beta**
- Package: `io.goreecloud.browser.beta`
- Version: `0.1.0-beta.1+android.12`
- versionCode: `10012`
- Minimum Android: Android 8.0 / API 26
- Target API: 35
- Rendering dependency: Android System WebView/Chromium

GoreeCloud owns the Browser product layer, navigation/search behavior, mobile browser chrome, privacy defaults, security gates, and GoreeCloud integrations. Android System WebView/Chromium provides the web rendering/runtime foundation and is not the product identity.

### Linux desktop development candidate

The Linux GTK3 development host can present the GoreeCloud Browser shell on an available GTK desktop display. X11/XWayland keeps the native CEF child-window path. Native Wayland can use the CEF windowless/software-rendering path: CEF supplies BGRA page frames and the Browser paints those frames into its GTK web surface. Exact-head validation plus representative Zorin OS 17.3 Wayland evidence have established real Development page pixels. Pointer motion/enter/leave, left/middle/right clicks, wheel input, and focus forwarding are integrated on the windowless path; representative interaction acceptance plus keyboard/IME, popup, drag-and-drop, clipboard, cursor, accessibility, performance, and accelerated-buffer acceptance remain separate gates.

## Installing the Android beta

The CI-generated APK is debug-signed for testing. Use an APK supplied from the GoreeCloud Browser Android Beta workflow or another explicitly supplied GoreeCloud beta artifact tied to an exact source revision.

Android may require you to allow installation from the app used to open the APK. Enable that Android setting only for the trusted installation source you intend to use.

The current CI beta signing key is not the production signing authority. Fresh CI environments can produce different debug certificates. If Android reports that an update cannot be installed because signatures differ, remove the older beta package and perform a fresh install. Removing the beta can remove its local application data.

Do not treat the beta APK as a production release, managed update channel, or long-term data-preservation target.

## Mobile browser chrome

The Android shell uses two Browser-owned chrome regions:

- a top omnibox; and
- a bottom navigation toolbar.

The web page occupies the full region between them.

### Omnibox

The top omnibox reserves most of its width for the unified address/search field. When the field is not being edited, Browser presents a condensed address with the hostname kept at the leading edge. This prevents a long path such as `/preferences` from horizontally scrolling the field so far that the hostname disappears. Focus the field to expose and select the complete current URL for editing.

Use the arrow action at the right edge of the omnibox or the Android keyboard's Go action to navigate.

Enter a complete `https://` or `http://` URL to navigate directly. A host such as `example.com` is upgraded to HTTPS before navigation.

Text that is not interpreted as a URL is classified as a GoreeCloud Search intent, but this Development build keeps the query local and shows an authorization-required surface until accepted Privacy Shield authorization and compatible GoreeCloud Search capability evidence are available. Browser does not silently fall back to another search provider.

### Bottom navigation

The bottom toolbar contains:

- Back;
- Forward;
- Start page;
- Reload, which becomes Stop while a page is loading; and
- Browser menu.

Back and Forward visibly disable when no matching history action is available.

Page-loading progress is drawn at the top of the web-content region rather than using a separate toolbar row.

### Browser menu

The Browser menu uses a Browser-owned Glaze bottom sheet rather than Android's platform-default popup menu.

The current bounded menu actions are:

- Site information, which shows the current website origin and HTTPS/HTTP transport without claiming an independent trust or certificate verdict;
- Privacy & security, which summarizes Browser-enforced Android defaults and explicit fail-closed integration boundaries;
- Find in page, which searches only the currently rendered page and provides live match count plus Previous/Next controls;
- Page controls, which groups Text size, Desktop site, JavaScript, and automatic image loading into one compact Browser-owned surface;
- Guidance & tips, which replays setup and controls optional device-local Browser tips;
- Clear browsing data, which opens a confirmation surface before removing website data;
- Copy page address, only when the current surface has a Browser-approved HTTP(S) page address;
- Share page, under the same disclosure rule; and
- About this development build.

Browser-owned local surfaces such as the start page and local Search/error explanations do not expose implementation-only addresses through Copy or Share. The Start page already has a dedicated bottom-toolbar control. The current menu is still not the final Browser settings system.

### First-use setup and guidance

A fresh Android Browser profile opens a required three-step setup before ordinary use. Setup explains:

- GoreeCloud Browser ownership versus the Android System WebView engine dependency;
- the current fail-closed privacy and security defaults, including blocked mixed content, third-party-cookie defaults, certificate-error handling, website-permission denial, and the Clear browsing data control; and
- direct website navigation, the current non-transmitting free-text Search boundary, Find in page, and the Page controls surface.

Setup progress is stored only in Browser-local Android preferences. Progress is written synchronously before Browser advances to the next setup step, so Activity recreation resumes from the last durably accepted step rather than assuming an unsaved transition. The initial setup cannot be skipped. After completion, **Guidance & tips** can replay the setup voluntarily; replay can be dismissed without changing the completed first-use state.

Optional contextual tips are enabled by default after setup. The current compact Browser tip explains the direct-address versus fail-closed free-text Search boundary and can be dismissed with **Got it**. Guidance & tips can disable or re-enable optional tips and show the dismissed tip again.

First-use and tip state is device-local. It is not an account setting, Browser Sync dataset, Privacy Shield decision, telemetry signal, website permission, or provider authorization.

### Find in page

Find in page uses the active Android WebView's local page-search facility. Enter text to see live match status, then use Previous or Next to move among matches. Closing the Find surface clears the match highlights. The find text stays inside the active WebView and is not sent to GoreeCloud Search or another provider.

Find input is capped at 512 characters to keep the local operation bounded.

### Page text size

Text size changes website text rendering in GoreeCloud Browser from 75% through 200% in 25-point steps. Reset returns to 100%.

The selected value is stored as an application-local Browser preference and applies to later pages and launches. It does not change Android's system font-size setting and is not represented as synchronized Browser state until an accepted Browser Sync preference contract explicitly includes it.

### Page controls

Page controls groups several page-level settings into one compact Glaze surface:

- Text size opens the existing 75%–200% Browser text-size control. Text size remains an application-local Browser preference.
- Desktop site switches the current session between mobile and desktop-style presentation.
- JavaScript can be enabled or disabled for the current Browser session.
- Images can be enabled or disabled for automatic page loading in the current Browser session.

JavaScript and Images default to On for ordinary web compatibility. Changing either setting reloads the current HTTP(S) page so the new setting applies consistently. Their state survives Android Activity recreation but is not stored as a durable Browser preference and is not synchronized.

Disabling JavaScript or automatic image loading may cause websites to lose functionality or content. Browser does not describe these controls as Privacy Shield policy or as a security verdict.

### Desktop site

Desktop site changes the current Browser session to a desktop-style website presentation. Browser derives the desktop user agent from the active WebView engine user agent instead of pinning a separate stale Chromium version, enables wide-viewport/overview presentation, and reloads the current website when necessary.

Desktop site is session-local. It survives Android Activity recreation through saved state but is not stored as a durable Browser preference and is not synchronized.

### Clear browsing data

Clear browsing data always opens a confirmation surface before deletion. The confirmation identifies both the data that will be cleared and the settings that will remain.

The current Development action clears Android WebView cookies and website sign-in state, website storage, cached web content, form data, WebView navigation history, and WebView SSL preferences. After clearing, Browser returns to its local Start page and clears the prior back-navigation history again after that page finishes loading.

Browser preferences such as Page text size and Android app permissions are preserved. This action is app-wide for GoreeCloud Browser's Android WebView data; it is not yet a profile-scoped privacy-context control.

## Android Back behavior

When the omnibox is being edited, Android Back first leaves omnibox editing and dismisses the software keyboard. Otherwise, Browser Back navigates web history when history is available; if not, Android handles leaving the activity.

## Glaze on Android

The Android beta maps Browser-owned chrome to the current **Glaze V1.7 / 1.7.0 Anchor** contract using native Android controls. V1.7.0 is a bounded release that inherits the accepted V1.6.0 runtime behavior. Browser acceptance still depends on Browser-specific implementation and evidence; the shared Glaze Anchor does not by itself promote Browser lifecycle status.

Browser remains migration-required/not accepted until repository-local rendered/native visual, accessibility, representative-device/posture, large-text, RTL/localization, reduced-effects, performance, rollback, workflow, and production evidence is accepted. Glaze presentation never creates Browser authorization, privacy/security truth, provider precedence, or execution authority.

## Security behavior

The Android beta intentionally fails closed in several areas while the full GoreeCloud platform integrations are being completed:

- TLS/certificate errors are cancelled rather than bypassed.
- Android Safe Browsing is enabled; detected unsafe navigation returns to safety.
- Mixed-content loading is disabled.
- WebView file access and content access are disabled.
- Third-party cookies are disabled by default.
- Credential-bearing URLs, unsupported schemes, control-character injection, zero/out-of-range ports, and ambiguous/invalid numeric IPv4 host forms fail closed before ordinary navigation.
- Internationalized DNS hosts are converted to lowercase ASCII A-label form before navigation and unfocused address display. This canonicalization does not by itself establish DNS, certificate, reputation, registrable-domain, or Unicode-confusable trust.
- Website permission requests are denied until Browser-owned permission and policy surfaces are integrated.
- Geolocation permission requests are denied.
- Downloads are blocked until the Android path can satisfy the authoritative Wardveil download verification and release contract.

These behaviors do not mean the beta has completed Wardveil Security production acceptance. Wardveil status must remain tied to actual authenticated runtime evidence.

## Privacy behavior

The beta uses privacy-protective defaults where a complete user-controlled Privacy Shield workflow does not yet exist. Third-party cookies are disabled and site permission grants fail closed. The Browser menu's Privacy & security sheet makes these local defaults and unavailable-provider boundaries visible without claiming Privacy Shield or Wardveil runtime acceptance.

Android now also provides a Browser-owned Clear browsing data control for local WebView state. This is a local deletion mechanism, not evidence of complete Privacy Shield, profile isolation, Private/Isolated Private mode, or Close & Forget acceptance.

The Android beta does not yet provide the complete production Privacy Shield filtering, consent, diagnostics, private-browsing isolation, or user-control surface required for Stable release.

## Website permissions

Camera, microphone, geolocation, and other website permission requests are currently denied. There is no beta override that silently grants them.

Browser-owned permission prompts and the required Privacy Shield/Wardveil policy integration remain future beta work. A site that depends on these permissions may therefore have reduced functionality in the current beta.

## Downloads

Downloads are currently blocked. This is intentional.

GoreeCloud Browser already has a Browser-to-Wardveil download release architecture in its native core. Android downloads remain unavailable until the Android transfer path can stage downloaded bytes, bind the exact digest to authoritative Wardveil evidence, and release or hold the file according to the accepted security decision without creating a bypass.

## External links

Ordinary HTTP and HTTPS links stay in GoreeCloud Browser. A non-web URI triggered by an explicit user gesture may be handed to an installed Android application that can handle that URI. If no application can handle it, Browser reports that the link cannot be opened.

File, JavaScript, and other non-web schemes are not accepted as ordinary Browser web navigation.

## Current limitations

The Android beta does not yet claim:

- production signing or managed beta signing continuity;
- production or Stable readiness;
- complete Glaze V1.7 native-device acceptance;
- complete Wardveil Security runtime acceptance;
- complete Privacy Shield runtime acceptance;
- Everkeep backup/recovery acceptance;
- private-browsing and Close & Forget acceptance;
- Android download/file-upload acceptance;
- Browser-owned website-permission prompts;
- complete Android multi-tab/session/settings surfaces;
- production GoreeCloud Identity, Vault, Sync, DNS, Network, Mesh, or Everkeep adapters;
- Play Store or other store publication;
- signed update, downgrade, rollback, or application-data migration acceptance;
- sustained real-device performance, battery, compatibility, and accessibility acceptance.

## Reporting beta problems

When reporting an Android beta problem, include the Browser version, Android version, device model, Android System WebView version, what you attempted, the expected result, and the observed result. Do not include passwords, authentication tokens, private browsing content, or other reusable secrets in bug reports.

## Acceptance language

A successful GoreeCloud Browser Android CI run proves only the checks performed by that workflow for the exact source revision: unit tests, Android lint, APK assembly, signature/package verification, checksum generation, and artifact creation. It does not by itself establish production security, privacy, accessibility, real-device compatibility, recovery, or Stable qualification.

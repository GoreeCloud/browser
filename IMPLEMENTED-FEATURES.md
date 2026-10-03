# GoreeCloud Browser — Implemented Features

**Record type:** Repository implemented-feature inventory  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Authority:** Current `main` source and accepted repository evidence  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## Interpretation

This record describes capabilities present in the current GoreeCloud Browser Development source. It does **not** establish production deployment, accepted Browser-wide GLAZE UI conformance, representative-device acceptance, Release Candidate, or Stable qualification.

`FEATURES.md` remains the product-facing feature description. This file is the lifecycle authority for what is implemented.

## Implemented Development capabilities

### Browser core and authority boundaries

- GoreeCloud-owned engine-independent browser core and replaceable Browser Engine Layer.
- Browser-owned Android logical tab-session foundation with stable logical tab IDs independent from transient WebView identity, one exact active tab, a 32-tab ceiling, deterministic active-tab fallback after close, last-tab protection, exact-tab URL/title updates, bounded identifiers/locations/titles, and fail-closed invalid location mutation. This is a model foundation only; it is not yet wired to multi-WebView Android chrome, persistence/recovery, or privacy-context lifecycle.
- Pinned Linux x86_64 CEF render-candidate bootstrap for CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83`, with repository-owned SHA-1/SHA-256 archive identity enforcement, official-sidecar verification, safe extraction, immutable-cache/provenance validation, exact-version CMake enforcement, and one-command Development build/launch scripts.
- The integrated Linux CEF/Chromium Development renderer preserves the X11/XWayland native child-window path and adds a windowless/off-screen path for GTK backends that cannot expose an X11 child handle. The windowless path implements `CefRenderHandler` sizing and BGRA `OnPaint` delivery into a Browser-owned software frame sink painted by the GTK web drawing area.
- Exact-head Ubuntu 22.04 validation for the integrated renderer required successful HTTPS main-frame completion and a materially non-uniform software-rendered frame. Representative owner-device Zorin OS 17.3 Wayland evidence additionally verified real `https://example.com/` page pixels inside the Browser content surface without the former compatibility placeholder.
- The software-rendered Wayland path forwards GTK pointer motion/enter/leave, left/middle/right button presses including multi-click counts, wheel input, and focus state through a Browser-owned native-surface input bridge into CEF. Windowless CEF context-menu requests are serialized into an engine-independent Browser-host request, presented as a Glaze-styled GTK popover, and return selected command IDs or explicit cancellation through the CEF callback while X11/XWayland keeps the native child-window path. Keyboard/IME, cursor propagation, broader popup behavior, drag-and-drop, clipboard, accessibility, and sustained interaction acceptance remain open.
- CEF browser-process initialization receives the host Linux `argc/argv`; X11 native child-window embedding uses the CEF integral window-handle type; persistent profile paths resolve under the Browser cache root.
- Linux GTK3 native beta shell build path with automatic X11 child-surface selection or software windowless fallback.
- Linux Development tab chrome with multi-tab presentation plus create, activate, explicit close, cyclic next/previous navigation, keyboard-first shortcuts for common tab/navigation actions, Ctrl+K/F6 location focus, and Escape dismissal for Browser-owned panels.
- First-party Linux New Tab/Home search field and wired Bookmarks, Downloads, and Settings quick actions.
- Semantic Linux Browser panels for Bookmarks, Reader Mode, Privacy Shield, Wardveil Security, Clipboard, DNS, Proxy, Search-unavailable, and Development Downloads state without manufacturing provider-owned authority.
- Structured Linux Settings section presentation for current Browser-owned settings taxonomy; functional controls remain acceptance-gated.
- Friendly Browser-owned internal-page title/location presentation that suppresses implementation-only `goreecloud://` locations from ordinary visible chrome.
- Canonical Browser compass branding synchronized from `GoreeCloud/branding-assets` full-color and monochrome identity sources.
- GLAZE UI V1.6 / 1.6.0 source mapping with exact accepted release/qualification anchors, Android-native V1.6 mapping, and a Browser-owned Linux desktop Development presentation tranche: branded chrome, compact active-tab surface, primary navigation capsule, bounded secondary tools popover, responsive first-party cards, friendly internal-page titles, and redesigned first-party internal surfaces.
- GoreeCloud Search integration as the sole integrated query authority while direct structurally valid HTTP(S) navigation remains independent from search.
- Fail-closed first-party capability evidence that does not manufacture Search, Vault, Sync, Identity, Mesh, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, or Bookmarks authority.
- Browser-owned Sync submission/retrieval contracts with bounded validation, pagination, privacy-safe tombstones, and signer-shape checks.
- Advanced Download Manager core with Wardveil-oriented staged-transfer/release-gate architecture; production downloads remain blocked until accepted Wardveil verification exists.
- Privacy-safe local session-recovery checkpoint/candidate core excluding Private and Isolated Private windows before persistence.
- Privacy-safe local liveness/readiness contract separating process health from release readiness.

### Android beta foundation

- Installable Development APK target with Android API 26 minimum and API 35 target.
- Android System WebView/Chromium rendering dependency behind GoreeCloud-owned product policy and behavior.
- Back, Forward, Reload, Go, unified address/search field, progress state, and web-content region.
- Direct HTTP/HTTPS navigation, HTTPS upgrade for bare hosts, and GoreeCloud Search intent classification for non-URL input.
- Direct-navigation canonicalization rejects credential-bearing URLs, unsupported schemes, control-character injection, zero/out-of-range ports, and ambiguous or invalid numeric IPv4 host forms before WebView navigation.
- Internationalized Android HTTP(S) DNS host identity is canonicalized to lowercase ASCII A-label form before direct navigation and Browser-owned unfocused address presentation; path, query, fragment, explicit valid ports and trailing DNS root dots are preserved, while malformed STD3 labels and bracketed non-IPv6 authorities fail closed.
- Free-text Search and omnibox classification reject C0/C1 and narrow Unicode Bidi_Control formatting characters before trimming, enforce a 2,048-character Search-query maximum, and reject Search capability claims above the Browser-supported 100-result maximum before request construction; this does not enable remote Search.
- Android Browser-owned page-title and unfocused-address presentation strips Unicode bidirectional formatting controls before rendering chrome text while retaining the unchanged full URL as navigation authority.
- Browser-intent handling for HTTP/HTTPS links.
- TLS errors fail closed; Android Safe Browsing enabled with return-to-safety behavior.
- Mixed content disabled, third-party cookies disabled by default, and WebView file/content access disabled.
- Browser-owned Site information and Privacy & security sheets expose the current origin/transport and enforced/fail-closed Android protection state without manufacturing certificate, Privacy Shield, Wardveil, Policy, or Search authority.
- Page-address Copy/Share disclosure is restricted to canonical Browser-approved HTTP(S) URLs; Browser-owned local/resource addresses are not placed on the clipboard or sent through Android sharing.
- Android Find in page uses the active WebView's local find facility with bounded input, live match status, previous/next navigation, and match cleanup when the Browser-owned surface closes; the find text is not delegated to Search or another provider.
- Browser-owned page text zoom supports 75% through 200% in 25-point steps and persists the selected value as an application-local Android preference without changing system font scale or claiming Browser Sync.
- Android Desktop site mode is Browser-owned, session-local, restores across Activity recreation, derives a desktop-style user agent from the current WebView engine identity, enables wide-viewport/overview presentation, and does not persist or claim Sync authority.
- Android Clear browsing data requires an explicit confirmation and clears WebView cookies/sign-in state, HTML5 website storage, cache, form data, navigation history, and SSL preferences, then returns to the local start page while preserving Browser settings and Android app permissions.
- Android Page controls groups page text size, Desktop site, JavaScript, and automatic image loading into one Glaze surface. JavaScript and image-loading choices are session-local, survive Activity recreation only, reload ordinary web pages when changed, and default to enabled for compatibility.
- Android first-use guidance provides a required resumable three-step fresh-install setup, durable local setup progression using synchronous preference commits, a compact dismissible Browser-chrome tip, and one grouped Guidance & tips surface for replay/tip controls. The state is device-local and adds no account, telemetry, permission, Search, or provider authority.
- Website permissions/geolocation denied until Browser-owned policy surfaces and authority adapters are accepted.
- Downloads remain fail-closed until the Wardveil release contract is satisfied.
- CI covers Browser-owned navigation behavior, Android lint/build, package/signature verification, SHA-256, artifact provenance, and an Android 15 managed-emulator smoke lane for Browser-owned chrome, conservative WebView defaults, and fail-closed local Search behavior.

### PermissionBroker Development foundation

- Platform-neutral website-permission request/profile/privacy-context/tab/origin binding.
- Typed camera, microphone, geolocation, protected-media, and MIDI SysEx resources.
- Duplicate/invalid request rejection, request expiry, deterministic lifecycle, and final context revalidation.
- Independent host-OS state handling and fail-closed Policy, Privacy Shield, and Wardveil authority gates.
- Explicit user allow/deny scopes without automatic engine grant before all gates pass.
- Persistent permission decisions prohibited in Private and Isolated Private contexts.
- Request/context-close cancellation and core smoke coverage.

### Native extension Development foundation

- Native manifest/API v1 models and `.gcex` package inventory validation.
- Bounded GCEX package-byte reader and strict manifest decoding.
- Closed native extension-permission vocabulary and fail-closed authorization model.
- Permission ledger covering one-shot, one-hour, tab/session, website, browser-session, profile binding, expiry, and revocation semantics.
- Bounded durable permission-ledger snapshot/recovery for approved lifetimes with corruption/clock/profile checks.
- Bounded package-local Ed25519 signature-verification foundation; a valid package signature can establish only package-signature state, not developer identity or production trust.

## Implemented-but-not-accepted boundaries

The following foundations exist but remain acceptance-gated and therefore also appear in `PLANNED-FEATURES.md`:

- Android Browser runtime without representative-device/accessibility/performance/production acceptance.
- Current repository source mapping targets GLAZE UI V1.6 / 1.6.0, but Browser-specific rendered/accessibility/adaptive/device/performance/rollback/Human Visual Excellence acceptance remains incomplete.
- Session recovery without authenticated encrypted persistence, lifecycle wiring, restore execution, Everkeep integration, and runtime acceptance.
- PermissionBroker without Android prompt/OS/live-authority adapters and durable Normal-context decision storage.
- Native extension package/signature/permission foundations without installation, execution, sandboxing, privileged APIs, trusted developer-key authority, or production acceptance.

## Maintenance rule

When an obligation in `PLANNED-FEATURES.md` becomes implemented and verified on the authoritative integration line, reconcile it here and record the material change in `CHANGELOGS.md`. Draft or unmerged pull requests are not implementation authority.
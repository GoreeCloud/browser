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
- The software-rendered Wayland path forwards GTK pointer motion/enter/leave, left/middle/right button presses including multi-click counts, wheel input, focus state, direct key events, GTK IME text composition, and cursor changes through Browser-owned engine-independent native-surface contracts into CEF. The direct-keyboard bridge maps common letters, digits, punctuation, editing/navigation keys, function keys and keypad keys to normalized virtual/native key events and emits printable BMP character events with Shift/Caps state; Defined Browser-owned Control shortcuts for location/search focus, tab creation/close/cycling, and refresh plus F5/F6 remain ahead of page forwarding; other Control combinations are forwarded to page content without synthesizing printable character events while Control is held; non-conflicting Alt combinations are likewise forwarded without character synthesis while Browser-owned Alt+Left/Right/Home remain reserved, and Super/Meta combinations remain reserved for follow-up policy. The GTK `GtkIMMulticontext` bridge forwards UTF-16 preedit composition, selection, committed text, cancellation, and focus/reset lifecycle into CEF OSR through `ImeSetComposition`, `ImeCommitText`, and `ImeCancelComposition`. Standard cursors are applied through GTK diagnostics, while custom CEF cursor bitmaps are bounded to 512×512, copied out of transient engine memory, hotspot-clamped, carried through the same Browser-owned cursor boundary, converted from BGRA to RGBA, and presented through GTK; reported image scale is retained as bounded diagnostic metadata. Windowless CEF context-menu requests are serialized into a Browser-host request, presented as a Glaze-styled GTK popover, and return selected command IDs or explicit cancellation while X11/XWayland keeps the native child-window path. CEF windowless popup lifecycle and `PET_POPUP` pixels are carried through the Browser-owned software-frame boundary and composited over the GTK page surface with explicit hide/detach cleanup. Popup logical geometry is carried separately from popup pixel-buffer dimensions; GTK clamps the displayed popup into the visible content view, scales the engine pixel buffer into the logical rectangle, and remaps pointer coordinates back to CEF coordinates when clamping displaces the popup. The pinned CEF/Xvfb runtime also completes an ordinary page clipboard regression through the same Control-key bridge: Ctrl+A/C copies the selected source-field value, Tab transfers page focus, Ctrl+V pastes into a target field, and the DOM observes the exact pasted value. The Development external-drop path accepts copied UTF-8 text up to 1 MiB or exactly one `http`/`https` URI up to 8 KiB from GTK, copies the payload before crossing the engine-neutral boundary, and completes CEF OSR DragEnter/DragOver/Drop only after renderer acceptance; local `file://` URIs, filenames, paths, and file contents are intentionally unrepresentable. The pinned CEF/Xvfb external-drop regression requires local-file URI rejection plus page-observed accepted text and safe-web-link drops before fixed success navigation. Representative native-Wayland popup interaction and placement, native widget interoperability, representative IME/dead-key/non-BMP/international input, high-DPI custom-cursor behavior, representative non-conflicting Control/Alt shortcut acceptance, Super/Meta page-shortcut policy, representative safe external-drop acceptance, live drag-hover feedback, outgoing page drags, file-drop policy/implementation, native-Wayland clipboard-manager/primary-selection/private-context behavior, accessibility, and sustained interaction acceptance remain open.
- Linux windowless IME candidate-position source bridge: CEF `OnImeCompositionRangeChanged` character bounds are converted to a bounded engine-neutral caret rectangle and forwarded to GTK `GtkIMContext` cursor location, without exposing CEF types through the native-surface boundary. Representative IBus/Fcitx/XIM candidate-window placement remains unaccepted.
- Linux CEF renderer-loss recovery: recoverable abnormal termination, kill, crash, or OOM clears transient popup/drop/media state and permits one automatic page reload; a successful main-frame load resets the guard, while repeated pre-recovery termination plus child-launch or code-integrity failure remain fail-closed. The pinned CEF/Xvfb regression kills a Browser renderer descendant, requires termination/reload diagnostics, and requires a second successful HTTP 200 main-frame completion. Representative repeated-failure and user-facing recovery acceptance remain open.
- CEF browser-process initialization receives the host Linux `argc/argv`; X11 native child-window embedding uses the CEF integral window-handle type; persistent profile/site data can be separated from the immutable CEF runtime through a Browser-owned user-data root, with installed Linux launchers resolving that root under XDG data while Development builds retain the historical runtime-tree fallback.
- Private-only Browser startup now defers creation of the normal persistent engine context until a normal window is actually requested. Private and Isolated Private startup continue to use cache-path-free, non-persistent private request contexts; Release smoke requires the persistent default context to be absent after private initialization and created only after a later normal-window request. Browser-owned private windows carry exact private-session identity, and session teardown destroys all windows for the target private session before destroying that ephemeral context so no `WindowController` can retain a dangling engine-context reference; Release smoke proves teardown isolation across two private sessions plus a Normal window. Installed CEF runtime validation separately requires a live private session to render successfully without materializing the normal `profile` directory under the XDG user-data root. Engine-level cookie/site-data/permission/authentication purge completion, private clipboard isolation, user-facing Close & Forget, and representative privacy acceptance remain open.
- Linux Development install-runtime staging with a prefix-relocatable public launcher, private libexec Browser/CEF subprocess, exact pinned CEF binary/resource payload, `$ORIGIN` library resolution, private runtime branding, and GTK program identity aligned to `io.goreecloud.Browser`. Exact-branch physical Zorin/GNOME Wayland validation built the branch, passed all 13 CTests, installed the complete runtime, launched through the installed wrapper, loaded `https://example.com/` with HTTP 200, presented a materially non-uniform windowless frame, and wrote persistent Chromium data beneath the dedicated XDG data root rather than the private runtime. Production distro packaging, sandbox-helper ownership/mode, signing, upgrade/uninstall, reproducibility, and production acceptance remain open.
- Linux GTK3 native beta shell build path with automatic X11 child-surface selection or software windowless fallback.
- Linux Development tab chrome with multi-tab presentation plus create, activate, explicit close, cyclic next/previous navigation, engine-independent tab reordering with active-tab identity preservation, Ctrl+Shift+PageUp/PageDown keyboard reorder actions, keyboard-first shortcuts for common tab/navigation actions, Ctrl+K/F6 location focus, Escape dismissal for Browser-owned panels, and tab-specific accessible names for close controls so assistive technologies can identify which tab will close.
- First-party Linux New Tab/Home search field and wired Bookmarks, Downloads, and Settings quick actions.
- Semantic Linux Browser panels for Bookmarks, Reader Mode, Privacy Shield, Wardveil Security, Clipboard, DNS, Proxy, Search-unavailable, and Development Downloads state without manufacturing provider-owned authority.
- Structured Linux Settings section presentation for current Browser-owned settings taxonomy; functional controls remain acceptance-gated.
- Friendly Browser-owned internal-page title/location presentation that suppresses implementation-only `goreecloud://` locations from ordinary visible chrome.
- Canonical Browser compass branding synchronized from `GoreeCloud/branding-assets` full-color and monochrome identity sources.
- Glaze V1.7 / 1.7.0 bounded source mapping with exact release/qualification authority, inherited accepted V1.6.0 runtime semantics, Android-native V1.7 mapping, and a Browser-owned Linux desktop Development presentation tranche: branded chrome, compact active-tab surface, primary navigation capsule, bounded secondary tools popover, responsive first-party cards, friendly internal-page titles, and redesigned first-party internal surfaces.
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
- Current repository source mapping targets Glaze V1.7 / 1.7.0 with inherited V1.6.0 runtime behavior, but fresh Browser-specific rendered/accessibility/adaptive/device/performance/rollback/Human Visual Excellence acceptance remains incomplete.
- Session recovery without authenticated encrypted persistence, lifecycle wiring, restore execution, Everkeep integration, and runtime acceptance.
- PermissionBroker without Android prompt/OS/live-authority adapters and durable Normal-context decision storage.
- Native extension package/signature/permission foundations without installation, execution, sandboxing, privileged APIs, trusted developer-key authority, or production acceptance.

## Maintenance rule

When an obligation in `PLANNED-FEATURES.md` becomes implemented and verified on the authoritative integration line, reconcile it here and record the material change in `CHANGELOGS.md`. Draft or unmerged pull requests are not implementation authority.
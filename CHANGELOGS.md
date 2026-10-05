# GoreeCloud Browser — Changelogs

## 2026-10-05 — Linux packaging metadata validation

### Added

- Added fast CI validation for the installed desktop entry with `desktop-file-validate`.
- Added offline AppStream metadata validation with `appstreamcli validate --no-net`.
- Installed only the standard Ubuntu validation packages in the existing GTK beta-shell lane so invalid launcher/AppStream metadata fails before packaging promotion.

### Acceptance boundary

This is repository packaging-metadata quality evidence only. It does not establish distro packaging, launcher visual acceptance, signing, store publication, production approval, Stable, Seal, or Anchor qualification.


## 2026-10-05 — Linux private Close & Forget source candidate

### Added

- Added a private-only Glaze GTK overflow action labeled `Close & Forget` with a private-session-specific accessible name.
- Added an explicit confirmation that the action closes every Browser window in the current private session and destroys its non-persistent Browser context while warning that data intentionally saved or copied outside the session is not removed.
- Added two-phase runner lifetime handling: the GTK host closes and detaches the active engine view first, the event loop stops, and only then does Browser destroy the exact private session.
- Reused the existing Release-tested `BrowserApplication::close_private_session` primitive rather than adding a second cleanup path.
- Added Core CI source-contract checks for private-only UI wiring and exact-session teardown invocation.

### Privacy boundary

This command destroys Browser-owned windows and the non-persistent private request context for the exact session. It does not manufacture unsupported cookie/storage/history/permission deletion success and does not remove data the user intentionally persisted outside the private session.

### Acceptance boundary

Development source candidate only. Representative Close & Forget interaction on native Wayland, engine-level deletion where explicitly supported, private clipboard isolation, crash/recovery cleanup, production privacy acceptance, Stable, Seal, and Anchor remain open.


## 2026-10-05 — Linux same-process pointer tab-reorder candidate

### Added

- Added an engine-independent relative tab-reorder primitive that moves a source tab before or after a target while preserving active-tab identity.
- Added Release-active three-tab smoke coverage for both reorder directions, self/no-op handling, and invalid-target rejection.
- Added a GTK same-process pointer-drag path on tab title controls using the private `application/x-goreecloud-tab-id` target and only opaque Browser tab IDs.
- Added before/after selection from the target midpoint plus bounded 256-byte payload parsing and truthful drag completion based on Browser-model acceptance.
- Added runtime diagnostics containing only source/target IDs, side, and acceptance; page URLs and engine objects do not cross the tab-drag payload.
- Added Core CI source-contract assertions for model, runner, GTK host, drag target, and Release smoke coverage.

### Acceptance boundary

Development source candidate pending exact-head hosted and physical-Linux validation. Representative native-Wayland pointer/drag reorder interaction, drag accessibility beyond the existing keyboard alternative, sustained-use behavior, production approval, Stable, Seal, and Anchor remain open.


## 2026-10-05 — Fail-closed CEF cleanup reporting candidate

### Changed

- CEF request-context cleanup no longer reports unsupported data classes as successfully cleared.
- `clear_all_data` now accepts only the currently implemented HTTP-cache class and fails before side effects when cookies, storage, history, authentication, permissions, or other unsupported classes are requested through that aggregate path.
- Origin-scoped authentication cleanup now fails closed because pinned CEF exposes only context-wide HTTP-auth credential clearing.
- Chromium engine capability reporting no longer advertises site-data control, origin-scoped cleanup, or permission-state cleanup that the pinned CEF delegate cannot perform.

### Privacy boundary

This corrects Browser/engine authority reporting; it does not add cookie, local-storage, IndexedDB, service-worker, history, permission, or origin-scoped authentication deletion support. Private-context destruction remains separate from explicit engine-data cleanup, and user-facing Close & Forget remains acceptance-gated.

### Acceptance boundary

Development correctness candidate pending exact-head hosted validation and physical-Linux build/CTest evidence. Production privacy acceptance, Close & Forget, and representative private-session cleanup remain open.



## 2026-10-05 — Linux safe external web-link drop runtime evidence candidate

### Changed

- Extended the existing real-GTK external-drop regression so the page must observe both an accepted plain-text drop and an accepted safe `https://` URI drop before producing the fixed success navigation.
- Preserved the existing pre-engine rejection check for local `file://` URIs.
- Reused the existing Browser-owned drop implementation unchanged; this candidate adds evidence only.

### Acceptance boundary

If exact-head CI passes, this establishes pinned CEF/Xvfb automated evidence for file-URI rejection plus accepted external text and safe-web-link delivery. Representative native-Wayland drag/drop behavior, live drag-hover feedback, outgoing page drags, file-drop support/policy, accessibility, sustained performance, production approval, Stable, Seal, and Anchor remain open.


## 2026-10-04 — Private startup and session teardown minimization candidate

### Changed

- Deferred creation of the normal persistent Browser engine context when startup is Private or Isolated Private.
- Kept private startup on the existing cache-path-free, non-persistent private request-context path.
- Added Release-active Browser core coverage requiring no default persistent context after private startup and lazy creation only when a normal window is later requested.
- Bound Browser-owned private windows to their exact private-session identity.
- Replaced unsafe context-only teardown with session-scoped teardown that destroys every Browser-owned window for the target private session before destroying its non-persistent engine context, preventing dangling `EngineContext` references while preserving Normal and unrelated private sessions.
- Expanded Release-active smoke coverage to two private sessions plus a Normal window, including shared-session multi-window teardown, cross-session isolation, repeated/missing-session behavior, and preservation of the Normal context.
- Added an installed CEF runtime regression that keeps a private session live through a successful HTTPS page load and requires the ordinary persistent `profile` directory to remain absent from the XDG user-data root.

### Privacy boundary

This removes unnecessary normal-profile materialization during private-only startup and makes Browser-owned multi-window private-session teardown structurally safe and session-scoped. It does not yet establish engine-level cookie/site-data/permission/authentication purge completion, private clipboard isolation, crash/recovery cleanup, user-facing Close & Forget, or representative native-device privacy acceptance.

### Acceptance boundary

Development candidate pending fresh exact-head Core, security/evidence/signature, installed-runtime, and physical-Linux build/CTest validation. Production approval, Stable, Seal, and Anchor remain open.


## 2026-10-04 — Representative Linux CEF runtime evidence

- Built exact Browser source `16d02d80292fea225308b07ff69d947286dbb193` / tree `59a60318a3dd30c6f3d07d39d22f0d062b5fa4a4` on the owner's Zorin OS 17.3 x86_64 laptop using pinned CEF `152.0.6+g708dc14+chromium-152.0.7977.83`.
- Recorded exact Browser artifact SHA-256 `29016e07b63ad4e02f847e725f7f6d53f9377729c2e30970bfb78994707b0ab0`, subprocess SHA-256 `36cd547c0c69361f3238e053dd9ad36d090e0231058141587129057c85c11af7`, and `libcef.so` SHA-256 `9575a379b967d8efb42e763aaecebbb39665b3c233016b3f8a5efc47ae89ca2f`.
- The exact render-capable build completed and passed all 13 repository CTest smoke tests.
- On the representative GNOME/Zorin Wayland session, normal and isolated-private startup paths initialized the exact CEF runtime, presented nonuniform Browser-owned 1280×880 frames, and completed `https://example.com/` with HTTP 200.
- Preserved observed sandbox-helper, VAAPI, and NSS diagnostics rather than converting the successful page load into broad security/privacy acceptance.
- A deterministic cookie probe did not demonstrate durable normal-cookie persistence across the externally timed lifecycle; because the process was timeout-terminated rather than gracefully shut down, this is retained as an unresolved lifecycle/isolation gap rather than a defect claim.
- Added an explicit Privacy Shield FR-013 dimension gap map. Browser remains Development and not production approved.

**Record type:** Repository change history  
**Repository:** `GoreeCloud/browser`  
**Lifecycle:** Development / non-Stable  
**Governing standard:** Standard — Repository Feature Tracking and Changelog Governance v1.0, effective September 22, 2026.

## 2026-10-05 — Linux searchable tab switcher candidate

### Added

- Added a compact Browser-owned tab-search glyph beside the Linux tab strip.
- Added a Glaze-styled GTK popover with an accessible search field and scrollable matching-tab results.
- Filter tab titles with UTF-8 case folding so title search is case-insensitive without exposing page URLs.
- Preserve pinned and active visual indicators in the result list, provide an explicit empty state, and activate the exact Browser tab ID selected by the user.
- Added Ctrl+Shift+A as a keyboard-first entry point that opens and focuses the tab-search popover, plus Enter activation of the first visible filtered result.
- Added Core CI source-contract guards for the tab-search control, accessible labels, Unicode folding, rendering path, keyboard shortcut/activation path, and empty-result state.

### Acceptance boundary

Development source and compile/source-contract coverage only. The keyboard shortcut and Enter-first-result source paths are implemented, but representative native-Wayland keyboard interaction, large tab sets, focus traversal, large text, screen-reader behavior, localization/RTL, performance, production approval, Stable, Seal, and Anchor qualification remain open.

## 2026-10-05 — Linux repeated renderer failure surface

### Added

- Added a Browser-owned `goreecloud://renderer-failure` recovery surface for a second recoverable CEF renderer termination before the first automatic recovery completes.
- Preserved the existing single automatic reload for the first abnormal/killed/crashed/OOM renderer termination.
- Added a recovery-exhausted guard so repeated failures cannot create an automatic reload loop.
- Added a deterministic pinned-CEF Xvfb regression backed by a local HTTP server that deliberately holds the recovery request, kills the replacement renderer, and requires exactly one automatic reload followed by the local failure surface.
- Extended that regression to send an explicit Browser Ctrl+R retry from the local failure surface, require the original page to recover without consuming a second automatic retry, then kill the recovered renderer and require exactly one fresh automatic recovery; the harness counts only top-level document requests so favicon traffic cannot consume the delayed-recovery slot and explicitly focuses the Xvfb GTK window before injecting the shortcut.

### Acceptance boundary

This establishes Development repeated-failure behavior and automated runtime evidence. Representative native-Wayland crash UX, manual retry behavior, accessibility, repeated long-running failure patterns, production recovery policy, Stable, Seal, and Anchor qualification remain open.

## 2026-10-05 — Linux pinned-tab foundation

### Added

- Wired normal Browser windows to the in-memory Advanced Tab Manager with automatic managed-tab registration and unique Browser window identities.
- Added manager-backed pinned-state readback to Browser chrome.
- Added stable pinned-first tab ordering and fail-closed reorder boundaries so pinned and ordinary tabs cannot be dragged or keyboard-reordered across each other accidentally.
- Added compact GTK glyph pin/unpin controls with accessible action names and pinned-tab visual state.
- Added Release-active smoke coverage for automatic registration, pin/unpin ordering, boundary rejection, active-tab identity, manager state, and Browser-chrome pinned presentation.

### Privacy and architecture boundary

Private/Isolated Private windows remain outside the ordinary Advanced Tab Manager in this tranche. Pin state is kept in the Browser-owned tab manager rather than duplicated into engine-owned tab objects.

### Acceptance boundary

This is Development source and automated model/UI coverage. Representative native-Wayland pinned-tab interaction, pointer-drag behavior across mixed pinned/ordinary tabs, tab overflow/search, groups, persistence/recovery, accessibility, production approval, Stable, Seal, and Anchor qualification remain open.

## 2026-10-04 — Bounded Linux CEF renderer crash recovery candidate

### Added

- Added CEF request-handler coverage for unexpected renderer termination.
- Recoverable abnormal termination, process kill, crash, or OOM now clears Browser-owned transient popup/drop/media state and permits exactly one automatic reload attempt.
- A successful main-frame load resets the recovery guard; repeated termination before recovery is suppressed.
- Child launch failure and code-integrity failure remain fail-closed and are not auto-reloaded.
- Added a pinned-CEF Xvfb runtime exercise that kills a renderer subprocess and requires termination diagnostics, the bounded reload path, and a second successful HTTPS main-frame load.

### Acceptance boundary

Integrated Development behavior. Exact-head hosted renderer-kill recovery passed before integration and required a second successful HTTPS main-frame load after one bounded recovery reload. Representative user-facing crash/error surfaces, repeated-crash behavior, process-hang handling, private-context cleanup, production approval, Stable, Seal, and Anchor remain open.

## 2026-10-04 — Linux Wayland IME candidate-position geometry

### Added

- Added an engine-neutral text-input geometry sink for software/windowless Browser surfaces.
- Mapped CEF `OnImeCompositionRangeChanged` caret position and view-coordinate character bounds into a bounded logical cursor rectangle.
- Routed that rectangle into GTK `gtk_im_context_set_cursor_location` so native IME candidate UI receives Browser content geometry without exposing CEF types to GTK.
- Added Core CI source-contract assertions for the CEF callback, Browser-owned geometry contract, GTK cursor-location call, and diagnostics.

### Acceptance boundary

This is Development source/compile coverage pending fresh exact-head validation. Representative IBus, Fcitx, XIM, compositor-specific candidate-window placement, surrounding-text/delete-surrounding behavior, accessibility, production approval, Stable, Seal, and Anchor remain open.

## 2026-10-04 — Linux popup logical geometry on drop-capable packaged runtime

### Changed

- Restacked logical popup geometry onto the integrated external-drop and self-contained CEF runtime baseline.
- Kept popup logical view geometry separate from pixel-buffer dimensions, clamped displayed rectangles into the visible GTK content area, scaled popup pixels into logical bounds, and remapped pointer coordinates back into CEF view coordinates when clamping displaces the popup.
- Preserved external-drop source/validation, packaged-runtime launch behavior, popup lifecycle cleanup, and the existing popup selection regression.

### Acceptance boundary

Development restack candidate pending fresh exact-head validation. Representative native-Wayland placement, high-DPI/scale-factor rendering, pointer selection across real display scales, native-widget interoperability, accessibility, sustained performance, production approval, Stable, Seal, and Anchor remain open.


## 2026-10-04 — Linux bounded external-drop restack on packaged runtime

### Added

- Restacked the Browser-owned external-drop contract onto the self-contained Linux CEF install-runtime baseline.
- Retained copied text up to 1 MiB and exactly one `http`/`https` URI up to 8 KiB, with local file URIs, paths, filenames, file contents, and multi-URI payloads intentionally outside the contract.
- Retained GTK drop-target integration and CEF OSR drag entry/over/drop completion only after renderer acceptance.
- Retained the real GTK drag-source Core CI exercise requiring file-URI rejection plus page-observed text-drop completion.

### Acceptance boundary

This is a Development restack candidate pending fresh exact-head Core, security, evidence, extension-signature, and post-integration validation on the packaged-runtime baseline. Representative native-Wayland drag/drop, accepted web-link runtime evidence, live hover feedback, outgoing page drags, file-drop support/policy, accessibility, production approval, Stable, Seal, and Anchor remain open.


## 2026-10-04 — Release-active Browser runtime smoke checks

### Changed

- Replaced the runtime smoke's 96 dynamic `assert(...)` checks with fail-fast `GC_REQUIRE(...)` checks that remain active when `NDEBUG` is defined.
- Added requirement diagnostics with the failed expression, source file, and line number.
- Added a Core CI source contract that requires the Release-active guard and rejects future dynamic `assert(...)` use in `runtime_smoke.cpp`.

### Acceptance boundary

This strengthens automated Development verification only. It does not change Browser product behavior or establish production qualification.

## 2026-10-04 — Linux CEF install-runtime staging

### Added

- Added a prefix-relocatable public Linux launcher and a private `libexec/goreecloud-browser` runtime containing the Browser binary, CEF subprocess, exact pinned CEF binary/resource payload, locales, and runtime branding.
- Added `$ORIGIN` install RPATHs for the private Browser and subprocess so the colocated `libcef.so` resolves without development-tree library paths.
- Added a Browser-owned writable Chromium/CEF user-data root separate from the immutable runtime payload. Installed launchers use an explicit override when provided, otherwise `XDG_DATA_HOME/goreecloud/browser/cef` or the standard `$HOME/.local/share` fallback; Development builds retain the existing runtime-tree fallback.
- Aligned the GTK program identity with `io.goreecloud.Browser` before GTK initialization for Wayland desktop association.
- Added Core CI source contracts plus a staged `cmake --install` runtime exercise that validates the private payload, models package-owned sandbox-helper permissions, launches through the public wrapper under Xvfb, requires CEF initialization and HTTPS 200, and proves profile data is written outside libexec.
- Added `docs/LINUX_PACKAGING.md` as the Development packaging/runtime contract.

### Physical Linux evidence

The exact packaging branch was configured against Glaze V1.7 and the pinned CEF runtime on the authorized Zorin/GNOME Wayland owner device, built successfully, passed all 13 CTests, staged the complete install tree, resolved `libcef.so` from the private `$ORIGIN` runtime, and launched `https://example.com/` through the installed wrapper on the active Wayland session. The run completed CEF initialization, received HTTP 200, painted a materially non-uniform 1280×723 windowless frame, and placed persistent Chromium data under the isolated XDG data tree with no `profile-cache` under the private runtime.

### Acceptance boundary

This is Development install/runtime staging, not a production Linux package. Signed/reproducible package generation, production sandbox-helper ownership/mode, upgrade/uninstall preservation, package-manager integration, representative desktop-environment validation, production signing/distribution, Release Candidate, Stable, Seal, and Anchor remain open.

## 2026-10-04 — Glaze V1.7 consumer mapping and documentation governance

### Changed

- Advanced Browser source identity guards and Android-native Glaze contract tests to the current bounded Glaze V1.7 / `1.7.0` Anchor release while preserving the accepted V1.6.0 runtime behavior as the inherited implementation and immediate rollback.
- Pinned the V1.7 Stable-contract qualification source `7c4ded83d7a8725165bb6a55dfb175667cc9589e` and release integration revision `1a5756daed2294155be2e9972b24f580f6222b7b` without importing V1.7.1 Development-only behavior.
- Updated Browser README, specification, feature inventories, Android beta/user documentation, and platform declaration to distinguish V1.7 source mapping from fresh Browser-local acceptance.
- Corrected the Android beta documentation artifact identity to `0.1.0-beta.1+android.12` / versionCode `10012`.
- Added canonical `docs/NOTES.md` and converted the stale root `NOTES.md` into a compatibility pointer.

### Acceptance boundary

This is Development source/contract/documentation migration. It does not establish Browser-local rendered/native Glaze V1.7 acceptance, representative-device accessibility or performance, production approval, Release Candidate, Stable, Seal, or Anchor product maturity.

## 2026-10-04 — Linux keyboard tab reordering foundation

### Added

- Added engine-independent tab reordering to `WindowController` while preserving the active tab by stable tab identity.
- Added bounded move-left and move-right operations for the active tab with no wrap at the strip boundaries.
- Added GTK tab actions and Ctrl+Shift+PageUp / Ctrl+Shift+PageDown shortcuts through the existing Browser-owned tab-action boundary.
- Strengthened the core runtime smoke to prove two-tab ordering changes in both directions while the same active tab remains active.

### Acceptance boundary

This is Development model and keyboard-shortcut support for tab reordering. Representative GTK shortcut interaction, pointer/drag tab-strip reordering, persistence of reordered session state, accessibility behavior during reordering, and the full render-capable Beta multi-tab gate remain open.

## 2026-10-04 — Linux tab-close accessibility labels

### Changed

- Replaced the generic `Close Tab` accessibility name on tab close buttons with a tab-specific label derived from the current tab title, while retaining `Close Tab` as the fallback when no title is available.
- Added a Core CI source-contract assertion so future Linux chrome changes cannot silently regress the per-tab accessible label.

### Acceptance boundary

This improves Browser-owned tab-chrome semantics but does not establish full ATK/AT-SPI, screen-reader, keyboard-navigation, high-contrast, large-text, or representative accessibility acceptance.

## 2026-10-03 — Linux windowless non-conflicting Alt shortcut forwarding

### Added

- Forwarded Alt-modified key press/release events from the GTK software/windowless surface to page content when the combination is not owned by Browser chrome.
- Preserved Browser-owned Alt+Left, Alt+Right, and Alt+Home navigation ahead of page forwarding.
- Reused the existing command-modified key policy so Alt shortcuts do not synthesize printable character events.
- Strengthened the pinned-CEF keyboard regression so page JavaScript must observe both Ctrl+E and Alt+E before ordinary typed text can satisfy the success condition.

### Acceptance boundary

This establishes a Development candidate for non-conflicting Alt page-shortcut forwarding in the pinned CEF/Xvfb environment. Super/Meta forwarding and representative native-Wayland shortcut acceptance remain open, along with production approval, Stable, Seal, and Anchor qualification.

## 2026-10-03 — Linux CEF runtime failure-log preservation

### Changed

- Added fail-only EXIT traps to all six Core CI CEF runtime exercises so Browser diagnostics are emitted even when an inner runtime assertion exits before the normal log-printing path.
- Covered sandboxed runtime startup plus windowless page/input, popup, direct keyboard/Control, clipboard, and GTK IME exercises.
- Preserved successful-run output behavior while improving first-failure evidence for transient or interaction-specific regressions.

### Acceptance boundary

This is CI diagnostics hardening only. It does not change Browser runtime behavior, make a previously failing interaction acceptable, or replace exact-head reruns and representative-device validation.

## 2026-10-03 — Linux windowless clipboard shortcut regression

### Added

- Added a pinned-CEF Xvfb runtime regression for ordinary page clipboard shortcuts using the already-integrated non-conflicting Control-key forwarding.
- The exercise selects `goreecloud` in a source field with Ctrl+A, copies it with Ctrl+C, moves page focus with Tab, pastes with Ctrl+V, and requires the target field DOM to observe the exact value before navigating to a fixed success marker.
- The first diagnostic run failed in the pre-existing custom-cursor/pointer/context-menu prerequisite before reaching clipboard; a targeted rerun passed that prerequisite, direct keyboard/Control, the clipboard exercise, and GTK IME on the unchanged exact head.

### Acceptance boundary

This establishes Development automated evidence for copy/paste in the pinned CEF/Xvfb environment. It does not establish representative native-Wayland clipboard-manager interoperability, primary-selection behavior, private-context cleanup, accessibility, production approval, Stable, Seal, or Anchor qualification.

## 2026-10-03 — Linux Wayland windowless popup surface candidate

### Added

- Added engine-neutral transient popup-frame delivery for software/windowless Browser surfaces, with popup coordinates kept relative to the web view rather than exposing CEF types to GTK.
- Added CEF OSR `OnPopupShow`, `OnPopupSize`, and `PET_POPUP` handling while preserving the existing `PET_VIEW` frame path.
- Added GTK popup-frame copying and compositing over the main Browser software surface using the same view coordinate scale.
- Added explicit popup cleanup on CEF hide, engine detach, and Browser-owned internal-surface transitions.
- Added a forced-windowless Xvfb runtime smoke that opens an engine-owned HTML `select` popup and requires CEF show/size/paint diagnostics plus GTK popup-frame presentation before Escape dismissal.

### Architecture boundary

CEF remains authoritative for popup lifecycle, rectangle, and pixels. Browser owns only the engine-neutral transient surface handoff and GTK presentation. Native X11/XWayland child-window behavior is unchanged.

### Acceptance boundary

The popup-surface source is integrated Development behavior and passed exact-head Core CI plus post-merge main validation. It does not establish representative native-Wayland popup placement, selection, dismissal, native-widget interoperability, accessibility, sustained performance, accelerated rendering, production approval, Stable, Seal, or Anchor qualification.

## 2026-10-03 — Linux Wayland non-conflicting Control shortcut forwarding

### Added

- Added selective software-surface routing for Control-modified page shortcuts that are not owned by Browser chrome.
- Preserved Browser-owned location/search focus, new/close tab, tab cycling, refresh, F5/F6, and the existing reserved Alt/Super/Meta boundary ahead of page forwarding.
- Suppressed printable CEF character events while Control is held so a forwarded shortcut does not also insert its printable key into editable page content.
- Extended the forced-windowless direct-keyboard runtime lane so the page must observe an unowned Ctrl+E keydown before ordinary `wayland` typing can trigger the fixed success navigation; accidental `e` insertion or shortcut capture fails the test.

### Acceptance boundary

This establishes Development source and automated Xvfb evidence for one non-conflicting Control-modified page shortcut while preserving current Browser-owned shortcuts. It does not establish representative native-Wayland shortcut behavior, clipboard copy/paste semantics, Alt/Super/Meta page forwarding, international/dead-key acceptance, accessibility, sustained performance, production approval, Stable, Seal, or Anchor qualification.

## 2026-10-03 — Linux Wayland GTK IME composition bridge

### Added

- Added an engine-independent text-input contract for software/windowless Browser surfaces with explicit preedit composition, committed text, and cancellation.
- Added GTK `GtkIMMulticontext` integration for software-rendered surfaces, including focus lifecycle, UTF-8 to UTF-16 preedit conversion, GTK character-offset to UTF-16 selection translation, commit handling, and reset/cancel cleanup.
- Added CEF OSR translation through `ImeSetComposition`, `ImeCommitText`, and `ImeCancelComposition`.
- Added a forced-windowless Xvfb runtime smoke that leaves GTK IME enabled and requires both GTK and CEF commit diagnostics plus DOM-driven navigation to the fixed success marker.

### Changed

- Preserved Browser-owned Ctrl/Alt/Super/Meta and F5/F6 shortcut ownership ahead of page/IME forwarding.
- Preserved the native X11/XWayland child-window input path unchanged and kept the direct-keyboard regression independent by disabling GTK IME in that specific lane.

### Acceptance boundary

The GTK composition/commit plumbing is integrated Development source and passed exact-head pre-merge Linux, security, and extension validation. It does not establish representative native-Wayland IME behavior, IBus/Fcitx/XIM interoperability, language-specific candidate-window placement, surrounding-text/delete-surrounding support, dead-key/non-BMP/international acceptance across locales, accessibility, sustained performance, production approval, Stable, Seal, or Anchor qualification.

## 2026-10-03 — Linux Wayland bounded custom cursor support

### Added

- Extended the engine-independent software-surface cursor contract with Browser-owned custom cursor bitmap data, dimensions, hotspot, and reported scale metadata.
- Bound CEF custom cursor dimensions to 512 by 512, copied transient CEF BGRA bytes before returning from the engine callback, clamped hotspots to the accepted bitmap, and rejected missing, invalid, or oversized buffers instead of retaining borrowed engine memory.
- Added GTK custom cursor presentation by converting the copied BGRA bitmap to RGBA and constructing a native cursor at the validated hotspot.
- Extended the forced-windowless CEF runtime lane with a real CSS custom-cursor region followed by the standard hand-cursor, pointer/wheel/navigation, and Browser-owned context-menu checks.

### Changed

- Restacked only the still-valid custom-cursor behavior from stale PR #124 onto the IME-integrated current-main lineage instead of replaying its stale ancestry.
- Preserved native X11/XWayland child-window cursor handling unchanged. The reported CEF custom-cursor scale factor is bounded and retained for diagnostics; representative high-DPI rendering acceptance remains open.

### Acceptance boundary

This is Development source and test coverage, not representative native-Wayland or high-DPI cursor acceptance. Broader popup behavior, drag-and-drop, clipboard, accessibility, sustained interaction performance, accelerated rendering, production approval, Stable, Seal, and Anchor qualification remain open.

## 2026-10-02 — Android logical tab-session model foundation

### Added

- Added a Browser-owned logical tab/session state model independent from transient Android WebView identity.
- Added stable bounded tab identities, one exact active tab, a 32-tab ceiling, duplicate/unknown-ID rejection, last-tab protection, deterministic active-tab fallback after close, and exact-tab URL/title updates.
- Added fail-closed rejection for blank or oversized location mutations instead of allowing invalid location state to reach the logical-tab constructor path.
- Added JVM coverage for active/inactive close behavior, tab ceilings, exact-tab mutation, invalid location rejection, and bounded/normalized titles.

### Architecture boundary

This is a model foundation only. It does not add a visible Android tab switcher, multiple WebViews, per-tab engine history, process-death persistence/recovery, profiles/Webspaces, Private or Isolated Private semantics, Sync projection, or new network/provider authority.

### Acceptance boundary

The runtime object-model roadmap remains open. Android chrome/engine binding, lifecycle restoration, accessibility/adaptive-device behavior, privacy-context isolation, representative-device evidence, and release qualification remain separate gates.

## 2026-10-03 — Linux Wayland direct keyboard-input candidate

### Added

- Added an engine-independent native key-event contract for software/windowless Browser surfaces and CEF OSR forwarding for raw key-down, key-up, and printable character events.
- Added GTK mapping for common letters, digits, shifted number-row symbols, punctuation, editing/navigation keys, F1-F24, and keypad keys while carrying hardware keycode plus Shift/Caps state.
- Extended the forced-windowless CEF validation lane with an actual editable page: CI focuses the Browser web surface, types `wayland` through X events, and requires the page to navigate to a fixed success marker only after the DOM input receives the exact text.

### Changed

- Preserved existing Browser-owned Ctrl/Alt/Super/Meta shortcuts by deferring those combinations instead of automatically forwarding them to page content in this tranche.
- Preserved native X11/XWayland child-window keyboard handling unchanged; the new key bridge is exercised only by software/windowless surfaces.

### Acceptance boundary

This is a Development candidate until fresh exact-head workflows pass and guarded integration/readback completes. IME composition/preedit/commit, dead-key/non-BMP/international text acceptance, non-conflicting page keyboard shortcuts, clipboard shortcuts, representative native-Wayland typing, accessibility, sustained interaction performance, production approval, Stable, Seal, and Anchor qualification remain open.

## 2026-10-02 — Linux Wayland standard cursor propagation

### Added

- Added an engine-independent standard cursor bridge for software/windowless Browser surfaces.
- Mapped CEF pointer, text, hand, wait/help, resize, move/panning, vertical-text, cell, context-menu, alias, progress, drag-state, zoom, grab/grabbing, hidden and not-allowed cursor types into GTK cursor presentation without leaking CEF types into the native host contract.
- Reset Browser-owned software-surface cursor state when the engine view is detached or Browser-owned internal/panel surfaces replace web content.
- Extended the forced-windowless CEF runtime lane so a full-page link must produce both a CEF hand-cursor event and a GTK-applied hand cursor before the existing click/wheel/context-menu checks continue.

### Changed

- Preserved native X11/XWayland child-window cursor handling unchanged; the Browser-owned cursor sink is attached only to the software/windowless path.
- Left CEF custom bitmap cursors explicitly outside this tranche rather than silently converting them into an inaccurate standard cursor.

### Acceptance boundary

This is Development interaction evidence only. Representative native-Wayland cursor behavior, custom bitmap cursors, keyboard/IME, broader popup behavior, drag-and-drop, clipboard, accessibility, sustained interaction performance, accelerated rendering, production approval, Stable, and Anchor qualification remain open.

## 2026-10-02 — Linux Wayland windowless context-menu host bridge

### Added

- Added an engine-independent native context-menu request/selection bridge for software/windowless engine surfaces.
- Added a Glaze-styled GTK popover presenter that preserves command, check, radio, separator, and submenu structure from the CEF menu model and returns the selected command ID or explicit cancellation.
- Added active-menu replacement/window-destroy cleanup so outstanding windowless context-menu callbacks fail closed instead of being left unresolved.
- Extended the forced-windowless Core CI lane to right-click a real rendered page, require Browser-owned menu presentation and Escape cancellation, and reject regression to CEF's native-window OSR context-menu error.

### Changed

- Preserved the existing X11/XWayland native child-window context-menu path; the Browser-owned GTK presenter is attached only to the windowless/software surface.
- Restacked only the still-valid context-menu behavior from stale PR #113 onto current authoritative Browser main rather than replaying its stale ancestry.

### Acceptance boundary

This is Development interaction evidence only. Representative native-Wayland context-menu selection behavior, keyboard/IME, cursor propagation, broader popup behavior, drag-and-drop, clipboard, accessibility, sustained interaction performance, accelerated rendering, production approval, Stable, and Anchor qualification remain open.

## 2026-10-02 — Android first-use setup and contextual guidance

### Added

- Added a required three-step first-use setup for fresh Android Browser profiles.
- Added device-local setup progression, completion, optional-tip enablement, dismissed-tip state, setup replay, and dismissed-tip reset.
- Added a compact post-setup Browser tip plus one grouped **Guidance & tips** menu surface rather than multiple top-level onboarding controls.
- Added unit coverage for setup-step bounds plus Android 15 instrumentation coverage for preference persistence, full fresh-setup completion, and Activity recreation resuming the durably accepted setup step.

### Changed

- Setup progress and guidance mutations use synchronous local preference commits; Browser does not visually advance a setup transition when the corresponding local state write fails.
- Onboarding text now reflects the integrated Page controls, Clear browsing data, current privacy/security defaults, and fail-closed free-text Search boundary rather than replaying the stale pre-Page-controls UI from historical PR #108.
- Advanced the Android beta artifact identity to `0.1.0-beta.1+android.12` / versionCode `10012` and updated the exact APK identity guard.

### Privacy and authority boundary

First-use and tip state is stored only in device-local Browser preferences. This feature adds no account, telemetry, Search transmission, website permission, provider authorization, Sync dataset, Privacy Shield decision, or Wardveil authority.

### Acceptance boundary

Representative-device fresh-install/upgrade behavior, interruption outside Activity recreation, TalkBack/Switch Access, large-text/reflow, localization/RTL, adaptive form factors, Browser-specific Glaze acceptance, protected Development signing/update continuity, production signing/distribution, Release Candidate, Stable, and Anchor qualification remain open.

## 2026-10-02 — Android Page controls, JavaScript, and image-loading controls

### Added

- Added a compact Browser-owned **Page controls** Glaze sheet that groups Text size, Desktop site, JavaScript, and automatic image loading.
- Added session-local **JavaScript** On/Off control for Android WebView pages.
- Added session-local **Images** On/Off control using WebView automatic/network image-loading settings.
- Added focused unit coverage for Page controls defaults and managed-emulator assertions that JavaScript and automatic image loading remain enabled by default.

### Changed

- Moved Text size and Desktop site out of the top-level Browser menu and into Page controls to reduce menu density while preserving direct Find in page and Clear browsing data access.
- JavaScript and Images choices survive Android Activity recreation but are not written to durable Browser preferences or represented as synchronized profile state.
- Ordinary HTTP(S) pages reload when a Page control changes so the new content setting is applied consistently.
- Restacked this tranche after the integrated internationalized-host hardening and advanced the Android beta artifact identity to `0.1.0-beta.1+android.11` / versionCode `10011` with the matching CI package/version verification contract.

### Privacy and compatibility boundary

JavaScript and Images remain enabled by default for web compatibility. Turning either off is an explicit session-local user choice and may reduce website functionality or content. These controls do not manufacture Privacy Shield, security, profile-policy, or Browser Sync authority.

### Acceptance boundary

These are Development page-content controls. Representative-device usability, disabled-content compatibility across real websites, TalkBack/Switch Access, localization/RTL, durable per-site policy, profile-scoped settings, production approval, Stable, and Anchor qualification remain open.

## 2026-10-02 — Current-main internationalized host canonicalization

### Added

- Added a Browser-owned internationalized-host policy that converts Unicode HTTP(S) DNS names to lowercase ASCII A-label identity before Android direct navigation and Browser-owned unfocused address presentation.
- Added regression coverage for direct and scheme-less Unicode domains, trailing DNS root dots, explicit ports, path/query/fragment preservation, canonical numeric IPv4, bracketed IPv6, malformed STD3 labels, credential rejection, and credential-safe address presentation.

### Changed

- Reconciled the still-valid intent of historical PR #80 onto current authoritative Browser architecture without replaying its stale resolver implementation or discarding newer Search/control-character/port/numeric-host/disclosure hardening.
- Bracketed non-IPv6 authorities and invalid port forms fail closed before WebView navigation; valid IPv6-shaped bracket contents are syntactically validated without allowing ordinary bracketed hostnames to trigger DNS resolution.
- Advanced the Android beta artifact identity to `0.1.0-beta.1+android.10` / versionCode `10010` and updated the CI identity guard.

### Trust boundary

A-label canonicalization establishes deterministic Browser-owned host identity only. It does not establish UTS #39/confusable safety, DNS resolution trust, certificate validity, reputation, registrable-domain identity, Wardveil verdicts, or origin acceptance.

### Acceptance boundary

This is Development source hardening. Broader Unicode confusable/spoofing analysis, certificate-detail/origin transitions, full bidi/RTL acceptance, representative-device validation, production approval, Stable, and Anchor qualification remain open.

## 2026-10-02 — Android Desktop site and Clear browsing data controls

### Added

- Added a Browser-owned **Desktop site** control that changes the current Android Browser session between mobile and desktop-style presentation, derives its desktop user agent from the active WebView engine identity, and enables wide-viewport/overview behavior without pinning a separate Chromium version.
- Added a confirmed **Clear browsing data** flow that explicitly lists data to remove versus settings to preserve before clearing Android WebView cookies/sign-in state, website storage, cache, form data, navigation history, and SSL preferences.
- Added unit coverage for desktop user-agent derivation and browsing-data scope plus managed-emulator coverage that verifies WebView cookie removal.

### Changed

- Desktop site state is preserved across Android Activity recreation but remains session-local rather than becoming a durable or synchronized preference.
- Clear browsing data returns Browser to the local Start page and clears prior WebView navigation history after that local page finishes loading.
- Advanced the Android beta artifact identity to `0.1.0-beta.1+android.9` / versionCode `10009` and advanced the CI package/version verification contract with it.

### Privacy boundary

Clear browsing data does not erase Browser preferences or Android app permissions. It is an app-wide Android WebView data reset in the current Development shell, not profile-scoped Private/Isolated Private or Close & Forget acceptance.

### Acceptance boundary

These are Development local browsing controls. Representative-device usability, destructive-action accessibility, TalkBack/Switch Access, localization/RTL, profile-scoped deletion, private-context isolation, production approval, Stable, and Anchor qualification remain open.

## 2026-10-02 — Android Find in page and page text-size controls

### Added

- Added a Browser-owned **Find in page** Glaze bottom sheet backed by the active Android WebView's local find facility, with bounded 512-character input, live match status, Previous/Next navigation, and highlight cleanup on close.
- Added Browser-owned **Page text size** controls from 75% through 200% in 25-point steps with Reset to 100%.
- Added focused unit coverage for find-status presentation and bounded text-zoom behavior, plus managed-emulator verification of the default WebView text-zoom state.

### Changed

- Page text zoom now persists as an application-local Browser preference across pages and launches without changing Android system font scale.
- Advanced the Android beta artifact identity to `0.1.0-beta.1+android.8` / versionCode `10008`.
- Updated the feature inventory and user manual so Find in page is explicitly local-only and page text zoom is not represented as synchronized Browser state.

### Privacy boundary

Find in page does not delegate text to GoreeCloud Search or another provider. Page text zoom stores only the selected percentage as local application preference state.

### Acceptance boundary

These are Development browser-utility capabilities. They do not establish representative-device accessibility, large-text interaction quality, localization/RTL behavior, form-factor acceptance, Browser Sync inclusion, production approval, Stable, or Anchor qualification.

## 2026-10-02 — Android origin disclosure, site information, and privacy-status controls

### Added

- Added a Browser-owned **Site information** bottom sheet that presents the current website origin and HTTPS/HTTP transport classification without turning HTTPS into a certificate, Wardveil, or trust verdict.
- Added a scrollable Browser-owned **Privacy & security** bottom sheet that truthfully exposes enforced Android/WebView defaults and explicit fail-closed Search, permission, download, and provider boundaries.
- Added focused unit coverage for page-address disclosure, site-information presentation, protection-state rendering, numeric-host handling, and port-range validation.

### Changed

- Page-address Copy/Share actions now appear only when the current surface has a canonical Browser-approved HTTP(S) URL, preventing Browser-owned `goreecloud://`, local-resource, data, blob, file, or content addresses from being disclosed through Android clipboard/share flows.
- Android navigation now rejects zero/out-of-range ports and ambiguous or invalid numeric IPv4 host forms in addition to the existing credential, unsupported-scheme, and control-character fail-closed checks.
- Reconciled Browser capability records to the merged Linux Wayland pointer/wheel/focus forwarding in PR #111 and updated the user manual to the current Android beta identity and actual `BrowserActivityV2` behavior.
- Updated the documented canonical Glaze repository identity from the retired `GoreeCloud/glaze-ui` name to `GoreeCloud/glaze`.

### Acceptance boundary

These changes are Development hardening and truthful Browser-owned status UX. The Site information surface does not replace engine certificate details, the Privacy & security surface does not manufacture Privacy Shield/Wardveil/Policy/Search authority, and this change does not establish representative-device accessibility/usability, production signing, Release Candidate, Stable, or Anchor acceptance.

## 2026-09-29 — Linux Wayland page rendering integrated; pointer interaction candidate

### Changed

- Integrated the CEF windowless/software-rendering path after exact-head CI verified HTTPS completion and materially non-uniform page frames, and representative Zorin OS 17.3 Wayland evidence showed the real `https://example.com/` page inside the Browser content surface.
- Preserved the native X11/XWayland child-window rendering path.
- Advanced a separate Development candidate that forwards GTK pointer motion, mouse buttons, wheel input, and focus into the windowless CEF browser without duplicating native-child input.
- Stage the canonical Browser SVG with GTK development binaries so the desktop chrome can render product artwork when launched from the build directory.
- Correct stale Browser fallback copy that still described native Wayland page rendering as unavailable.

### Acceptance boundary

Integrated Wayland page pixels are Development evidence, not production acceptance. The pointer/branding continuation remains candidate work until exact-head automated and representative-device interaction evidence exists. Keyboard/IME, cursor propagation, popup/context-menu behavior, drag-and-drop, clipboard, accessibility, accelerated rendering, sustained performance, Browser-wide Glaze acceptance, production approval, Stable, and Anchor remain open.

## 2026-09-28 — Linux Wayland CEF software-rendering candidate

### Changed

- Added a Browser-owned software-frame surface contract for engine renderers that cannot attach a native child window.
- Added CEF windowless/off-screen rendering support with `CefRenderHandler` view metrics and BGRA frame delivery while preserving the existing X11/XWayland child-window path.
- Added GTK drawing-area presentation of the software CEF frame and automatic fallback selection on non-X11 GTK backends.
- Added a Development-only `GOREECLOUD_BROWSER_FORCE_WINDOWLESS=1` validation switch and a Core CI runtime gate that requires both HTTPS main-frame completion and an observed software-frame presentation.

### Acceptance boundary

This source change is a Development render candidate until the changed exact head passes applicable CI and the owner-device Wayland test shows real page pixels. It does **not** yet establish page input/IME/popup/drag-and-drop/accessibility correctness, accelerated rendering, sustained performance, Browser-wide visual acceptance, production approval, Stable, or Anchor maturity.

## 2026-09-28 — Linux Wayland-session startup resilience

### Changed

- Allow the GTK3 GoreeCloud Browser shell to start on an available non-X11 GTK display backend instead of terminating the entire application at the host boundary.
- Preserve the current CEF X11/XWayland child-surface contract and present an explicit renderer-compatibility surface when that native page surface is unavailable.
- Resolve persistent CEF profile cache paths against the Browser cache root so the default relative `profile` path no longer falls back to in-memory storage solely because it is relative.
- Correct the stale Android Glaze documentation baseline from V1.5/1.5.1 to the current V1.6/1.6.0 Stable source baseline.

### Acceptance boundary

This is a Development resilience and documentation correction. It does **not** implement native Wayland CEF page rendering, establish visible web-page pixels on Wayland, close representative-device visual acceptance, or promote Browser to Release Candidate, production-approved, Stable, or Anchor maturity.

## 2026-09-26 — Immutable Linux CEF archive identity hardening

### Changed

- Preserved the current CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` runtime candidate while pinning the reviewed Linux minimal archive to both SHA-1 and SHA-256 identities.
- Require the upstream checksum sidecar, downloaded archive, cached archive, and extracted provenance record to match the repository-owned digest pair rather than accepting future bytes published under the same archive name.
- Made the CMake CEF version pin source-owned rather than a user-overridable cache value.
- Extended the offline CEF bootstrap contract gate to assert the immutable archive identities and recorded the exact dependency constraint in the Platform Contract manifest.

### Acceptance boundary

This is Development supply-chain hardening only. It does not change the already verified renderer-start or HTTPS-navigation evidence and does not establish visible page pixels, representative-device interaction, long-session stability, Browser-wide GLAZE UI acceptance, Release Candidate, production approval, Stable, or Anchor maturity.

## 2026-09-26 — Linux CEF HTTPS navigation smoke accepted

### Changed

- Added bounded CEF main-frame load start, load-end, and load-error diagnostics behind the existing runtime-diagnostics boundary.
- Advanced the Ubuntu 22.04 sandboxed CEF smoke from renderer-process detection to successful engine-level HTTPS navigation.
- Exact-head Core CI now requires both a Chromium renderer subprocess and a main-frame `OnLoadEnd` with HTTP 200 for `https://example.com/`.
- The smoke fails closed on a CEF main-frame load error.

### Acceptance boundary

This proves engine-level HTTPS navigation completion in the pinned sandboxed CEF runtime under Ubuntu 22.04 CI. It does **not** yet prove visible page pixels inside the GoreeCloud GTK child surface, representative-owner-device interaction, navigation-control behavior against visible content, TLS/certificate UX, renderer crash recovery, private-context isolation, accessibility, performance, packaging acceptance, Release Candidate, production approval, Stable, or Anchor maturity.

## 2026-09-26 — Linux CEF sandboxed runtime smoke accepted

### Changed

- Restacked the Linux CEF runtime milestone onto current `main` and revalidated it at exact head.
- Verified the repository-pinned CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` candidate through the Ubuntu 22.04 compile gate.
- Verified the CEF runtime payload, sandbox-preserving Xvfb + D-Bus startup, Browser-process initialization, subprocess role handling, and creation of a Chromium renderer subprocess in Core CI.
- Preserved the Linux Chromium sandbox rather than bypassing it to obtain a green runtime result.
- Kept the initial Browser URL outside inherited CEF subprocess positional arguments.
- Integrated the current CEF client/media-probe/runtime compatibility fixes required by real compilation and launch validation.

### Acceptance boundary

This is Development runtime-start evidence. It proves that the exact Browser candidate can compile and launch the pinned CEF runtime and reach a Chromium renderer subprocess under Ubuntu 22.04 CI with sandboxing preserved. It does **not** yet establish visible page pixels, representative-owner-device HTTPS rendering/navigation, TLS/certificate UX, renderer crash recovery, private-context isolation/cleanup, accessibility, performance, packaging acceptance, Release Candidate, production approval, Stable, or Anchor maturity.

## 2026-09-25 — Android browser-chrome bidirectional-text hardening

### Changed

- Added a shared Android Browser chrome presentation boundary that strips Unicode bidirectional formatting controls from untrusted page-title and unfocused-address text.
- Applied the boundary to Android page-title presentation and condensed omnibox presentation without changing the full URL used for navigation authority.
- Added focused JVM regression coverage for bidi override and isolate characters in Browser-owned title/address surfaces.
- Preserved ordinary Browser navigation, Search, permissions, networking, and engine authority boundaries.

### Acceptance boundary

This is Development presentation hardening only. It does not establish complete IDN/confusable/origin/certificate/bidirectional-text acceptance, representative-device usability or accessibility, Browser-wide GLAZE UI acceptance, production signing/distribution, Release Candidate, production approval, Stable, or Anchor product maturity.

## 2026-09-25 — Linux CEF render-candidate compile validation

### Changed

- Added an exact-head Ubuntu 22.04 Core CI job that bootstraps and compiles the pinned CEF 152 / Chromium 152 Linux render candidate with GTK/X11 dependencies.
- Corrected the CMake C-language probe requirement, stale Linux smoke source path, CEF media-probe bridge/API usage, render-app construction, and GoreeCloud exception/RTTI build requirements exposed by real compilation.
- Verified the resulting exact candidate through the repository Core CI render-build gate before integration.

### Acceptance boundary

Successful CI compilation establishes Development buildability of the pinned Linux render candidate. It does not establish representative-device HTTPS rendering, TLS/certificate presentation, renderer crash handling, private-context isolation/cleanup, accessibility, performance, packaging, Release Candidate, production approval, Stable, or Anchor product maturity.

## 2026-09-25 — Android Search-query boundary hardening

### Changed

- Added one shared Android Search-boundary text-safety policy used before omnibox classification and again before remote Search request construction.
- Reject C0/C1 control characters and the narrow Unicode Bidi_Control formatting set before trimming or delegation, while preserving ordinary Unicode shaping such as ZWJ emoji.
- Bounded normalized free-text Search input to 2,048 characters before transport construction.
- Reject Search capability evidence that advertises more than the Browser-supported 100-result maximum.
- Added focused JVM regression coverage for leading/trailing/interior controls, bidirectional formatting controls, exact-limit and over-limit queries, ordinary Unicode shaping, and incompatible capability limits.
- Preserved direct structurally valid HTTP(S) navigation and all existing Privacy Shield, Identity, production-capability, POST-body, no-telemetry, and fail-closed remote-Search boundaries.

### Acceptance boundary

This is Development input/contract hardening only. It does not enable remote Search, establish live Search/Privacy Shield/Identity authority, prove representative physical-device usability, complete Unicode confusable/origin trust acceptance, satisfy Browser-wide GLAZE UI acceptance, or establish Release Candidate, production, Stable, or Anchor product maturity.

## 2026-09-25 — Pinned Linux CEF render bootstrap

### Changed

- Pinned the current Linux render milestone to CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83` using the official CEF Linux x86_64 minimal binary distribution.
- Added a Browser-owned bootstrap that fetches the official CEF checksum first, verifies the archive, rejects unsafe extraction members, records local SHA-256 provenance, and reuses only a matching accepted cache.
- Added CMake validation that rejects a CEF root whose `cef_version.h` does not report the exact repository pin.
- Added one-command Linux render-candidate build and sandbox-preserving launch scripts.
- Propagated the host Browser process `argc/argv` into CEF browser-process initialization and corrected the default subprocess executable name.
- Corrected the X11 parent-window conversion to the integral CEF Linux window-handle type.
- Added an offline CI contract gate that syntax-checks the bootstrap/launch scripts and verifies the bootstrap and CMake pins cannot silently diverge.

### Acceptance boundary

This establishes reproducible Development dependency/bootstrap evidence only. It does not establish successful CEF compilation on a representative machine, HTTPS rendering, private-context runtime acceptance, render-capable Beta status, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Linux panel-dismissal and visual-density follow-up

### Changed

- Added explicit close affordances to Browser-owned Linux Development panels and restored the prior internal/web content surface when a panel is dismissed.
- Added Escape panel dismissal plus Ctrl+K and F6 unified-location focus alongside the existing keyboard-first Browser controls.
- Normalized GTK symbolic icon sizing to 20 px and changed the Downloads toolbar glyph to a more reliable native save/download symbol.
- Pulled first-party cards and Browser panels upward from the vertical dead zone while preserving Glaze spacing and responsive minimums.
- Reduced the visual prominence of the Development lifecycle badge without removing lifecycle truth.
- Corrected literal escaped-newline text in the Linux build guide and documented the current keyboard control set.

### Acceptance boundary

This follow-up remains Development presentation behavior. It does not establish renderer integration, complete accessibility acceptance, production-grade panel behavior, Release Candidate, production approval, or Stable/Anchor maturity.

## 2026-09-25 — Linux polish, semantic panels, and keyboard navigation

### Changed

- Replaced the generic Linux `Browser tool` placeholder with semantic Glaze panels for Bookmarks, Reader Mode, Privacy Shield, Wardveil Security, Clipboard, DNS, Proxy, Search-unavailable, and live Development Downloads presentation.
- Added keyboard-first desktop navigation for location focus, new/close tab, forward/back tab cycling, reload, Back/Forward, and Home without bypassing Browser-owned command routing.
- Added explicit next/previous tab lifecycle operations to `WindowController` and smoke coverage for cyclic tab activation.
- Added a structured Settings section grid while keeping non-implemented controls visibly non-authoritative.
- Simplified redundant first-party Development badges so the start surfaces retain one bounded renderer/status indicator while the top chrome continues to expose Development lifecycle.
- Added pure Browser panel-presentation contracts and smoke coverage so provider-owned privacy/security/network truth remains fail-closed.
- Reconciled stale implementation-status references from superseded Tabmark artwork to the canonical Browser compass identity.

### Acceptance boundary

This tranche remains Development source work. It does not establish renderer integration, live Bookmarks/Privacy Shield/Wardveil/DNS/Proxy authority, complete Settings controls, representative-device accessibility/performance acceptance, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Android 15 managed-emulator Browser smoke

### Changed

- Added AndroidX instrumentation support and an exact-head Android 15 managed-emulator lane.
- Added runtime smoke coverage for Browser-owned chrome controls, initial Back/Forward disabled state, package identity, conservative WebView file/content/mixed-content/cookie defaults, and fail-closed free-text Search.
- Updated the APK build lane to assemble the instrumentation APK before the dependent emulator job.
- Reconciled Android Beta documentation from stale V1.5.1 wording to the integrated GLAZE UI V1.6 / 1.6.0 source and qualification mapping.
- Preserved the branding-provenance validation and `assets/branding/**` Android workflow trigger integrated on current main.

### Acceptance boundary

Managed-emulator evidence is Development evidence only. It does not establish representative physical-device launcher rendering, accessibility, practical network browsing, sustained performance, OEM behavior, production signing, release qualification, Production Acceptance, or Anchor product maturity.

## 2026-09-25 — Android launcher branding provenance guard

### Changed

- Added fail-closed source checks that pin the Browser-local full-color and monochrome branding assets to the current canonical `GoreeCloud/branding-assets` Git blobs.
- Pinned the reviewed Android launcher background, foreground, and monochrome packaging derivatives so silent redraws or substitutions fail CI.
- Tightened adaptive-icon validation to require the expected Browser launcher drawable references.
- Updated Android Beta workflow path filters so branding-only changes under `assets/branding/**` trigger launcher identity validation and APK verification.
- Preserved the newer canonical Browser artwork and branding documentation already integrated by the Linux visual-acceptance tranche rather than reapplying superseded PR #75 artwork.

### Acceptance boundary

These checks establish source provenance and CI coverage only. They do not establish physical-launcher rendering quality, representative-device acceptance, accessibility, production signing, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Linux real-device visual acceptance follow-up

### Changed

- Corrected first-party internal-page presentation so `goreecloud://` implementation URLs no longer leak into the visible tab title or omnibox presentation for Browser-owned internal surfaces.
- Replaced ambiguous primary Unicode toolbar glyphs with platform-native symbolic icons with text fallbacks.
- Added an interactive Linux tab strip with visible tab titles and Development create/activate/close behavior.
- Added a functional first-party start-surface search field plus Bookmarks, Downloads, and Settings quick actions.
- Synchronized the Linux Browser full-color and monochrome consumer artwork to canonical `GoreeCloud/branding-assets` compass sources.
- Reconciled stale Browser-local branding documentation that still described the superseded aqua Tabmark or artwork-pending state.

### Authority and acceptance boundary

Canonical Browser branding is verified at `GoreeCloud/branding-assets` main revision `a831479976fd3f82dc31cf9b7785145a757e7e28`. Exact-head Browser Core CI and security workflows passed for the integrated Linux follow-up; fresh owner-device rendered review remains required. Android branding validation did not run for branding-only changes under the prior workflow path filter, which is corrected by the launcher-provenance follow-up below. This does not establish Browser-wide Glaze acceptance, renderer acceptance, Release Candidate, production approval, or Stable/Anchor product maturity.

## 2026-09-25 — Glaze V1.6 contract and responsive follow-up

### Changed

- Reconciled stale V1.5.1 qualification assertions and evidence anchors that remained after the Linux V1.6 source migration.
- Added exact V1.6 smoke gates for accepted release source, qualification source, qualification evidence integration, rollback baseline, and Browser presentation invariants.
- Migrated the Android-native Glaze contract and tests from V1.5.1 to V1.6 / 1.6.0 and added explicit disabled-control presentation.
- Hardened the GTK first-party cards for narrower windows by removing 560–600 px minimum-size assumptions, constraining text measure, and allowing status chips to wrap.
- Reconciled the Browser adoption record with canonical GLAZE UI V1.6 Anchor lifecycle terminology while retaining Stable as compatibility release-channel vocabulary.

### Lifecycle boundary

This follow-up remains Development source evidence. Browser-wide rendered/native, accessibility, large-text, localization/RTL, representative-device/form-factor, performance, rollback, renderer-integrated, release, and production acceptance remain open.

## 2026-09-25 — Linux desktop Glaze V1.6 source migration tranche

### Changed

- Mapped Browser source guards from historical GLAZE UI V1.5.1 to exact current Stable GLAZE UI V1.6 / 1.6.0 release source `a7180679ea851389e0f3004515f9a25f420e716d`.
- Reworked the GTK/X11 Development chrome around a compact active-tab surface, primary navigation capsule, bounded secondary tools popover, branded first-party surfaces, and explicit Development state.
- Added friendly first-party tab titles for New Tab, Home, Settings, and Private Browsing.
- Reordered initial GTK presentation so the visible first-party surface is selected after the window is shown.
- Lowered the project CMake floor to 3.22 and moved the dedicated GTK beta-shell CI lane to Ubuntu 22.04 for Zorin/Ubuntu 22.04-class compatibility.
- Corrected stale Browser Glaze documentation that referenced an obsolete Glaze 2.0 baseline.

### Lifecycle boundary

This tranche is Development source implementation only. It does not establish rendered/accessibility/device/performance acceptance, render-capable CEF/Chromium desktop acceptance, Release Candidate, production approval, or Stable status.

## 2026-09-22 — Repository feature/changelog governance migration

### Added

- `IMPLEMENTED-FEATURES.md` as the authoritative implemented-feature inventory.
- `PLANNED-FEATURES.md` as the authoritative planned/incomplete-feature inventory.
- `CHANGELOGS.md` as the authoritative repository change-history record.

### Changed

- Retired the repository `FEATURE-ROADMAP.md` control model.
- Removed the obsolete requirement to synchronize feature-roadmap authority with Google Drive.
- Preserved `FEATURES.md`, `SPECIFICATIONS.md`, supporting contracts, Git history, pull requests, workflow evidence, and `NOTES.md` as product/specification/evidence sources rather than duplicate feature-lifecycle authority.
- Recorded the current V1.5.1 Browser source mapping as migration-required against current Official Stable GLAZE UI V1.6 / 1.6.0 rather than silently rebinding earlier evidence.

### Lifecycle boundary

This migration changes documentation/control-plane authority only. Browser remains Development/non-Stable. Representative-device usability/accessibility/performance, current V1.6 application acceptance, platform integrations, recovery, production signing/deployment, Release Candidate, and Stable qualification remain open.

## Historical change evidence

Historical implementation and validation evidence remains preserved by Git history, merged pull requests, repository documentation, workflow records, exact-source evidence, and product-specific governed evidence. Future material integrated changes must be recorded here.

## Maintenance rule

Record material integrated changes here with enough exact repository evidence to distinguish authoritative `main` state from draft/unmerged work. Do not convert source presence, an installable beta artifact, or green CI into production or Stable claims.
# GoreeCloud Browser — Beta 0.1 Acceptance Status

**Version:** 0.1.0-beta.1  
**Channel:** Beta  
**Production approved:** No

This record separates completed source/native-shell, CI renderer-start, and engine-level HTTPS navigation evidence from representative-device visible rendering and production gates that remain open.

## Accepted Development evidence for Beta 0.1

- Browser version/channel metadata identifies `0.1.0-beta.1` and explicitly denies production approval.
- GoreeCloud Browser remains the sole normal user-facing product identity.
- Official Stable GLAZE UI V1.6 / `1.6.0` is the enforced Browser source baseline at exact reviewed source revision `a7180679ea851389e0f3004515f9a25f420e716d`.
- Engine-independent Browser core builds and CTest smoke tests are gated in GitHub Actions on Linux Release and Debug configurations.
- The GTK3/X11 GoreeCloud/Glaze native shell has a dedicated GitHub Actions compile/test gate.
- The executable identity is normalized to `goreecloud-browser`.
- Linux desktop and AppStream metadata are present.
- The canonical GoreeCloud Browser compass identity is synchronized from `GoreeCloud/branding-assets` under the `io.goreecloud.Browser` application identity.
- New Tab, Home, Settings, and Private Start use GoreeCloud internal routes.
- Linux Development chrome includes real tab create/activate/close controls, keyboard-first navigation, semantic Browser panels, and the unified search/navigation surface.
- GoreeCloud Search remains the sole search authority and fails closed when its endpoint is unavailable or unconfigured.
- Direct URL navigation is independent from Search availability.
- Normal and private Browser startup paths are represented, including `--private` and `--isolated-private`.
- Browser Engine Layer, Chromium adapter, CEF runtime delegate, request-context separation, and native-surface contracts are present in source.
- The Linux render candidate is pinned to CEF `152.0.6+g708dc14+chromium-152.0.7977.83` / Chromium `152.0.7977.83`, with official-checksum verification and exact-version CMake enforcement.
- Exact-head Ubuntu 22.04 Core CI compiles the pinned CEF/Chromium + GTK/X11 candidate, verifies the runtime payload, launches it under Xvfb + D-Bus with the Chromium sandbox preserved, verifies that a Chromium renderer subprocess starts, and requires the main frame to complete `https://example.com/` with HTTP 200.
- The runtime smoke keeps the initial Browser URL out of inherited CEF subprocess positional arguments and exercises the current browser/renderer process-role integration.
- The Flatpak manifest remains a Development packaging scaffold and does not yet constitute accepted CEF runtime packaging.

## Not yet accepted as a render-capable desktop beta

The following still require direct representative runtime evidence:

- Visible CEF/Chromium web-content pixels inside the GoreeCloud GTK shell on representative desktop hardware.
- Visible HTTPS page pixels and ordinary web interaction inside the GoreeCloud GTK child surface on the owner-device test path. Engine-level `https://example.com/` load completion is already CI-verified.
- Real Back, Forward, Refresh/Stop, loading/title/address updates, and renderer navigation callbacks against actual pages.
- Multi-tab engine-view lifetime and Browser tab behavior against the real renderer, including ordering/reordering and lifecycle stress.
- TLS/certificate/security-state presentation and fail-closed error behavior.
- Private request-context cookie/storage isolation verified against actual websites.
- Per-origin private-data cleanup and permission cleanup. These remain fail-closed where not yet implemented.
- Packaged Chromium sandbox and site-isolation acceptance beyond the current CI startup smoke.
- Renderer crash/unresponsive recovery behavior.
- X11/XWayland representative-device acceptance and later native Wayland support.
- Browser accessibility, performance, power, sustained-session, and Human Visual Excellence acceptance with the real renderer.
- Flatpak/installer runtime packaging, signing, update/rollback, and artifact-provenance acceptance.

## Not accepted for Stable

Stable remains blocked by the full production validation matrix: required feature completion, Privacy Shield/Wardveil evidence, GoreeCloud service adapters, accessibility, security testing, compatibility, packaging, signing, update/rollback, recovery, artifact provenance, real-device Android acceptance, Windows acceptance, Flatpak runtime acceptance, and sustained daily use.

## Promotion rule

The **Beta 0.1 Development source/native-shell, CI renderer-start, and engine-level HTTPS navigation evidence** may be used for development and testing. A binary must not be described as a **render-capable GoreeCloud Browser beta** until every applicable renderer gate above has direct representative runtime evidence. Neither beta designation authorizes production claims.

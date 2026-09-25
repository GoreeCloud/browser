# Building GoreeCloud Browser

GoreeCloud Browser is on the **0.1.0-beta.1** development channel. The repository contains a build-tested Browser core, an optional visible Linux GTK/X11 Glaze shell, the Chromium engine adapter, and an optional CEF runtime path. A successful build is not production-readiness evidence.

The Linux build supports **CMake 3.22 or newer**, including the system CMake supplied by Ubuntu 22.04 / Zorin OS 17-class environments. The dedicated GTK beta-shell CI lane runs on Ubuntu 22.04 so this compatibility remains continuously checked.

## Engine-independent beta core

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DGOREECLOUD_ENABLE_CHROMIUM=OFF \
  -DGOREECLOUD_ENABLE_CEF=OFF \
  -DGOREECLOUD_ENABLE_LINUX_GTK_HOST=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

This uses `DevelopmentEngine`, which is deliberately non-rendering.

## Visible Linux beta shell

Install GTK3/X11 development packages, then configure:

```sh
cmake -S . -B build-gtk \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DGOREECLOUD_ENABLE_LINUX_GTK_HOST=ON \
  -DGOREECLOUD_ENABLE_CHROMIUM=OFF \
  -DGOREECLOUD_ENABLE_CEF=OFF
cmake --build build-gtk --parallel
ctest --test-dir build-gtk --output-on-failure
```

The executable is `goreecloud-browser`. The first beta native host uses GTK3 with an X11 child-surface contract. On a Wayland desktop, use XWayland/X11 for this milestone. Native Wayland engine-surface attachment remains pending.

This configuration produces a visible GoreeCloud/Glaze shell but still uses the non-rendering development engine.

## Chromium adapter scaffold

```sh
cmake -S . -B build-chromium \
  -DGOREECLOUD_ENABLE_CHROMIUM=ON \
  -DGOREECLOUD_ENABLE_CEF=OFF \
  -DBUILD_TESTING=ON
cmake --build build-chromium --parallel
ctest --test-dir build-chromium --output-on-failure
```

This validates the engine abstraction, contexts, views, navigation state, private cleanup contracts, native-surface attachment, and adapter smoke tests without linking a production renderer.

## CEF render path

Use a reviewed CEF binary distribution containing `include/cef_app.h` and `cmake/FindCEF.cmake`:

```sh
cmake -S . -B build-cef \
  -DCMAKE_BUILD_TYPE=Release \
  -DGOREECLOUD_ENABLE_CHROMIUM=ON \
  -DGOREECLOUD_ENABLE_CEF=ON \
  -DGOREECLOUD_ENABLE_LINUX_GTK_HOST=ON \
  -DGOREECLOUD_CEF_ROOT=/absolute/path/to/cef
cmake --build build-cef --parallel
```

The build integrates CEF's binary-distribution CMake targets, `libcef_dll_wrapper`, CEF runtime libraries/resources, subprocess dispatch, GoreeCloud request-context separation, and the GTK/X11 child-content surface.

Runtime environment:

```sh
export GOREECLOUD_BROWSER_RUNTIME_ROOT=/absolute/path/to/runtime
export GOREECLOUD_BROWSER_SUBPROCESS=/absolute/path/to/goreecloud-browser
export GOREECLOUD_BROWSER_RESOURCES=/absolute/path/to/runtime
export GOREECLOUD_BROWSER_LOCALES=/absolute/path/to/runtime/locales
export GOREECLOUD_SEARCH_URL=https://your-authorized-goreecloud-search-endpoint/search
./build-cef/goreecloud-browser
```

`GOREECLOUD_SEARCH_URL` is intentionally required for search behavior. If it is absent or GoreeCloud Search is unavailable, search fails visibly; the Browser does not substitute another search provider. Direct URL navigation remains available.

Private launch examples:

```sh
./build-cef/goreecloud-browser --private
./build-cef/goreecloud-browser --isolated-private
./build-cef/goreecloud-browser https://example.com/
```

## Beta runtime boundary

CEF/Chromium source integration does not by itself satisfy the render-capable beta gate. Real runtime evidence is still required for HTTPS rendering, native child-window behavior, navigation, private-context isolation, cleanup, sandbox/site isolation, and sustained desktop operation. See `docs/BETA_0_1.md`.

## Security requirements

GoreeCloud Browser refuses the production Chromium adapter when sandboxing or site isolation are disabled. Private contexts must use non-persistent storage. Integration convenience must not weaken TLS, certificate, sandbox, site-isolation, authentication, or authorization boundaries.

## Distribution direction

The accepted source is intended to feed Linux Flatpak, Windows `.exe`, Android APK, and later approved artifacts. The current Flatpak manifest is a beta-shell manifest and intentionally leaves CEF disabled until a reviewed CEF runtime module, provenance, sandbox, codecs, and runtime packaging are incorporated.


## Linux Development keyboard controls

The GTK/X11 Development shell currently exposes these Browser-owned keyboard paths:

- `Ctrl+L`, `Ctrl+K`, or `F6` — focus and select the unified address/search field.
- `Ctrl+T` — open a new tab.
- `Ctrl+W` — close the active tab.
- `Ctrl+Tab` / `Ctrl+Shift+Tab` — cycle tabs.
- `Ctrl+PageDown` / `Ctrl+PageUp` — cycle tabs.
- `Ctrl+R` or `F5` — reload or stop.
- `Alt+Left` / `Alt+Right` — Back / Forward.
- `Alt+Home` — Home.
- `Escape` — dismiss an open Browser Development panel and return to the prior content surface.

These controls are Development behavior only. They do not establish accessibility acceptance or production browser completeness.

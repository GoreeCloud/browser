# Building GoreeCloud Browser

GoreeCloud Browser is on the **0.1.0-beta.1** development channel. The repository contains a build-tested Browser core, an optional visible Linux GTK/X11 Glaze shell, the Chromium engine adapter, and an optional CEF runtime path. A successful build is not production-readiness evidence.\n\nThe Linux build supports **CMake 3.22 or newer**, including the system CMake supplied by Ubuntu 22.04 / Zorin OS 17-class environments. The dedicated GTK beta-shell CI lane runs on Ubuntu 22.04 so this compatibility remains continuously checked.

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


## Pinned Linux CEF render-capable Development build

The repository includes a reproducible Development bootstrap for the selected Linux CEF runtime candidate.

Current pin:

- CEF: `152.0.6+g708dc14+chromium-152.0.7977.83`
- Chromium: `152.0.7977.83`
- Platform: `linux64`
- Distribution: `minimal` (Release-only)
- Source: official CEF automated-build service
- Integrity: official CEF `.sha1` sidecar is fetched first and must match the downloaded archive; a local SHA-256 is also recorded in the extracted provenance record.

The pin corresponds to the CEF Stable build selected for the Browser Development render milestone. It is not a claim that the GoreeCloud Browser desktop renderer has passed the render-capable Beta gate.

### Dependencies

On Ubuntu 22.04 / Zorin OS 17-class systems, the existing GTK Development dependencies remain required:

```bash
sudo apt update
sudo apt install -y \
  python3 cmake ninja-build build-essential pkg-config \
  libgtk-3-dev libx11-dev libcurl4-openssl-dev
```

Additional CEF runtime libraries may already be present on a desktop installation. If the dynamic loader reports a missing library, install the matching Ubuntu package rather than disabling a CEF feature or sandbox control.

### One-command bootstrap and build

From the repository root:

```bash
./scripts/build_linux_render_beta.sh
```

The script:

1. selects only the repository-pinned CEF Linux x86_64 minimal distribution;
2. fetches the archive checksum from the official CEF build service;
3. verifies the archive before extraction;
4. rejects unsafe archive paths and special files;
5. records local provenance, including the official SHA-1 and locally computed SHA-256;
6. configures CMake with Chromium, CEF, GTK/X11, tests, and the libcurl download transport enabled;
7. requires the CEF headers to report the same exact pinned version;
8. builds and runs repository tests.

The default local cache is `.cache/cef` and the default build directory is `build-cef-render`. Both can be overridden:

```bash
GOREECLOUD_CEF_CACHE_DIR="$HOME/.cache/goreecloud/cef" \
GOREECLOUD_BROWSER_CEF_BUILD_DIR="$HOME/.cache/goreecloud/browser-render" \
./scripts/build_linux_render_beta.sh
```

### Launch the render candidate

After a successful build:

```bash
./scripts/run_linux_render_beta.sh https://example.com/
```

The launcher sets the runtime/resource/subprocess paths to the exact build output and preserves the CEF/Chromium sandbox requirement.

If CEF specifically reports that the Linux SUID sandbox helper is required and not configured, configure the exact generated helper rather than passing `--no-sandbox`:

```bash
sudo chown root:root build-cef-render/chrome-sandbox
sudo chmod 4755 build-cef-render/chrome-sandbox
```

Do this only for the exact Browser build output you intend to test. GoreeCloud Browser deliberately refuses to initialize its CEF runtime with sandboxing disabled.

### Acceptance boundary

A successful CEF download, compile, or launch is not render-capable Beta acceptance. The runtime must still satisfy the governed gates in `docs/BETA_0_1.md`, including real HTTPS rendering/navigation, multi-tab engine ownership, private-context isolation and cleanup, TLS/sandbox/site-isolation preservation, and real-device runtime evidence.

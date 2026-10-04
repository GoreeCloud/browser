# GoreeCloud Browser — Linux CEF Install Runtime

**Lifecycle:** Development / non-Stable  
**Scope:** render-capable Linux CEF staging and package input  
**Browser baseline:** Glaze V1.7 / `1.7.0`

## Purpose

The Linux CEF build can be installed as a self-contained runtime tree instead of
requiring the development build directory and `run_linux_render_beta.sh`.

This is an install/runtime staging contract. It is not yet a signed distro
package, AppImage, Flatpak, Release Candidate, or production-approved artifact.

## Installed layout

For a conventional prefix such as `/usr`:

- `/usr/bin/goreecloud-browser` — public launcher.
- `/usr/libexec/goreecloud-browser/goreecloud-browser` — private Browser binary.
- `/usr/libexec/goreecloud-browser/goreecloud-browser-subprocess` — CEF subprocess.
- `/usr/libexec/goreecloud-browser/` — pinned CEF binaries/resources/locales.
- `/usr/libexec/goreecloud-browser/assets/branding/goreecloud-browser.svg` — runtime GTK branding.
- `/usr/share/applications/io.goreecloud.Browser.desktop` — desktop entry.
- `/usr/share/metainfo/io.goreecloud.Browser.metainfo.xml` — AppStream metadata.
- `/usr/share/icons/hicolor/scalable/apps/io.goreecloud.Browser.svg` — launcher icon.

The launcher resolves the private runtime relative to its own installed location,
so a staged or alternate installation prefix remains usable.

## Persistent Browser data

Persistent Chromium/CEF profile and site data are deliberately separated from
the immutable runtime payload.

Resolution order:

1. `GOREECLOUD_BROWSER_USER_DATA_ROOT`, when explicitly supplied.
2. `$XDG_DATA_HOME/goreecloud/browser/cef`.
3. `$HOME/.local/share/goreecloud/browser/cef`.

The Development build keeps its existing build-tree fallback when no dedicated
user-data root is supplied.

Private Browser contexts remain governed by the engine privacy contract and must
not use persistent storage.

## CEF runtime contents

The install rules copy the exact pinned minimal CEF payload used by the build,
including:

- `chrome-sandbox`
- `libcef.so`
- `libEGL.so`
- `libGLESv2.so`
- `libvk_swiftshader.so`
- `libvulkan.so.1`
- `v8_context_snapshot.bin`
- `vk_swiftshader_icd.json`
- `chrome_100_percent.pak`
- `chrome_200_percent.pak`
- `resources.pak`
- `icudtl.dat`
- `locales/`

The installed Browser and subprocess use an `$ORIGIN` runtime search path so
the colocated `libcef.so` is resolved without a development-tree RPATH.

## Sandbox packaging requirement

The Browser does not disable Chromium sandboxing.

The CMake staging install preserves `chrome-sandbox` as an executable, but
CMake does not claim production package ownership policy. A final privileged
Linux package must install the sandbox helper with the ownership/mode required
by the target Chromium/CEF platform policy and prove that sandboxing and site
isolation remain enabled on representative systems.

CI may model this package-owned state inside an isolated staging prefix before
launching the installed runtime.

## Validation

The Core CI install smoke must:

1. install the exact CEF build into a temporary prefix;
2. verify the private Browser/subprocess and required CEF payload;
3. verify the private runtime contains no `profile-cache`;
4. launch the public installed wrapper under Xvfb;
5. require CEF initialization and HTTPS main-frame completion;
6. require profile data to appear under the temporary XDG data root instead of
   the private runtime;
7. fail if sandbox-disable or relative-profile diagnostics appear.

Representative native-Wayland installed-package validation is tracked
separately from hosted Xvfb evidence.

## Remaining packaging gates

Still open:

- signed and reproducible distro package generation;
- production sandbox-helper ownership/mode packaging;
- package upgrade/uninstall behavior without profile loss;
- desktop/AppStream validation in package CI;
- launcher association across representative GNOME/KDE/other Wayland desktops;
- package signing and distribution authority;
- AppImage/Flatpak decisions, if adopted;
- representative-device production acceptance.

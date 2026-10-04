# GoreeCloud Browser — Linux Wayland Owner-Device Evidence — 2026-10-03

**Status:** Development runtime evidence  
**Production approved:** No  
**Source revision:** `27e1b8a9ecea3feb184425a72cdf8fd6eb9ea89b`  
**Platform:** Zorin OS 17.3, Linux 6.8.0-138-generic x86_64  
**Desktop session:** Active GNOME/Zorin Wayland session  
**CEF:** `152.0.6+g708dc14+chromium-152.0.7977.83`  
**Chromium:** `152.0.7977.83`

## Purpose

This record captures fresh owner-device Linux evidence for the current GoreeCloud Browser Development renderer. It proves a bounded native-Wayland startup/rendering path on the exact source revision above. It does not promote the Browser to render-capable Beta, Production Acceptance, Stable, Seal, or Anchor maturity.

## Exact-source build evidence

The Browser was checked out in an isolated detached worktree at the exact source revision. The existing user working tree was not modified.

The repository-pinned CEF cache passed its immutable provenance checks and was reused. The render-capable build was configured with Chromium, CEF, GTK3, tests, and the libcurl download transport enabled.

The local build used reduced parallelism to avoid unnecessary device resource pressure. The exact revision completed successfully and produced:

- `goreecloud-browser`
- the dedicated `Release/goreecloud-browser-subprocess`
- `libcef.so`
- `icudtl.dat`
- 220 locale files

All 13 configured CTest targets passed.

## Native Wayland runtime exercise

The active desktop session reported:

- session type: `wayland`
- active: yes
- state: active
- Wayland display: `wayland-0`
- Xwayland remained available separately for Chromium dependencies

The launch explicitly selected the GTK Wayland backend with `GDK_BACKEND=wayland`. The Development-only `GOREECLOUD_BROWSER_FORCE_WINDOWLESS` override was **not** used, so the host selected the software/windowless path through its normal backend detection.

The Browser was launched against `https://example.com/` with runtime diagnostics enabled and with the Chromium sandbox requirement left enabled. The run was intentionally bounded by an 18-second timeout.

Observed Browser/runtime evidence:

- CEF runtime mode selected successfully.
- Browser creation reported `mode=windowless`, `parent=0`, and a 1280×723 content surface.
- Main-frame navigation started for `https://example.com/`.
- Main-frame navigation completed with HTTP 200.
- GTK reported `windowless-frame-presented nonuniform=yes size=1280x723 scale=1`.
- The Browser remained running until the deliberate timeout.

The bounded timeout returned status 124, which is expected for the intentional external timeout and is not a Browser crash result.

## Evidence identity

The retained local runtime log for this exact exercise had SHA-256:

`6216b70d693f0fe467aece3e3f1c826033ada69692dc126ab770c29d468f3e66`

The repository does not treat this hash alone as a substitute for the source revision, workflow evidence, or future reproducible representative-device runs.

## Open observations

The following device/runtime diagnostics remain open and must not be converted into acceptance claims:

- The generated local `chrome-sandbox` helper was not configured as root-owned setuid. The launcher warned about this condition. GoreeCloud Browser did **not** pass `--no-sandbox` or otherwise disable the Browser sandbox requirement. Packaged Chromium sandbox acceptance remains open.
- The device exposes VAAPI 1.14 while the current Chromium runtime reports a 1.17 minimum for that path. Hardware-accelerated video/rendering acceptance remains open.
- Chromium emitted an NSS root-certificate loading diagnostic (`NSS error code -8018`) even though the tested HTTPS request completed with HTTP 200. Certificate-store integration, TLS/certificate presentation, and failure-path acceptance remain open.
- Zygote termination-status diagnostics occurred when the externally bounded run was terminated. Sustained runtime and graceful shutdown acceptance remain open.

## Acceptance boundary

This evidence advances the current Linux Development state by verifying exact-source build/test success and ordinary automatic windowless rendering on a representative owner-device Wayland session.

It does **not** establish:

- human visual-quality acceptance of the rendered page;
- representative native-Wayland pointer, keyboard, IME, popup, clipboard, or drag-and-drop acceptance;
- IBus/Fcitx/XIM interoperability;
- accessibility or assistive-technology acceptance;
- high-DPI or multi-monitor acceptance;
- sustained performance, power, thermal, memory, or long-session acceptance;
- TLS/certificate UI acceptance;
- packaged sandbox/site-isolation acceptance;
- private-context isolation/cleanup acceptance;
- Flatpak, Debian, AppImage, installer, signing, update, rollback, or release acceptance;
- Production Acceptance, Stable, Seal, or Anchor qualification.

Future representative-device evidence should remain bound to an exact Browser source revision and should preserve failed or warning diagnostics rather than silently normalizing them away.

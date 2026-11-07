# GoreeCloud Browser — Linux-first development preview

**Active target:** Linux desktop. Windows and macOS are deferred; there is no release-ready installation for either platform. This is a native Qt Widgets browser prototype with Qt WebEngine, **not** a daily-driver browser.

## Validated Linux environment

The development preview has been exercised on Zorin OS 18.1 (Ubuntu Noble family), Python 3.12 and PySide6 6.11.3. Native Qt startup, clean shutdown and a local HTTP navigation smoke test have passed. X11/XCB startup and clean shutdown also passed under Xvfb. These headless smoke checks do not establish full Wayland/X11, accessibility or cross-distro acceptance.

## Prerequisites

Python 3, python3-venv, pip, a graphical desktop session and compatible Qt graphics libraries. Ubuntu-family dependencies can include libegl1, libopengl0 and libxkbcommon-x11-0. The installer never uses sudo, changes global handlers or installs system packages.

## From source

~~~sh
git clone https://github.com/GoreeCloud/browser.git
cd browser
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python -m goreecloud_browser
~~~

## Install into the Linux application menu

From a trusted, inspected checkout:

~~~sh
python3 scripts/linux/install_user.py
~~~

This creates a separate Python environment under the XDG data home in goreecloud-browser/runtime/venv, and a freedesktop launcher in applications/com.goreecloud.browser.desktop. Default XDG data home is ~/.local/share. The launcher displays "GoreeCloud Browser (Preview)" with a generic system web-browser icon, not a fictional GC UI brand asset.

To remove only the application runtime and launcher (not bookmarks or browsing
profile data), close the app and run:

~~~sh
python3 scripts/linux/uninstall_user.py
~~~

The installer stops rather than overwriting an existing installation or desktop launcher. PySide6 and build dependencies come from the package index unless already cached. It does not set the default browser, request sudo, install a service, or register HTTP URL handling. Application site data and settings are separate from the program folder; do not delete them without explicit consent and recovery planning.

## Validate

~~~sh
python3 -m unittest discover -s tests -v
python3 -m compileall -q goreecloud_browser scripts tests
QT_QPA_PLATFORM=offscreen QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  GC_BROWSER_QT_SMOKE=1 python3 -m unittest discover \
    -s tests -p test_linux_smoke.py -v
QT_QPA_PLATFORM=xcb QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  GC_BROWSER_QT_SMOKE=1 xvfb-run -a -s '-screen 0 1280x800x24' \
    python3 -m unittest discover -s tests -p test_linux_smoke.py -v
desktop-file-validate \
  "$HOME/.local/share/applications/com.goreecloud.browser.desktop"
~~~

The disable-gpu flag is only for headless testing; it does **not** disable the Chromium sandbox. Normal desktop operation should use default GPU/sandbox behavior.

## Security and privacy status

GoreeShield: session-only cookies by default, limited tracker-blocking rules, tracking-link cleaning, and per-session website permission decisions. Wardveil: browser threat protection has **not** been qualified. The interface is native Qt; untrusted websites are rendered by Qt WebEngine with no privileged Python-webpage bridge. "Clear cookies and HTTP cache" does not remove every HTML5 storage item. A separate confirmed website-data reset removes the app-owned Qt profile/cache on the next launch, using a Linux single-instance lock; it preserves bookmarks and settings. No cloud sync, network telemetry, browser-level DNS control, or full content blocker is provided. Startpage remains the search default pending verified GoreeCloud Search maturity.

## Linux acceptance priorities

1. Fully qualify per-user installation, Linux desktop entry, rollback and sandboxing.
2. Audit redirects, TLS handling, POST/signed URLs, tab/window navigation, permissions, site data, downloaded files and profile lifecycle.
3. Validate whole-screen appearance and usability on real X11/Wayland sessions with keyboard, zoom, screen readers, high contrast and both themes.
4. Deliver trustworthy Linux distribution packages, license notices, code signing, browser-engine security patch cadence and rollback.
5. Defer Windows and macOS until Linux acceptance criteria have been achieved.

A passed test or development install does not confer stable-release status.

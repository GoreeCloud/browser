# GoreeCloud Browser — Linux-first development preview

**Active target:** Linux desktop. Windows and macOS are deferred; there is no release-ready installation for either platform. This is a native Qt Widgets browser prototype with Qt WebEngine, **not** a daily-driver browser.

## Validated Linux environment

The development preview has been exercised on Zorin OS 18.1 (Ubuntu Noble family), Python 3.12 and PySide6 6.11.3. Native Qt startup, clean shutdown and loopback HTTP navigation smoke tests passed.
A native Home/New Tab launch and 700x480 scroll/overlap regression check passed
in the laptop's real Wayland session; X11/XCB layout validation also passed under
Xvfb. None of these
checks establishes full Wayland/X11 interaction, accessibility or cross-distro
acceptance.

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

This creates a separate Python environment under the XDG data home in goreecloud-browser/runtime/venv, and a freedesktop launcher in applications/com.goreecloud.browser.desktop. Default XDG data home is ~/.local/share. The launcher displays "GoreeCloud Browser (Preview)" with an **absolute path to the bundled first-party Prism Tabs v5 icon** copied into the new private program runtime. It no longer uses the generic system browser icon. The icon path is validated before writing the freedesktop entry. This does not change the default browser or replace pre-existing installations.

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
QT_QPA_PLATFORM=offscreen QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  GC_BROWSER_QT_SMOKE=1 python3 -m unittest discover \
    -s tests -p test_linux_network_smoke.py -v
QT_QPA_PLATFORM=xcb QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  GC_BROWSER_QT_SMOKE=1 xvfb-run -a -s '-screen 0 1280x800x24' \
    python3 -m unittest discover -s tests -p test_linux_network_smoke.py -v
QT_QPA_PLATFORM=offscreen QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  GC_BROWSER_QT_SMOKE=1 python3 -m unittest discover \
    -s tests -p test_linux_download_smoke.py -v
desktop-file-validate \
  "$HOME/.local/share/applications/com.goreecloud.browser.desktop"
~~~

The disable-gpu flag is only for headless testing; it does **not** disable the Chromium sandbox. Normal desktop operation should use default GPU/sandbox behavior.

## New native Linux chrome and Webspaces

New profiles open in **Light** unless an existing user saved Dark. The top
row is Back, Forward, Refresh, Home, Omnibox, Bookmarks, Extensions, Settings,
Profile; the omnibox nests Verification and Permissions at left and Webspaces
at right. Native Settings expose ChatGPT prompt copying (no automatic send),
per-Webspace cookies and stored session permission decisions (with selective
or confirmed bulk reset), the theme manager, system DNS status and validated
application-wide proxy preferences applied **after restart**. SOCKS5 and
per-Webspace proxy routing require further acceptance. User-created Webspaces
get separate Qt WebEngine profiles and cookies, but not process or IP isolation.

The built-in OISD Small and StevenBlack lists are enabled by default and
refresh over their fixed public HTTPS endpoints when cached content is missing
or older than 24 hours. The initial refresh **does disclose the computer's
connection IP to both list hosts**; browsing URLs and cookies are not sent.
No list is claimed active before valid domains load. Disabled lists do not
refresh, and caches are retained after update failures. Filtering covers
third-party domain-matched subresources, not comprehensive cosmetic ads.
The Privacy Shield exception manager lets users explicitly allow third-party
requests for an exact first-party hostname across **all Webspaces**. Adding an
exception requires confirmation because it may permit tracking; removing it
restores domain filtering for future requests. IP addresses, wildcard patterns
and URL prefixes are not accepted. See [feature status and Privacy Shield
roadmap](PRODUCT_SCOPE.md).

## Security and privacy status

Downloads use a user-chosen filename but land in a private temporary directory
first, then publish atomically on completion with no existing-file overwrite.
Interrupted downloads do not become user-visible files. This does not include
antivirus scanning or trusted-source validation; never execute untrusted files.

GoreeShield: session-only cookies by default, independently enabled domain filters,
tracking-parameter cleaning **only for user-entered/pasted URLs**, conservative
 hostname and Unicode bidi-control validation, and per-session
website permission decisions. HTTP redirects and clicked links are not cleaned;
never assume the cleaner prevents all tracking. Wardveil: browser threat protection has **not** been qualified. The interface is native Qt; untrusted websites are rendered by Qt WebEngine with no privileged Python-webpage bridge. "Clear cookies and HTTP cache" does not remove every HTML5 storage item. A separate confirmed website-data reset removes the app-owned Qt profile/cache on the next launch, using a Linux single-instance lock; it preserves bookmarks and settings. No cloud sync, behavioral telemetry, browser-level DNS control, or full cosmetic content blocker is provided. Startpage remains the search default pending verified GoreeCloud Search maturity.

## Linux acceptance priorities

1. Fully qualify per-user installation, Linux desktop entry, rollback and sandboxing.
2. Audit redirects, TLS handling, POST/signed URLs, tab/window navigation, permissions, site data, downloaded files and profile lifecycle.
3. Validate whole-screen appearance and usability on real X11/Wayland sessions with keyboard, zoom, screen readers, high contrast and both themes.
4. Deliver trustworthy Linux distribution packages, license notices, code signing, browser-engine security patch cadence and rollback.
5. Defer Windows and macOS until Linux acceptance criteria have been achieved.

A passed test or development install does not confer stable-release status.

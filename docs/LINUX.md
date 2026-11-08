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

## Download scanning and consent

This preview's Wardveil download gate inspects files **before** they become
visible at the selected destination, using an optional locally installed
ClamAV `clamscan` from a root-owned, non-writable `/usr/bin` or
`/usr/local/bin` system installation. It does **not** install ClamAV,
download antivirus signatures, or change OS services.

- With trusted ClamAV available: the GUI launches one asynchronous scan at a
  time (90-second timeout), and only publishes if ClamAV exits 0 meaning
  **no signature detection**. The scanner receives explicit
  `--alert-exceeds-max=yes` and `--alert-encrypted=yes` flags: encrypted or
  over-limit content is **blocked**, not accepted as fully scanned. Positive
  alerts (commonly exit 1), errors (including some limit alerts reported with
  exit 2), crash or timeout all fail closed. Private staging is destroyed,
  not kept as a persistent
  malware quarantine. A second simultaneous download requiring a scan is
  discarded with a warning, not saved without one.
- Without trusted ClamAV: **no scanner runs**. After the usual save-path dialog
  the browser asks for separate approval to save an **UNSCANNED** file;
  **No is the default** and discards the file. If explicitly approved, it is
  labeled *Saved — NOT SCANNED*. It is never opened automatically and is saved
  with restrictive permissions.
- A scanner reporting no detection is **not a guarantee**: signatures may be
  outdated, archives may use unsupported encryption or formats, and malware
  families may be unknown. The scanner limits checked are the supported
  file/scan-size and recursion limits, not every internal work bound. The
  browser **does not yet verify signed signature-database freshness**.
  Downloaded programs must not be executed blindly.

ClamAV's CLI exit code semantics are documented at
[clamscan(1)](https://manpages.ubuntu.com/manpages/noble/man1/clamscan.1.html).
The test-only scanner fixture exercises the state transitions and does **not**
demonstrate detection against real threats. The authorized Zorin development
laptop did not have ClamAV installed at this validation, and no system
installation or real official signature database is claimed. For additional
verification, the Ubuntu archive ClamAV 1.5.4 CLI and libclamav12 packages
were downloaded using verified apt metadata and checked against their signed
package-index SHA-256 hashes, then extracted into a private temporary directory.
The real engine returned no detection for a harmless control, an alert for the
harmless EICAR antivirus test pattern, and a nonzero failure for an over-limit
file when tested through GoreeCloud Browser's native asynchronous gate. The
test used a disposable SHA-256 `.hsb` test signature, **not** an official
Cisco Talos ClamAV database. It passed offscreen, X11/Xvfb and Wayland.
Notably, an attempted `--fail-if-cvd-older-than=7` freshness option returned
success while missing an EICAR match in this custom-signature configuration;
that option is deliberately excluded until a trustworthy production signature
freshness and scan-completeness preflight can be implemented. No OS-wide
ClamAV service, DB, trust store or default-browser setting was changed.

The optional real-engine smoke test is disabled in routine test discovery. To
run it after *independently verifying* a trusted ClamAV binary in an isolated
environment, set `GC_BROWSER_REAL_CLAMSCAN` to its absolute executable path,
`GC_BROWSER_QT_SMOKE=1`, and supply any needed loader library directory
or `CVD_CERTS_DIR` within your test environment. The test generates a local
`.hsb` signature and EICAR control, and calls only the selected local engine:

~~~sh
GC_BROWSER_QT_SMOKE=1 GC_BROWSER_REAL_CLAMSCAN=/path/to/verified/clamscan \
QT_QPA_PLATFORM=offscreen QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  python3 -m unittest discover -s tests -p test_linux_real_clamav_smoke.py -v
~~~

For reproducible CI, `scripts/linux/run_clamav_engine_acceptance.py` obtains
Ubuntu packages from configured APT metadata, verifies each package's SHA-256
against its authenticated APT index record, extracts them into disposable
storage and runs this same real-engine Qt test with an offline local test
signature. It does not run `apt install`, `freshclam`, or a daemon. CI uses a
pinned Ubuntu 24.04 runner; ordinary application and unit tests do not require
the test-only ClamAV packages.

This does not change the browser's trusted production scanner path. For
ClamAV's alert and signature-management semantics see
[official scanning documentation](https://docs.clamav.net/manual/Usage/Scanning.html)
and [signature documentation](https://docs.clamav.net/manual/Signatures.html).

To run the optional native test using a disposable local HTTP server and
a test-only process (no external malware or internet access):

~~~sh
GC_BROWSER_QT_SMOKE=1 GC_BROWSER_FILTER_NETWORK=off QT_QPA_PLATFORM=offscreen \
QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  python3 -m unittest discover -s tests -p test_linux_antimalware_smoke.py -v
~~~

## Unpublished wheel integrity and reproducibility gate

The Python wheel is **not** a signed Linux distribution or an official release.
GitHub CI builds the wheel twice on the same pinned Ubuntu 24.04 runner, with
`SOURCE_DATE_EPOCH=1700000000` and disabled pip wheel caching, then verifies
that the archives are byte-for-byte identical. It also validates package
identity and pinned dependencies, the `goreecloud-browser` entry point, the
wheel `RECORD` SHA-256 hashes and sizes, and byte equality with the checked-out
first-party Python modules and approved icon assets. The mandatory source
`LICENSE`, `NOTICE` and `docs/THIRD_PARTY.md` must be packaged unchanged under
wheel `.dist-info/licenses/`. Unexpected files, symlinks, traversal paths,
missing notices, or tampered bytes fail CI. The first checked wheel is also
installed under a disposable, non-system target and imported with its bundled
Prism Tabs asset (no default-browser setting or user profile is changed).
The verifier uses Python 3.11+;
it does not change the browser's Python >=3.10 runtime compatibility target.

~~~sh
SOURCE_DATE_EPOCH=1700000000 python -m pip wheel --no-deps --no-cache-dir \
  --wheel-dir /tmp/browser-wheel-a .
python3 scripts/linux/verify_wheel_integrity.py /tmp/browser-wheel-a/*.whl \
  --source-root .
~~~

**Scope:** reproducibility is tested only for two builds on the **same CI
runner/toolchain**. Cross-run, cross-host, cross-architecture and independent
rebuild equivalence, third-party SBOM completeness, audited Qt redistribution
obligations, signed provenance, a system package and managed updates/rollback
remain open Linux release gates. No wheel is uploaded or published by this CI.

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
# Opt-in real-network site exception check; .invalid hosts resolve only in this process
GC_BROWSER_QT_SMOKE=1 GC_BROWSER_FILTER_NETWORK=off QT_QPA_PLATFORM=offscreen \
QTWEBENGINE_CHROMIUM_FLAGS='--disable-gpu --no-proxy-server --host-resolver-rules="MAP news.browser-test.invalid 127.0.0.1, MAP ads.browser-test.invalid 127.0.0.1"' \
  python3 -m unittest discover -s tests -p test_linux_site_exception_network_smoke.py -v
desktop-file-validate \
  "$HOME/.local/share/applications/com.goreecloud.browser.desktop"
~~~

The disable-gpu flag is only for headless testing; it does **not** disable the Chromium sandbox. Normal desktop operation should use default GPU/sandbox behavior.

## Wardveil Qt WebEngine patch-level baseline

The build checks the installed PySide6 binding, Qt WebEngine runtime, Chromium
source base and the Qt-reported security-patch metadata. At the reviewed
Linux 6.11.3 baseline, `qWebEngineChromiumVersion()` reports the Chromium
**140.0.7339.264** base and `qWebEngineChromiumSecurityPatchVersion()`
reports **155.0.8059.85** as the patch reference. The latter is not the same
as the Chromium source base, and neither API provides a full CVE audit.

~~~sh
python3 scripts/linux/check_webengine_patch_baseline.py
~~~

Run with the pinned PySide6 environment; GitHub CI executes it after Qt imports.
The checker refuses a changed/unpinned PySide6 dependency, incompatible Qt
WebEngine runtime, downgraded patch metadata or an upgrade to a new Qt version
before that version's official Chromium mapping is independently reviewed.
Qt 6.11.3 was published October 26, 2026 as the final Qt 6.11 patch release;
a controlled Qt 6.12 upgrade requires separate testing and license review.
See [Qt's upstream Chromium version table](https://wiki.qt.io/QtWebEngine/ChromiumVersions)
and the [Qt 6.11.3 announcement](https://www.qt.io/blog/qt-6.11.3-released).
This is a **release-monitoring checkpoint**, not real-time vulnerability
scanning, proof of Chromium CVE coverage or a replacement for Qt patch updates.

## Wardveil WebEngine sandbox preflight and offline process audit

The Linux browser now **refuses explicit sandbox-disabling settings** before
creating any QWebEngine profile or accessing browser site data. Do not set
`QTWEBENGINE_DISABLE_SANDBOX` or pass `--no-sandbox`, `--single-process`,
`--disable-seccomp-filter-sandbox`, or comparable unsafe flags in the launcher,
CLI or `QTWEBENGINE_CHROMIUM_FLAGS`. The browser shows a refusal message;
there is no supported silent insecure fallback. Unset the override and restart.

From a trusted local checkout with PySide6 installed, an optional **offline,
read-only** process audit can be run using the normal desktop display:

~~~sh
python3 scripts/linux/audit_webengine_sandbox.py
~~~

This command uses a temporary XDG profile, loads only local inline HTML, and
checks descendant Linux `/proc` status and namespace symlinks. It does not
change system DNS, user namespaces, system sandbox policy, installed packages,
profiles or the default browser. An observed seccomp-filtered, namespaced
Qt WebEngine child is **evidence of one process layer**, not a guarantee that
all renderer, GPU, plugin and network processes are equally protected. An
inconclusive result is a release blocker, not a reason to disable sandboxing.
The Zorin host test observed one child with seccomp mode 2, NoNewPrivs 1 and
separate user namespace; other child types did not all report the same layers.

## Content-security defaults for every Webspace

GoreeCloud Browser explicitly disables active mixed content, location access
from insecure origins, cross-file and remote URL access from local HTML,
JavaScript direct clipboard/paste access, popup window creation and hyperlink
ping auditing in **every** Qt profile (including newly created Webspaces).
This minimizes reliance on version-specific Qt defaults. It is separate from
GoreeShield's domain filter and does not imply a complete CSP, WebRTC,
certificate, or mixed-content audit. Some local file-based web applications may
be restricted; normal HTTP/HTTPS browsing and native Home/New Tab remain.

The offline Linux tests verify policy inheritance and demonstrate actual
local-file isolation using two disposable HTML files. The test-only positive
control shows cross-file access with Qt's default enabled setting; the protected
profile does not allow it. No user document or operating-system setting is
changed.

~~~sh
GC_BROWSER_QT_SMOKE=1 GC_BROWSER_FILTER_NETWORK=off QT_QPA_PLATFORM=offscreen \
QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  python3 -m unittest discover -s tests -p test_linux_content_security_smoke.py -v
GC_BROWSER_QT_SMOKE=1 GC_BROWSER_FILTER_NETWORK=off QT_QPA_PLATFORM=offscreen \
QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  python3 -m unittest discover -s tests -p test_linux_file_access_smoke.py -v
~~~

## HTTPS certificate and origin protection

GoreeCloud Browser rejects Qt WebEngine certificate errors without an unsafe
bypass prompt. The startup preflight also refuses explicit Chromium switches
such as `--ignore-certificate-errors`, `--allow-insecure-localhost`,
`--allow-running-insecure-content`, `--disable-web-security`, and insecure-origin
trust overrides. This is a Wardveil security guard, not a substitute for
comprehensive browser/network security testing.

An opt-in Linux acceptance test starts an **ephemeral loopback HTTPS server**
with a newly generated, untrusted self-signed certificate and verifies that
Qt rejects the connection without delivering the test HTML:

~~~sh
GC_BROWSER_QT_SMOKE=1 GC_BROWSER_FILTER_NETWORK=off \
QT_QPA_PLATFORM=offscreen QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu \
  python3 -m unittest discover -s tests -p test_linux_tls_rejection_smoke.py -v
~~~

The test requires OpenSSL, creates temporary key material only inside the
isolated test directory, and never installs/trusts a CA or contacts public
websites. Success covers only this local authority error; expired/mismatched
certificates, revocation, HSTS, certificate transparency and mixed-content
behavior remain release-acceptance gates.

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
Interrupted downloads do not become user-visible files. The optional
ClamAV gate blocks alerts/scan failures, but **does not** certify an unscanned
download, official signature freshness or trusted-source authenticity.
Never execute untrusted files blindly.

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

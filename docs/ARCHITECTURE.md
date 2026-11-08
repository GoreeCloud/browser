# Architecture and current limitations

**Status:** Linux-first development preview, 2026-11-06; source features and
Linux smoke checks do not prove a hardened or distributed app. Windows/macOS
implementation and acceptance are explicitly deferred.

## Architectural decision

Qt Widgets supplies native desktop chrome and platform accessibility APIs;
Qt WebEngine supplies a maintained Chromium-based web renderer. A Tauri shell
would add another webview but not by itself deliver browser-specific navigation,
profiles or interceptors. Qt is selected here as the direct desktop toolchain.

## Trust boundaries

- **Native UI:** Qt controls tabs, Home, New Tab, menus, preferences, bookmarks
  and file-save decisions. Native pages do not load remote content. The Home
  and New Tab widgets have a minimum usable content height inside theme-aware
  native scroll areas; at 700x480, controls remain separated and reachable by
  vertical scrolling instead of shrinking into overlapping elements.
- **Untrusted sites:** Rendered in Qt WebEngine; no privileged Python/web bridge.
- **Network:** Qt WebEngine handles TLS and OS DNS. A native request
  interceptor blocks known third-party trackers and domains loaded from the
  enabled OISD Small and StevenBlack subscriptions. Only fixed public HTTPS
  list origins are fetched for updates, never page URLs or cookies. Caches
  are parsed, bounded and atomically replaced with last-known-good fallback;
  lists are not cosmetic filters. Explicit exact-host site exceptions are
  stored in local QSettings and applied across all Webspaces to the first-party
  origin of future requests, with confirmation before weakening protections.
  The exception does not match subdomains or arbitrary URL prefixes and does
  not affect browser capability permissions. Unknown first-party contexts are
  not exempt. A real Qt WebEngine loopback test with process-local `.invalid`
  hostname resolution verifies block/allow/restore on a cross-origin image;
  it does not alter OS DNS or prove universal website compatibility.
  **Tracking-parameter cleaning only
  applies before navigating an entered or pasted address**, when enabled.
  Clicked links and HTTP redirects are **not** automatically cleaned: a local
  integration test showed that a redirect can deliver its tracking parameter
  to the server even if a request interceptor changes the displayed URL.
  That misleading interceptor rewrite was removed. Full DNS control, MITM, VPN
  and certificate overrides are not implemented.
- **Persistence:** QSettings stores preferences, Webspace registry and bookmarks
  locally. Default and custom Webspaces have separate Qt WebEngine profiles,
  cookie jars, HTML5 storage and cache (not per-process/IP isolation). Qt
  WebEngine can retain visited-link data. Session-only
  cookies are the default. The cookie/cache-clear action is narrower than the
  separately confirmed full-site reset on the next launch.
- **Linux single-instance safety:** An advisory OS lock prevents two browser
  processes sharing any Webspace's writable WebEngine profile. An explicit reset
  marker is processed only before creating any pages, while holding that lock;
  only app-owned default and Webspace web-profile/cache directories are eligible. Bookmarks
  and settings are deliberately preserved, and symlinked targets are rejected.
- **Downloads:** Require a native save prompt; Qt WebEngine writes only into
  per-download private temporary directories. On successful completion the
  browser publishes the file to the chosen destination with atomic no-clobber
  link operations; cross-device destinations use a private fully copied sibling
  file and atomic link. Existing files and symlink destinations are rejected.
  A cancelled/interrupted download is discarded and never auto-opened. Partial
  temporary downloads may remain after a process crash until OS temp cleanup.
  A Wardveil scan gate now runs a *root-owned system-installed* ClamAV
  command asynchronously against the private staged file, with a 90-second
  timeout and one concurrent scanner limit. Exit code 1 (detected threat),
  scanner failure, timeout, or staged-file metadata changes prevent publication
  and delete staged bytes; a scan result of 0 means **no detection**, not safe
  or fully scanned. On Linux without trusted ClamAV, the user must explicitly
  approve saving an **UNSCANNED** file (default refusal); Downloads shows this
  state and it never auto-opens. A scanner-busy new download is discarded with
  warning rather than bypassing checks. The scanner is explicitly passed
  `--alert-exceeds-max=yes` and `--alert-encrypted=yes`: encrypted archives
  and files that hit scanner size/recursion limits must not be treated as
  safely scanned. The QProcess exit status remains authoritative: both alert
  and failure statuses prevent publication, even when ClamAV reports a limit
  alert with exit code 2. The GUI describes ClamAV alerts generically because
  they are not necessarily infections. An isolated Ubuntu ClamAV 1.5.4
  executable was exercised through the actual native browser gate using an
  EICAR harmless test pattern, a benign control and an over-limit control,
  on Linux offscreen, X11/Xvfb and Wayland. Ubuntu 24.04 CI also runs this
  integration with package SHA-256 verification and user-scoped extraction.
  **The test-only .hsb signature is
  not the production signed ClamAV database.**
  ClamAV's `--fail-if-cvd-older-than` option was *not* enabled: with the
  isolated test-only signature, a real 1.5.4 binary returned code 0 without
  detecting EICAR when that flag was used, so database freshness cannot be
  inferred from it without additional acceptance. These protections do
  **not** include verified official signature freshness, signature updates,
  comprehensive malware detection, a persistent quarantine, authenticity
  verification or mature download history. Scanning an untrusted
  file with a privileged or vulnerable engine has its own security implications.
  Saved files remain untrusted and must not be executed blindly.

- **Proxy:** Validated application-wide System/Direct/HTTP/SOCKS5 settings
  are applied before profile creation on the next restart through Qt Network.
  HTTP proxy routing has a local-WebEngine test; SOCKS5, PAC, authentication
  and per-Webspace proxies remain separate acceptance gates. Python-based
  filter updates pause outside System proxy mode to avoid unexpected bypass.
- **Native chrome:** Founder-specified nine-item glyph toolbar and omnibox
  affordances use original monochrome SVG strokes tinted from GC UI tokens.
  Native Settings contain separate ChatGPT prompt, Webspace, cookie, DNS,
  proxy and theme sections. ChatGPT is launched only by explicit action;
  prompts are copied to clipboard only at user request, never sent automatically.

## Safeguards

- **Wardveil WebEngine startup security preflight (Linux):** a pure-Python
  configuration guard refuses known explicit sandbox-disabling Qt environment
  settings and Chromium options (including `--no-sandbox`, `--single-process`,
  `--disable-seccomp-filter-sandbox`, and network-service in-process mode)
  before a QWebEngineProfile or browser data path is used. The launcher displays
  an error rather than stripping flags or launching insecurely. The guard does
  **not** establish that OS sandbox layers succeeded. A separate offline
  `scripts/linux/audit_webengine_sandbox.py` uses a temporary XDG profile and
  reads only descendant process metadata from Linux `/proc`. On the authorized
  Zorin test host, 4 WebEngine descendants were observed, 3 with separate user
  namespaces, 1 with seccomp mode 2 and a separate user namespace. This is
  **partial evidence, not certification of every renderer/network/GPU role**.
  Kernel user namespaces, seccomp, AppArmor and distribution permissions still
  require broader real-device acceptance. Qt references:
  [platform sandbox notes](https://doc.qt.io/qt-6/qtwebengine-platform-notes.html)
  and [WebEngine debug options](https://doc.qt.io/qt-6/qtwebengine-debugging.html).

- **Wardveil Webspace content security:** Every default and newly created Qt
  WebEngine profile is configured through one first-party `web_security.py`
  policy before page construction. It disables `AllowRunningInsecureContent`,
  `AllowGeolocationOnInsecureOrigins`, `LocalContentCanAccessFileUrls`,
  `LocalContentCanAccessRemoteUrls`, JavaScript clipboard/paste, JavaScript
  window-opening and hyperlink ping auditing. `file://` navigation is already
  refused by the browser UI; local HTML file isolation is additional defense
  in depth for future page/script interactions. An offline, disposable fixture
  verifies Qt can read a neighboring file with its original default but **not**
  with this new setting; profile and page policy parity is tested across
  Webspaces. This does not certify all mixed-content, WebRTC, CSP or browser
  permissions behavior, and may restrict local file-based web applications.
- **Wardveil strict HTTPS errors:** BrowserPage explicitly rejects every
  `QWebEngineCertificateError`; there is no user, page or session bypass path.
  Preflight also rejects common Chromium CLI switches for ignoring certificate
  errors, accepting insecure localhost, disabling same-origin restrictions or
  allowing active mixed content. A disposable local HTTPS server with a fresh
  self-signed leaf cert verifies the failure signal, unsuccessful page load and
  absence of HTTP content delivery. It **does not** establish revocation,
  HSTS, hostname mismatch or every TLS and mixed-content failure mode.
  Reference: [Qt certificate errors](https://doc.qt.io/qt-6/qwebenginecertificateerror.html).
- Typed addresses reject script, file, custom schemes, userinfo credentials,
  ambiguous percent-encoded hostnames, malformed DNS labels and Unicode bidi
  control characters. Qt additionally checks parser validity before loading.
  IPv6 literals and legitimate IDNA hostnames remain supported.
- HTTPS is chosen for a bare hostname; explicit HTTP addresses remain usable.
- Signed URLs are exempt from typed-address cleaning. HTTP POST requests
  and redirects are not rewritten; a local loopback regression test covers
  signed GET URLs, redirects, encoded query parameters and form POST bodies.
- TLS/certificate errors are not overridden.
- Websites must request capability permission explicitly; accepted/denied
  permissions are held only in memory for the running session. The native
  Permissions glyph and Settings section expose the current Webspace's stored
  per-origin decisions (not other Webspaces) and permit reset to Ask, with
  confirmation before resetting every recorded decision. Consent prompts show
  the exact requesting origin, its capability and the top-level website origin,
  without displaying page path/query tokens. Qt may not expose transient media
  capture decisions through its list; resetting stored state does not guarantee
  an active stream stops. Close the relevant tab to stop ongoing activity.
- No telemetry, external code bridge or automated remote account login.

## Wardveil engine patch baseline and maintenance

The Linux CI runtime now checks that `pyproject.toml` and `requirements.txt`
agree on a single explicit PySide6 version and that actual PySide6 and Qt
WebEngine versions match. For the reviewed **6.11.3** preview, the Qt-published
WebEngine Chromium source base is **140.0.7339.264** and the Chromium security
patch metadata is **155.0.8059.85**. The gate rejects an older reported patch
level or an unreviewed new Qt branch rather than silently interpreting the
base Chromium number as the effective security patch number. Source:
[Qt's Chromium versions table](https://wiki.qt.io/QtWebEngine/ChromiumVersions)
and [Qt 6.11.3 release notes](https://www.qt.io/blog/qt-6.11.3-released).

This checkpoint is **not** a CVE database, vulnerability scan, confirmed
remediation of every Chromium issue, or a guarantee that the Qt release is
fully supported indefinitely. New Qt/Chromium releases require an explicit
dependency/version review, functional and sandbox/browser regression testing,
and an updated verified baseline. Runtime/browser data is not touched by this
read-only CI check.

## GitHub Actions CI trust boundary (Wardveil)

Linux CI runs external GitHub-maintained Actions before building artifacts.
To avoid implicit upgrades through mutable major-version tags, checkout and
Python setup use these immutable, upstream-verified Git commits:

- `actions/checkout` v4 branch head as of November 7, 2026:
  [`11d5960a326750d5838078e36cf38b85af677262`](https://github.com/actions/checkout/commit/11d5960a326750d5838078e36cf38b85af677262)
- `actions/setup-python` v5.6.0:
  [`a26af69be951a213d495a4c3e4e4022e16d87065`](https://github.com/actions/setup-python/commit/a26af69be951a213d495a4c3e4e4022e16d87065)

`scripts/linux/check_action_pins.py` validates all source-controlled workflow
files without third-party Python dependencies, and must pass immediately after
checkout. It rejects mutable/tag references, unreviewed third-party Actions,
changed approved commits, and non-read-only token permissions. A deliberate
change to the allowlist needs review against upstream source, regression tests,
and a full CI run. This checker uses intentionally limited YAML surface parsing
rather than a general YAML analyzer, and neither pins Ubuntu APT packages nor
certifies third-party Actions code, identity/signature provenance, GitHub runner
isolation, or a release artifact. No secrets are used by this workflow.

## Linux packaging provenance and integrity

The source uses `pyproject.toml` to build a pure-Python wheel, but PySide6/Qt
and Qt WebEngine remain separately distributed LGPL/GPL and third-party
runtime dependencies. The project explicitly declares its first-party Apache-2.0
license, NOTICE and `docs/THIRD_PARTY.md` in PEP 639 `project.license-files`.
The standalone `scripts/linux/verify_wheel_integrity.py` checks both the
standard wheel RECORD hash/size manifest and byte equality with the current
first-party source and icon files, rejects unexpected/symlink/traversal archive
members, and checks metadata, pinned Qt dependency and legal notices. Twelve
positive/negative unit tests exercise the checker. CI independently builds two
unpublished wheels with a fixed source timestamp and compares their bytes on
the same Ubuntu runner, then installs one into an isolated target to verify the
installed Python package imports with its icon before testing native Qt. This does not verify Qt's
binary supply chain, code-sign or attest the wheel, or qualify multi-machine
reproducibility or stable installation/upgrades. Release publication remains
blocked by other Linux acceptance requirements.

## GoreeCloud UI

Native theme values mirror official GC UI semantic roles, Lagoon accent,
light/dark values, radius and spacing inspected from the canonical
[GoreeCloud/gc-ui](https://github.com/GoreeCloud/gc-ui) styles/tokens.css
on November 6, 2026. The Qt adapter is **not** a second design authority.
Switch to an approved native token adapter once available.\n\n**Browser family identity:** The founder-approved Prism Tabs v5 color icon is shared with Android Browser Mobile, with a single canonical SVG in [brand-assets](https://github.com/GoreeCloud/brand-assets/blob/1b12ecbb043c89b4f461e68ce2cd83b5fd70eb3f/products/browser-mobile/app-icon.svg). Local PNG source fingerprints and wheel package data are validated. The Qt window and native Home/New Tab views use its QIcon; Linux per-user installation provides its own validated desktop launcher icon path. Details and remaining platform gates: [branding](BRANDING.md).

## Acceptance and roadmap

1. **Linux only:** qualify the Qt user installer and desktop launcher on
   representative Linux distributions; check launcher isolation and sandboxing.
2. **Linux only:** test redirects, POST/signed links, origins, permissions,
   cookies/site data, downloads, TLS errors, and anti-spoof URL display.
3. **Linux only:** complete full-screen UI, keyboard, contrast, zoom,
   assistive-technology, Wayland/X11 and light/dark acceptance on real devices.
4. Implement site-data management, secure download tracking, private profiles,
   content filtering, history controls and bookmark portability.
5. Define Linux dependency patch cadence, secure distributable bundles, code
   signing, update policy and rollback before any stable release.
6. Enable GoreeCloud Search only after independently verified stability.
7. **Later:** assess Windows and macOS implementations and acceptance after
   the Linux development-preview gates are materially satisfied.

A source commit or passing CI check is **not** a stable browser release.

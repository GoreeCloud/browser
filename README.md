# GoreeCloud Browser

**Active platform: Linux first.** Windows and macOS are deferred until the Linux
experience is sufficiently validated; neither is being qualified or packaged yet.

A **native Qt desktop-browser development preview** for GoreeCloud, implemented with Python,
PySide6 Qt Widgets and Qt WebEngine (Chromium rendering). Original first-party application
code, not a browser-engine fork.

**Status: v0.2.3 development preview. Not a production or daily-driver release.**

## Implemented in source

- Native Qt desktop navigation, tabs, back/forward/reload, and shortcuts.
- **Light mode by default**, with existing Light/Dark user preferences preserved.
- Founder-directed **glyph-only toolbar in exact order:** Back, Forward, Refresh,
  Home, Omnibox, Bookmarks, Extensions, Settings, Profile. Within the omnibox:
  Verification and Permissions at left, Webspaces at right. Icons are vector
  glyphs with accessible names, not text-only symbols.
- **Native Settings page:** theme selector; Privacy Shield source status;
  Webspaces manager; tab/download managers; ChatGPT.com user-copyable prompt
  templates; cookie inspector; OS DNS status; global proxy configuration.
  Unsupported extension and DNS controls explicitly report their limitations.
- **Wardveil sandbox startup preflight (Linux):** refuses known explicit
  Qt WebEngine sandbox-disabling settings before profile access, including
  `QTWEBENGINE_DISABLE_SANDBOX`, `--no-sandbox` and `--single-process`.
  A separate offline `/proc` audit can observe namespace/seccomp evidence.
  Neither check certifies every renderer or a hardened daily-driver browser.
- **Native Website Permissions Center:** the omnibox Permissions glyph opens
  per-Webspace, per-origin stored permission decisions, with explicit selected
  and confirmed-all reset to Ask; capability prompts show the requesting origin
  and top-level website, not only an ambiguous hostname. Qt may omit transient
  camera/screen-capture decisions; resetting a decision does not stop ongoing
  device capture. Close the active tab to stop an ongoing session.
- **Webspace containers** use separate Qt WebEngine cookie, cache and HTML5
  storage profiles. Their isolation is tested against a local HTTP server;
  they do not provide separate OS processes, IP addresses or per-space proxies.
- **Privacy Shield per-site compatibility exceptions:** native manager accepts
  only exact DNS hostnames, with explicit confirmation before disabling the
  built-in third-party domain blocker on that website. Exceptions are local,
  reversible, and apply **across all Webspaces**; no wildcard or suffix matches.
  A missing first-party origin never receives an exception. A reload applies
  changed protection to previously loaded content.
- **Default enabled OISD Small and StevenBlack domain subscriptions** with
  fixed-source HTTPS retrieval, strict parsing, 24-hour cache checks and
  last-known-good fallback. Only loaded rules are enforced; status is visible.
  This is domain-level subresource blocking, not cosmetic ad filtering.
- **Global System/Direct/HTTP/SOCKS5 proxy settings** applied on next restart;
  local HTTP proxy routing verified. Authentication/PAC and per-Webspace routing
  remain unqualified. DNS still follows operating-system settings.
- **Founder-approved Prism Tabs v5 browser identity** shared with [GoreeCloud/browser-mobile](https://github.com/GoreeCloud/browser-mobile): bundled 48/256px icons for the Qt window, native Home/New Tab mark, and the Linux desktop launcher. Canonical [brand-assets source](https://github.com/GoreeCloud/brand-assets/blob/1b12ecbb043c89b4f461e68ce2cd83b5fd70eb3f/products/browser-mobile/app-icon.svg); exact source and raster integrity are locally tested. See [brand provenance](docs/BRANDING.md).
- **Staged, user-confirmed downloads:** private temporary storage; publish completed
  files without overwriting existing destinations. Cancelled/failed files are
  discarded; downloaded content is never automatically opened or executed.
- **Distinct, entirely native Home and New Tab pages**; neither loads remote content.
  Both scroll instead of crushing or overlapping their controls at the minimum
  700×480 Linux window size, with theme-aware native scrollbar styling.
- Startpage search for text queries. Bare hosts use HTTPS by default. GoreeCloud Search
  is **not** enabled until it passes an independently verified stability gate.
- Opt-in-controllable cleaning of **entered/pasted** URLs for known tracking parameters,
  preserving non-tracking query encoding. Signed URLs are exempt. **Clicked links and
  site redirects are not automatically sanitized; do not assume complete protection.**
- Conservative entered-address validation rejects ambiguous percent-encoded hostnames,
  invalid DNS labels, bidi control characters, and URLs rejected by Qt's parser.
- Built-in third-party tracker and subscribed-domain blocker; **not** a
  complete ad blocker, anti-fingerprinting defense or full Privacy Shield.
- Local bookmark storage with a 500-bookmark preview cap.
- Session-only cookies by default; opt-in persistence; cookie/HTTP-cache
  clearing; and a separately confirmed **full website-data reset on restart**.
  An OS-level Linux lock prevents concurrent processes using the same profile.
  The cookie/HTTP-cache action alone does not clear HTML5 site storage.
- Explicit user consent for website capability requests.
- Light/dark native Qt theme adapted from the approved **GoreeCloud UI** Lagoon palette.

Custom DNS/DoH, account sync, extension installation, private windows, cosmetic
filtering, signed release packages and auto-updates are **not implemented**.
Enabled list updates may connect to the OISD and StevenBlack HTTPS hosts at
startup; **native Home and New Tab do not load remote website content**.
Details: [feature status and Privacy Shield roadmap](docs/PRODUCT_SCOPE.md).

## Run from source

Requires Python **3.10–3.14**, a graphical **Linux** desktop, and a supported
Qt WebEngine runtime. Local development has been exercised on Zorin OS 18.1;
other Linux distributions still need acceptance testing.

~~~sh
git clone https://github.com/GoreeCloud/browser.git
cd browser
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python -m goreecloud_browser
~~~

On Linux, ensure the desktop distribution provides Qt WebEngine runtime dependencies,
including libEGL/libOpenGL and XKBCommon X11. Do not disable WebEngine sandboxing
for routine use.

### Install into the Linux application menu (development preview)

The Linux-only per-user installer creates an isolated Python environment and a
freedesktop launcher, without root access or changing the system default browser:

~~~sh
python3 scripts/linux/install_user.py
~~~

See [Linux installation and validation](docs/LINUX.md) before installing. It
refuses to overwrite an existing install; this is not yet a managed updater.

## Tests

~~~sh
python3 -m unittest discover -s tests -v
python3 -m compileall -q goreecloud_browser tests
~~~

Linux CI checks pure helpers, compilation, Qt imports, the Python package,
native page lifecycle and isolated HTTP navigation/redirect/POST smoke tests
through both offscreen and virtual X11 (Xvfb) backends. It also verifies
Privacy Shield exact-host exceptions against **real Qt WebEngine HTTP requests**
using temporary RFC-reserved `.invalid` hosts mapped to 127.0.0.1 by
process-local Chromium resolver flags, with no OS DNS/hosts-file changes.
A short local Wayland session smoke was also verified on Zorin OS; CI does not
emulate a true interactive Wayland desktop. This does not establish full Linux
visual, accessibility, web-compatibility or browser security acceptance.
Windows/macOS testing remains deferred.

## Shortcuts

| Action | Shortcut |
| --- | --- |
| New / close tab | Ctrl+T / Ctrl+W |
| Focus address | Ctrl+L |
| Reload | Ctrl+R or F5 |
| Home | Alt+Home |
| Back / forward | Alt+Left / Alt+Right |
| Bookmark page | Ctrl+D |
| Zoom | Ctrl+Plus / Ctrl+Minus / Ctrl+0 |

See [feature status and Privacy Shield roadmap](docs/PRODUCT_SCOPE.md),
[architecture and limitations](docs/ARCHITECTURE.md) and
[third-party notes](docs/THIRD_PARTY.md).

**GoreeShield** governs privacy choices; **Wardveil** governs cybersecurity;
GoreeCloud Mesh integration must respect both. The UI follows **GoreeCloud UI**.
No telemetry, remote homepage or cloud account is configured by this code.

## License

Original source: [Apache License 2.0](LICENSE). Qt/Qt WebEngine and Chromium components
have separate obligations; see [NOTICE](NOTICE).

Copyright 2026 GoreeCloud.

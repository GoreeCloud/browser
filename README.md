# GoreeCloud Browser

**Active platform: Linux first.** Windows and macOS are deferred until the Linux
experience is sufficiently validated; neither is being qualified or packaged yet.

A **native Qt desktop-browser development preview** for GoreeCloud, implemented with Python,
PySide6 Qt Widgets and Qt WebEngine (Chromium rendering). Original first-party application
code, not a browser-engine fork.

**Status: v0.1.3 development preview. Not a production or daily-driver release.**

## Implemented in source

- Native Qt desktop navigation, tabs, back/forward/reload, downloads and shortcuts.
- **Distinct, entirely native Home and New Tab pages**; neither loads remote content.
- Startpage search for text queries. Bare hosts use HTTPS by default. GoreeCloud Search
  is **not** enabled until it passes an independently verified stability gate.
- Opt-in-controllable cleaning of **entered/pasted** URLs for known tracking parameters,
  preserving non-tracking query encoding. Signed URLs are exempt. **Clicked links and
  site redirects are not automatically sanitized; do not assume complete protection.**
- Conservative entered-address validation rejects ambiguous percent-encoded hostnames,
  invalid DNS labels, bidi control characters, and URLs rejected by Qt's parser.
- Small third-party tracker-domain blocker; **not** a complete ad blocker.
- Local bookmark storage with a 500-bookmark preview cap.
- Session-only cookies by default; opt-in persistence; cookie/HTTP-cache
  clearing; and a separately confirmed **full website-data reset on restart**.
  An OS-level Linux lock prevents concurrent processes using the same profile.
  The cookie/HTTP-cache action alone does not clear HTML5 site storage.
- Explicit user consent for website capability requests.
- Light/dark native Qt theme adapted from the approved **GoreeCloud UI** Lagoon palette.

Custom DNS, sync, extensions, private windows, full adblock lists, packaged installers
and auto-updates are **not implemented**. DNS currently uses the operating system.

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
through both offscreen and virtual X11 (Xvfb) backends.
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

See [architecture and limitations](docs/ARCHITECTURE.md) and
[third-party notes](docs/THIRD_PARTY.md).

**GoreeShield** governs privacy choices; **Wardveil** governs cybersecurity;
GoreeCloud Mesh integration must respect both. The UI follows **GoreeCloud UI**.
No telemetry, remote homepage or cloud account is configured by this code.

## License

Original source: [Apache License 2.0](LICENSE). Qt/Qt WebEngine and Chromium components
have separate obligations; see [NOTICE](NOTICE).

Copyright 2026 GoreeCloud.

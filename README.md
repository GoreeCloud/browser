# GoreeCloud Browser

A **native Qt desktop-browser development preview** for GoreeCloud, implemented with Python,
PySide6 Qt Widgets and Qt WebEngine (Chromium rendering). Original first-party application
code, not a browser-engine fork.

**Status: v0.1.0 development preview. Not a production or daily-driver release.**

## Implemented in source

- Native Qt desktop navigation, tabs, back/forward/reload, downloads and shortcuts.
- **Distinct, entirely native Home and New Tab pages**; neither loads remote content.
- Startpage search for text queries. Bare hosts use HTTPS by default. GoreeCloud Search
  is **not** enabled until it passes an independently verified stability gate.
- URL cleaning for common tracking parameters, with signed URLs exempted.
- Small third-party tracker-domain blocker; **not** a complete ad blocker.
- Local bookmark storage with a 500-bookmark preview cap.
- Session-only cookies by default, opt-in persistent cookies and cookie/HTTP-cache
  clearing. Local site storage is **not** completely cleared by that action.
- Explicit user consent for website capability requests.
- Light/dark native Qt theme adapted from the approved **GoreeCloud UI** Lagoon palette.

Custom DNS, sync, extensions, private windows, full adblock lists, packaged installers
and auto-updates are **not implemented**. DNS currently uses the operating system.

## Run from source

Requires Python **3.10–3.14**, graphical desktop and a supported Qt WebEngine runtime.
Linux, Windows and macOS runtime compatibility require independent testing.

~~~sh
git clone https://github.com/GoreeCloud/browser.git
cd browser
python3 -m venv .venv
source .venv/bin/activate        # Windows: .venv\Scripts\activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python -m goreecloud_browser
~~~

Do not disable WebEngine sandboxing for routine use.

## Tests

~~~sh
python3 -m unittest discover -s tests -v
python3 -m compileall -q goreecloud_browser tests
~~~

CI checks pure helpers, compilation and Qt imports, but cannot establish visual,
cross-OS, web-compatibility or security acceptance.

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

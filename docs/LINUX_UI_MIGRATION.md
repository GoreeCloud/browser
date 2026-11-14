# Mandatory Linux primary UI migration — GoreeCloud Browser

**Current compliance: MIGRATION REQUIRED.** The founder's [mandatory Linux UI framework standard](https://drive.google.com/file/d/1L-93POcFZE12F-vYxbSc9XquQ-KWOa6f/view) applies to existing applications with **no grandfathering**. Only Qt Quick on Qt **6.12.0**, Flutter Linux Desktop or Slint may be the primary UI.

## Verified source baseline

- `goreecloud_browser/app.py` directly imports `PySide6.QtWidgets`, creates widgets as the main window/chrome and renders pages through `PySide6.QtWebEngineWidgets.QWebEngineView`.
- `pyproject.toml` currently pins **PySide6==6.11.3**.
- This is a functional **legacy Qt Widgets development preview**, **not** compliant with the newer Linux primary UI requirement. Its existing Qt/Xvfb windowing and security tests do not establish framework compliance.
- Governing Linux requirement does **not** change Windows/macOS independently or claim that Qt Quick 6.12.0 is installed on the current laptop.

## Selected migration candidate (not yet implemented)

**Qt Quick on Qt 6.12.0** is the preferred candidate: it is closest to the existing Qt/Python backend. Check upstream **Qt Quick/WebEngine integration**, appropriate `QQmlApplicationEngine` ownership, Python↔QML interaction and platform-packaging APIs in the **pinned 6.12.0** release before adopting code. A QML theme overlay on an active `QMainWindow` is **not** a migration. Keep the actual top-level chrome, tabs, Settings and user workflows in approved Qt Quick controls. Backend components can stay in Python where safe.

Migration workstreams, in dependency order:

1. **Inventory and threat model.** Document window/UI ownership, Qt WebEngine profile lifetime, cookie/cache partitioning, Webspace privacy, URL/origin rules, navigation/permission/download interceptors, TLS/sandbox controls, settings and keyboard actions. Review licenses and the actual Qt 6.12.0 runtime before build.
2. **Separate backend services from widgets.** Extract first-party session/profile, filtering, storage, navigation and validation logic behind narrow interfaces with no GUI element reach-through. Preserve the fail-closed sandbox and certificate checks.
3. **Implement genuine Qt Quick UI.** Build a declarative app window, toolbar/omnibox, tab controls, permission/privacy controls, Downloads/Webspaces and accessible Settings. Wire web content using verified supported Qt Quick WebEngine APIs—not a new untrusted web-to-native bridge. Reuse canonical [GoreeCloud UI](https://github.com/GoreeCloud/gc-ui) design tokens; preserve no-telemetry defaults.
4. **Windowing regression parity.** Preserve normal/windowed startup, minimum size and multi-monitor work-area clamping, optional local-only normal size memory and explicit F11 maximize/full-screen restoration. Existing `goreecloud_browser/windowing.py` is a pure policy helper but must be integrated and retested with the actual Qt Quick window.
5. **Data, security, and accessibility parity.** Verify temporary profiles, persistent/ephemeral permissions, no credential leakage, downloads, local file boundaries, screen-reader semantics, keyboard reachability, 100–400% scaling, offline/error recovery and OS integration.
6. **Packaging and cutover.** Build actual Linux packages with reviewed Qt 6.12.0 dependencies, test clean install/update/rollback and existing-user migration in disposable profiles, then independently validate GNOME/KDE Wayland/X11 and representative real devices. Retire the Widget UI only after approved migration gates pass.

**No production switch, installer, Qt 6.12.0 install, QML runtime, accessibility acceptance or compliant release is claimed.** Maintain relevant Wardveil cybersecurity and GoreeShield privacy fixes during transition. Update `docs/linux-ui-framework-status.json` and its regression test only after verified new source, package and runtime evidence. Do not label this Browser compliant based on a roadmap, compatibility import or windowing test.

# Architecture and current limitations

**Status:** Development preview, 2026-11-06; source features do not prove an
installed, hardened or distributed app.

## Architectural decision

Qt Widgets supplies native desktop chrome and platform accessibility APIs;
Qt WebEngine supplies a maintained Chromium-based web renderer. A Tauri shell
would add another webview but not by itself deliver browser-specific navigation,
profiles or interceptors. Qt is selected here as the direct desktop toolchain.

## Trust boundaries

- **Native UI:** Qt controls tabs, Home, New Tab, menus, preferences, bookmarks
  and file-save decisions. Native pages do not load remote content.
- **Untrusted sites:** Rendered in Qt WebEngine; no privileged Python/web bridge.
- **Network:** Qt WebEngine handles TLS and DNS. A limited request interceptor
  blocks a few known third-party trackers and cleans safe GET navigations.
  It does not implement full DNS control, MITM, VPN or certificate override.
- **Persistence:** QSettings stores preferences and bookmarks locally. Qt
  WebEngine may retain HTML5 site storage and visited-link data. Session-only
  cookies are the default, but the clear action does not wipe all site storage.
- **Downloads:** Require a native save prompt. Saved files remain untrusted.

## Safeguards

- Typed addresses reject script, file, custom schemes and userinfo credentials.
- HTTPS is chosen for a bare hostname; explicit HTTP addresses remain usable.
- Signed URLs and non-GET requests bypass tracking-parameter rewriting.
- TLS/certificate errors are not overridden.
- Websites must request capability permission explicitly.
- No telemetry, external code bridge or automated remote account login.

## GoreeCloud UI

Native theme values mirror official GC UI semantic roles, Lagoon accent,
light/dark values, radius and spacing inspected from the canonical
[GoreeCloud/gc-ui](https://github.com/GoreeCloud/gc-ui) styles/tokens.css
on November 6, 2026. The Qt adapter is **not** a second design authority.
Switch to an approved native token adapter once available.

## Acceptance and roadmap

1. Launch and test on Linux, Windows and macOS, including sandboxing.
2. Test redirects, POST/signed URLs, permissions, cookies, downloads and safe
   URL display; maintain threat-model and dependency-vulnerability reviews.
3. Perform screenshot-led whole-screen visual audit, keyboard, high contrast,
   zoom, screen-reader and light/dark acceptance on real devices.
4. Develop robust filter subscriptions, site-data manager, private profiles,
   safe download tracking, bookmark export/import and history controls.
5. Implement platform packaging, signing, update policy, rollback and
   browser-engine vulnerability patch workflow.
6. Enable GoreeCloud Search only after independently verified stability.

A source commit or passing CI check is **not** a stable browser release.

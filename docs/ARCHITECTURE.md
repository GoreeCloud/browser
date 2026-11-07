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
- Websites must request capability permission explicitly; accepted/denied
  permissions are held only in memory for the running session.
- No telemetry, external code bridge or automated remote account login.

## GoreeCloud UI

Native theme values mirror official GC UI semantic roles, Lagoon accent,
light/dark values, radius and spacing inspected from the canonical
[GoreeCloud/gc-ui](https://github.com/GoreeCloud/gc-ui) styles/tokens.css
on November 6, 2026. The Qt adapter is **not** a second design authority.
Switch to an approved native token adapter once available.

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

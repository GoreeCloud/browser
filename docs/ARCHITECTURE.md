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
  blocks a few known third-party trackers. **Tracking-parameter cleaning only
  applies before navigating an entered or pasted address**, when enabled.
  Clicked links and HTTP redirects are **not** automatically cleaned: a local
  integration test showed that a redirect can deliver its tracking parameter
  to the server even if a request interceptor changes the displayed URL.
  That misleading interceptor rewrite was removed. Full DNS control, MITM, VPN
  and certificate overrides are not implemented.
- **Persistence:** QSettings stores preferences and bookmarks locally. Qt
  WebEngine can retain HTML5 site storage and visited-link data. Session-only
  cookies are the default. The cookie/cache-clear action is narrower than the
  separately confirmed full-site reset on the next launch.
- **Linux single-instance safety:** An advisory OS lock prevents two browser
  processes sharing the same writable WebEngine profile. An explicit reset
  marker is processed only before creating any pages, while holding that lock;
  only the app-owned web-profile and cache directories are eligible. Bookmarks
  and settings are deliberately preserved, and symlinked targets are rejected.
- **Downloads:** Require a native save prompt; Qt WebEngine writes only into
  per-download private temporary directories. On successful completion the
  browser publishes the file to the chosen destination with atomic no-clobber
  link operations; cross-device destinations use a private fully copied sibling
  file and atomic link. Existing files and symlink destinations are rejected.
  A cancelled/interrupted download is discarded and never auto-opened. Partial
  temporary downloads may remain after a process crash until OS temp cleanup.
  This does **not** provide antimalware scanning, authenticity verification,
  quarantine enforcement, or a mature download manager. Saved files remain
  untrusted and must not be executed blindly.

## Safeguards

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
  permissions are held only in memory for the running session.
- No telemetry, external code bridge or automated remote account login.

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

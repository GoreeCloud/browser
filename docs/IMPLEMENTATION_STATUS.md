# GoreeCloud Browser — Implementation Status

GoreeCloud Browser remains in active development. This document distinguishes implemented development scaffolding from planned production capability.

## Implemented development core

The repository currently contains GoreeCloud-owned contracts and initial runtime scaffolding for:

- Browser engine abstraction (`BrowserEngine`, `EngineContext`, `EngineView`).
- Browser application lifecycle (`BrowserApplication`).
- Browser window and tab lifecycle (`WindowController`, `Tab`).
- Development-only navigation history and view state (`DevelopmentEngine`).
- Engine-independent sessions, containers, bookmarks, downloads, proxy, memory, permissions, capture, Reader Mode, Identity, recovery, platform integration, security state, Glaze UI acceptance, networking policy, and service integration contracts.
- First-party internal-page routing for GoreeCloud New Tab, Home, and Settings.
- Unified search/navigation resolution that separates direct URL navigation from GoreeCloud Search and prohibits alternate-provider fallback.
- Explicit Local Bookmarks versus GoreeCloud Bookmarks destination selection.
- Explicit Local Vault versus GoreeCloud Vault destination selection.
- Latest-approved-Stable Glaze UI build baseline enforcement.
- Canonical blue-compass Browser identity synchronized from `GoreeCloud/branding-assets` with guarded consumer derivatives.
- Linux GTK/X11 Glaze Development chrome with real-device-reviewed first-party New Tab/Home/Settings surfaces, structured Development panels, multi-tab controls, and keyboard-first navigation.
- Flatpak packaging scaffold for Linux.
- WiX installer scaffold for Windows `.exe` distribution.
- Installable Android Development APK target with HTTP/HTTPS browser-role intent declarations and managed-emulator smoke coverage.

## Development engine

`DevelopmentEngine` is intentionally non-rendering. It exists only so the GoreeCloud Browser-owned runtime can be compiled and exercised before Chromium integration.

It must not be represented as:

- a production rendering engine;
- a Chromium replacement;
- evidence of web compatibility;
- evidence of production browser readiness.

## Chromium integration

`ChromiumEngineAdapter` defines the current integration seam. Chromium/Blink implementation types must remain behind that boundary.

The repository contains the Chromium adapter seam and an optional CEF-backed runtime source path. Render-capable desktop qualification still requires evidence for:

- reviewed and reproducibly acquired CEF runtime provenance;
- Chromium runtime initialization and shutdown on supported Linux hosts;
- process/subprocess integration;
- sandbox preservation;
- site isolation preservation;
- GPU integration;
- profile/context creation;
- native web-view creation and GTK/X11 child-surface behavior;
- real HTTPS rendering and navigation callbacks;
- download and permission interception;
- certificate/security-state inspection;
- private-context isolation and cleanup;
- site-data clearing;
- renderer crash/health reporting;
- accessibility integration;
- printing and PDF plumbing where applicable;
- sustained representative-device operation and recovery.

## Distribution scaffolds

### Linux Flatpak

`packaging/flatpak/io.goreecloud.Browser.yml` establishes the initial Flatpak application identity and permission surface. It is not production-ready until Chromium runtime packaging, sandbox behavior, portals, codecs, desktop metadata, canonical Browser branding derivatives, signing, and installation/upgrade validation are complete.

### Windows

`packaging/windows/GoreeCloudBrowser.wxs` establishes the initial WiX installer definition for an eventual signed Windows installer executable. Production readiness additionally requires signed executable/installer artifacts, supported default-browser registration, application associations, upgrade/repair/uninstall validation, Chromium runtime packaging, canonical Browser branding resources, and release-integrity evidence.

### Android APK

`apps/android` establishes a native Android application module using `io.goreecloud.browser`. The manifest declares launcher behavior and HTTP/HTTPS browsing intents. It deliberately does not implement the Browser as a generic WebView shell. Production APK acceptance requires native Browser runtime integration, Glaze UI acceptance, approved rendering-engine integration, canonical compass adaptive/round/monochrome assets, external signing configuration, upgrade validation, and real-device evidence.

Signing secrets must remain outside source control for every platform.

## Next implementation milestones

1. Qualify a reviewed, reproducible CEF runtime on Linux and produce real HTTPS rendering evidence on the GTK/X11 child surface without weakening Chromium sandbox or site isolation.
2. Connect renderer navigation, certificate/security state, permissions, crashes, printing/PDF, and download interception to the existing GoreeCloud-owned Browser contracts.
3. Replace Development-only Bookmarks/Reader/Privacy/Security/network panels with accepted provider-backed behavior while preserving fail-closed authority boundaries.
4. Add Local Bookmarks persistence and the GoreeCloud Bookmarks synchronization adapter.
5. Add Local Vault persistence and GoreeCloud Vault synchronization with explicit destination controls.
6. Wire local profile/session persistence and crash-safe restoration into the desktop runtime while preserving Private/Isolated Private exclusions.
7. Complete Privacy Shield interception boundaries and evidence reporting.
8. Route accepted Chromium download interception into GoreeCloud Advanced Download Manager and Wardveil release-gate handling.
9. Add accepted GoreeCloud Identity, Sync, DNS, Network, Policy, Observability, Manager, Mesh, Everkeep, and related service adapters.
10. Complete production packaging/signing/upgrade/rollback/deployment evidence for Flatpak, Windows, and Android from exact accepted Browser source.

## Production rule

Nothing in this file overrides the product acceptance requirements in `PRODUCT_INVENTORY.md`. A feature is production-ready only when its required implementation and acceptance evidence exist.

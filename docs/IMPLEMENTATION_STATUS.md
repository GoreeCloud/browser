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
- Canonical Browser compass artwork synchronized from `GoreeCloud/branding-assets` with consumer provenance checks.
- Flatpak packaging scaffold for Linux.
- WiX installer scaffold for Windows `.exe` distribution.
- Android Gradle application scaffold with HTTP/HTTPS browser-role intent declarations for future APK builds.

## Development engine

`DevelopmentEngine` is intentionally non-rendering. It exists only so the GoreeCloud Browser-owned runtime can be compiled and exercised before Chromium integration.

It must not be represented as:

- a production rendering engine;
- a Chromium replacement;
- evidence of web compatibility;
- evidence of production browser readiness.

## Chromium integration

`ChromiumEngineAdapter` defines the current integration seam. Chromium/Blink implementation types must remain behind that boundary.

The first adapter milestone requires:

- Chromium runtime initialization and shutdown;
- process/subprocess integration;
- sandbox preservation;
- site isolation preservation;
- GPU integration;
- profile/context creation;
- native web-view creation;
- navigation callbacks;
- download interception;
- permission interception;
- certificate/security-state inspection;
- private-context support;
- site-data clearing;
- renderer crash/health reporting;
- accessibility integration;
- printing and PDF plumbing where applicable.

## Distribution scaffolds

### Linux Flatpak

`packaging/flatpak/io.goreecloud.Browser.yml` establishes the initial Flatpak application identity and permission surface. It is not production-ready until Chromium runtime packaging, sandbox behavior, portals, codecs, desktop metadata, canonical compass derivatives, signing, and installation/upgrade validation are complete.

### Windows

`packaging/windows/GoreeCloudBrowser.wxs` establishes the initial WiX installer definition for an eventual signed Windows installer executable. Production readiness additionally requires signed executable/installer artifacts, supported default-browser registration, application associations, upgrade/repair/uninstall validation, Chromium runtime packaging, canonical compass resources, and release-integrity evidence.

### Android APK

`apps/android` establishes a native Android application module using `io.goreecloud.browser`. The manifest declares launcher behavior and HTTP/HTTPS browsing intents. It deliberately does not implement the Browser as a generic WebView shell. Production APK acceptance requires native Browser runtime integration, Glaze UI, Chromium/approved engine integration, canonical compass adaptive/round/monochrome assets, external signing configuration, upgrade validation, and real-device evidence.

Signing secrets must remain outside source control for every platform.

## Next implementation milestones

1. Complete the Linux CEF/Chromium render-capable runtime path and validate real HTTPS rendering on representative desktop hardware without weakening sandboxing or site isolation.
2. Turn the current Development Bookmarks/Library surface into persisted Local Bookmarks plus an accepted GoreeCloud Bookmarks synchronization adapter with explicit authority separation.
3. Turn the structured Settings surface into functional Browser-owned controls backed by authoritative provider/runtime state.
4. Add Local Vault persistence and GoreeCloud Vault synchronization adapter with explicit destination controls.
5. Complete local profile/session persistence and crash-safe restoration.
6. Implement Privacy Shield interception boundaries and evidence reporting.
7. Route Chromium download interception into GoreeCloud Advanced Download Manager.
8. Add GoreeCloud Identity, Sync, DNS, Network, Policy, Observability, and remaining service adapters.
9. Complete history/library/Reader Mode production surfaces and keyboard/accessibility behavior.
10. Make Flatpak, Windows installer, and Android APK pipelines consume exact accepted Browser source and signed release metadata.

## Production rule

Nothing in this file overrides the product acceptance requirements in `PRODUCT_INVENTORY.md`. A feature is production-ready only when its required implementation and acceptance evidence exist.

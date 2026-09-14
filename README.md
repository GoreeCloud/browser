# GoreeCloud Browser

GoreeCloud Browser is GoreeCloud's first-party native web browser. GoreeCloud owns the browser product, user experience, browser-state model, privacy controls, security integrations, synchronization behavior, session model, and first-party service boundaries. A mature web engine is used underneath as replaceable implementation infrastructure rather than as the identity of the product.

## Lifecycle status

**Beta development — not Stable or production approved.**

The repository contains a build-tested GoreeCloud-owned browser core, native desktop work, and an installable Android beta target. Beta source/build success does not imply production signing, complete runtime integration, design-system acceptance, security acceptance, privacy acceptance, accessibility acceptance, or Stable qualification.

## Product identity

GoreeCloud Browser is the normal user-facing product identity. Upstream engine or third-party implementation technology may appear where technically, legally, operationally, diagnostically, or compliance-wise required, but it does not become GoreeCloud product branding.

## Architectural direction

GoreeCloud Browser is not intended to be a conventional Chromium skin or a permanently deep Chromium fork.

GoreeCloud-owned application layers remain separated from the rendering engine behind explicit interfaces. Chromium/Blink and Android System WebView/Chromium are initial engine foundations. The architecture must preserve the ability to use platform-native engines, alternative engines, or future GoreeCloud-controlled engine work when a concrete privacy, security, performance, compatibility, platform, or product requirement justifies it.

## Core systems and services

- **GoreeCloud Browser** — browser shell, chrome, tabs, profiles, preferences, history, permissions, downloads, sessions, and navigation experience.
- **Browser Engine Layer** — GoreeCloud-owned abstraction between Browser and the active web engine.
- **GoreeCloud Manager** — bounded operational visibility and administration where accepted Browser integration exists.
- **Glaze UI** — authoritative GoreeCloud presentation and interaction system for Browser-owned surfaces.
- **GoreeCloud Search** — sole Internet/web/current-information search authority for approved Browser search entry points.
- **GoreeCloud Index** — universal/local federated discovery authority when Browser invokes device/application/content search.
- **GoreeCloud Bookmarks** — bookmarks, read-later, collections, and library subsystem.
- **GoreeCloud Sync** — synchronization application/service capability for approved Browser state while excluding indiscriminate sensitive website-state sync.
- **Privacy Shield** — privacy authorization and protection authority for Browser privacy operations.
- **Wardveil Security** — security/protection authority and evidence-backed Browser security integration.
- **Everkeep** — encrypted browser-state backup, recovery, portability, and continuity where applicable.
- **GoreeCloud Identity** — profiles, device authorization, synchronization identity, and account-bound preferences.
- **GoreeCloud Vault** — credentials, passkeys, secrets, secure autofill, and protected authentication material.
- **GoreeCloud DNS** — DNS resolution, resolver policy, DNS privacy, and Browser DNS integration.
- **GoreeCloud Network** — networking and private-connectivity authority.
- **GoreeCloud Mesh** — first-party coordination and governance layer.
- **GoreeCloud Wayfinder** — Browser-owned navigation, productivity, organization, capture, transfer, session, and workflow feature family.
- **Media Hover** — Browser-owned contextual media interaction layer subject to Browser privacy/security and destination boundaries.

The seven Integral Platform Systems are GoreeCloud Manager, Privacy Shield, Wardveil Security, Everkeep, Glaze UI, GoreeCloud Mesh, and GoreeCloud Identity. Search, Index, Sync, Vault, DNS, Network, Bookmarks, and other first-party products/services keep their own authority and acceptance contracts; they are not reclassified as Integral Platform Systems by Browser integration.

## Search and Index integration

Browser must preserve a clear distinction among navigation, Internet search, and universal/local discovery.

1. Valid navigable URL or explicit navigation intent → Browser navigation path.
2. Non-URL Internet/current-information query → GoreeCloud Search delegation.
3. Universal/local discovery intent → GoreeCloud Index invocation.
4. Ambiguous or unsafe input → no silent execution until classification and applicable policy checks succeed.

Browser must not silently fall back to a third-party search engine when GoreeCloud Search is unavailable.

Remote Search delegation remains subject to applicable Privacy Shield authorization. Browser must not attach unrelated tab state, browsing history, downloads, bookmarks, local Index results, cookies, or identity information to a Search request unless a separately specified and authorized feature requires it.

See [`docs/SEARCH_INDEX_INTEGRATION.md`](docs/SEARCH_INDEX_INTEGRATION.md).

## First-party capability acceptance boundary

Browser uses fail-closed first-party capability consumption.

A production capability is usable only when the expected record is unambiguous, versioned, current, authoritative, available, explicitly production accepted, and compatible with the requested contract. Missing, duplicate, stale, non-authoritative, degraded where healthy operation is required, version-incompatible, or pre-production evidence must not be transformed into a healthy production claim.

Development builds may use explicitly Development-only capability paths where repository-local evidence makes that exception visible. Development exceptions never authorize Stable promotion.

Consumer validation does not create producer authority. Search, Index, Vault, Sync, Identity, Mesh, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, Bookmarks, and other first-party systems and services retain independent contracts and lifecycle evidence.

## Glaze UI requirement

The current official Stable consumer target published by `GoreeCloud/goreecloud-glaze-ui` is **Glaze UI V1.4 / `1.4.0`**.

Previous Browser documentation named Glaze UI `2.2.0`. That value is not the current Stable authority and must not be used as present Browser conformance or release evidence.

Browser is therefore **V1.4 migration-required** until Browser-owned surfaces and native mappings have been updated and validated against the current Stable contract.

Glaze V1.4 requires bounded optical behavior, solid readable/critical-decision surfaces where appropriate, accessible solid fallbacks for Reduced Transparency and Forced Colors, strengthened clarity under Increased Contrast, and local deterministic optical behavior that does not require telemetry, analytics, camera access, or remote environmental sensing.

Central Glaze Stable status does not automatically certify Browser. Browser-local source, rendered/native, accessibility, runtime, and representative-device evidence remain required.

See [`docs/GLAZE_UI_ADOPTION.md`](docs/GLAZE_UI_ADOPTION.md).

## Privacy and security boundaries

Browser must continue to prefer explicit authority and fail-closed behavior.

- Privacy Shield controls applicable privacy authorization, purpose, minimization, destination, and retention decisions.
- Wardveil Security controls applicable protection/security state and evidence.
- Identity authentication is not blanket authorization for every Browser operation.
- Manager administration or operational visibility does not transfer Browser product authority.
- Vault owns protected credential/passkey material.
- DNS and Network retain their own configuration and runtime authority.
- Engine presence does not authorize bypassing GoreeCloud policy boundaries.

No single global “protected” state should be used to imply that every Browser subsystem or external service has independently satisfied its own acceptance requirements.

## Wardveil download security

The Advanced Download Manager retains a fail-closed Wardveil release model.

Completed transfer bytes remain in Browser-owned staging until an authoritative, current, correctly scoped clean finding is bound to the exact payload digest. Suspicious downloads are held for review. Malicious or unverifiable downloads remain blocked from ordinary release/open paths.

Browser must not claim canonical Wardveil quarantine authority merely because it can request or display a quarantine handoff. Production acceptance still requires authenticated Browser↔Wardveil transport, scanner/signature health, controlled runtime malware tests, concurrency-safe staging release, and accepted Privacy Shield and Glaze behavior.

Android download behavior must not create a bypass around this contract.

See [`docs/WARDVEIL_DOWNLOAD_SCANNING.md`](docs/WARDVEIL_DOWNLOAD_SCANNING.md).

## Design principles

1. GoreeCloud owns the browser product layer.
2. The rendering engine is replaceable infrastructure, not product identity.
3. GoreeCloud-specific behavior should remain outside deep engine forks when stable adapters are practical.
4. The seven Integral Platform Systems—GoreeCloud Manager, Privacy Shield, Wardveil Security, Everkeep, Glaze UI, GoreeCloud Mesh, and GoreeCloud Identity—retain substantive independent authority and evidence requirements; Search, Index, Sync, and other first-party products/services remain separate capabilities with their own contracts.
5. Browser state should use GoreeCloud-owned schemas rather than treating Chromium profile formats as canonical.
6. Upstream engine security updates must remain practical to consume.
7. Engine-specific exceptions must be isolated and documented.
8. Planned capabilities must not be presented as production-ready until implementation and acceptance evidence exists.
9. Security, privacy, credential, identity, DNS, networking, search, indexing, and synchronization authority boundaries remain explicit.
10. Browser-owned user-facing surfaces must conform to the latest approved Stable Glaze UI release before production acceptance.

## Development phases

### 0.x — GoreeCloud-owned browser shell

Build and harden the native application, current-Stable Glaze UI browser chrome, tabs, windows, sessions, profiles, settings, permissions UI, history, bookmarks integration, Search integration, Index invocation, downloads, Browser-owned state storage, Media Hover, and engine adapters.

### 0.x — First-party services

Integrate Sync, Identity, Vault, Privacy Shield, Wardveil Security, Everkeep, DNS, Network, Wayfinder, Mesh, Manager, Search, Index, and capability-gated Media Hover destinations/processors through explicit service contracts and authority boundaries.

### Later — selective engine ownership

Replace, patch, or take greater control of engine subsystems only where GoreeCloud has a concrete requirement that justifies the maintenance cost and risk.

## Repository layout

```text
apps/browser/                 Native desktop application shell
apps/android/                 Installable Android beta application target
browser/core/                 Engine-independent browser domain model
browser/engine/               Browser Engine Layer interfaces
browser/engine/chromium/      Chromium adapter implementation
browser/services/             GoreeCloud service contracts and adapters
browser/storage/              GoreeCloud-owned browser data stores
browser/ui/                   Browser-owned Glaze UI components
include/goreecloud/browser/   Public C++ interfaces
src/                          Implementation and platform hosts
docs/                         Architecture and engineering documentation
packaging/                    Linux, Flatpak, Windows, and other packaging work
```

## Documentation

- [`SPECIFICATIONS.md`](SPECIFICATIONS.md) — product specification.
- [`FEATURES.md`](FEATURES.md) — functionality and implementation state.
- [`BENEFITS.md`](BENEFITS.md) — supportable product value.
- [`COMPETITIVE-OBJECTIVES.md`](COMPETITIVE-OBJECTIVES.md) — benchmark and differentiation objectives.
- [`USER-MANUAL.md`](USER-MANUAL.md) — current user-facing behavior and beta guidance.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — architectural boundaries and ownership model.
- [`docs/ENGINE_LAYER.md`](docs/ENGINE_LAYER.md) — Browser Engine Layer contracts.
- [`docs/GLAZE_UI_ADOPTION.md`](docs/GLAZE_UI_ADOPTION.md) — current-Stable Glaze adoption policy.
- [`docs/SEARCH_INDEX_INTEGRATION.md`](docs/SEARCH_INDEX_INTEGRATION.md) — Browser/Search/Index delegation and authority contract.
- [`docs/MEDIA_HOVER.md`](docs/MEDIA_HOVER.md) — Media Hover contract.
- [`docs/WARDVEIL_DOWNLOAD_SCANNING.md`](docs/WARDVEIL_DOWNLOAD_SCANNING.md) — download security contract.
- [`docs/PRODUCT_INVENTORY.md`](docs/PRODUCT_INVENTORY.md) — planned inventory and authority boundaries.

## Stable acceptance

Browser remains non-Stable until the supported release revision has current evidence for:

- web-engine/runtime behavior;
- security and privacy enforcement;
- current Glaze UI adoption;
- accessibility;
- packaging and production signing;
- recovery and continuity;
- compatibility and migration;
- Search and Index integration boundaries;
- representative real-device and sustained-use validation.

Passing source or CI checks proves only the revision and scope exercised by those checks. It does not by itself authorize Stable promotion.

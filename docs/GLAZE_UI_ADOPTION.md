---
title: "GoreeCloud Browser — Glaze UI Adoption"
document_type: "Design System Adoption Record"
status: "Development"
version: "v1.1"
last_updated: "2026-09-25"
---

# GoreeCloud Browser — Glaze UI Adoption

GoreeCloud Browser tracks the current consumer-eligible GLAZE UI release while preserving Browser-local acceptance gates.

## Current baseline

- Glaze UI version: `1.6.0`
- Canonical lifecycle: **Anchor**
- Compatibility release-channel terminology: Stable
- Canonical repository: `GoreeCloud/glaze-ui`
- Accepted release source: `a7180679ea851389e0f3004515f9a25f420e716d`
- Qualification source anchor: `c7509c79256b04b0aa67cb9dd0737d7588e0ae4a`
- Qualification evidence integration: `354f5759385c28596fcfec26a3ad525e89fb1c35`
- Published artifact SHA-256: `687268b5eb76917eccae9d935ffa1bead333d5dee50b6098e996a3f44cee50af`
- Immediate shared rollback baseline: `1.5.1`
- Browser policy: `latest-approved-stable`
- Browser conformance state: **V1.6 source adoption in progress; Browser-local acceptance incomplete**
- Browser production eligibility from source adoption alone: **No**

The central GLAZE UI V1.6 release is consumer-eligible, but central qualification never substitutes for Browser-specific rendered, native, accessibility, performance, device, workflow, rollback, release, or production evidence.

## V1.6 Browser presentation requirements

Browser-owned presentation must preserve the V1.6 material, component-state, accessibility, responsive, localization, provenance, performance, and authority boundaries applicable to each supported platform.

Browser therefore requires:

- solid or raised surfaces for durable reading and consequential decisions;
- bounded functional glass for navigation, search, compact controls, and transient chrome;
- safe solid fallbacks when transparency is reduced, unsupported, or too expensive;
- at least 48 px/dp general interaction targets and applicable larger assisted-input targets;
- default, hover, focus, pressed, selected, disabled, loading, and error states where relevant;
- visible keyboard focus and equivalent non-hover interaction paths;
- non-color indicators for material semantic states;
- large-text reflow without shrinking interaction targets below supported minimums;
- responsive task continuity instead of desktop-layout compression;
- localization and RTL-safe composition without English-length assumptions;
- scalable, optically coherent iconography with labels where meaning is unclear;
- truthful status and provenance presentation for privacy, security, connectivity, capability, and source state;
- optional-effect reduction under accessibility or runtime pressure without changing semantic meaning or capability truth.

## Authority boundaries

Glaze UI is presentation-only. Browser must not infer or manufacture authorization, permission, consent, security state, privacy state, identity state, provider precedence, successful persistence, service availability, or consequential execution from visual context.

Privacy Shield remains authoritative for privacy truth. Wardveil Security remains authoritative for security truth. Identity, Policy, Mesh, Everkeep, Search, Sync, Vault, DNS, Network, and other owning systems retain their respective authority.

Missing or conflicting capability evidence fails closed. Unknown or unverified state must not be upgraded into a positive state merely because a positive visual treatment is available.

## Browser-owned scope

The adoption requirement covers Browser-owned user-facing surfaces, including application/window chrome, the omnibox, tabs, New Tab and Home, Settings, Downloads, Library/Bookmarks/History, Private Browsing, Reader Mode, media actions, menus/popovers, Wayfinder, proxy/DNS/network tools, Search/Index invocation, extensions, and first-party integration presentation.

Engine-critical, certificate, operating-system, and platform permission surfaces may retain native or engine-required presentation when replacement would reduce security, accessibility, or compatibility. That exception does not permit ordinary Browser-owned UI to remain on a superseded design mapping.

## Current source-adoption tranche

The current V1.6 source tranche updates:

1. Browser C++ Glaze version and exact source/qualification pins.
2. Android-native Glaze mapping and disabled-state presentation.
3. Linux GTK chrome composition, symbolic-icon fallbacks, focus/pressed states, compact secondary-tool overflow, structured Browser-owned surfaces, and bounded material hierarchy.
4. Build-time and smoke-test checks that fail if the V1.6 exact source mapping drifts.
5. Repository documentation and Platform Contract evidence references.

This tranche is substantive source adoption, not a metadata-only relabeling. It still does **not** establish Browser-wide V1.6 conformance.

## Acceptance still required

Before Browser can claim current Glaze UI conformance, exact-revision evidence must cover the applicable Browser surfaces and platforms, including:

- rendered Linux GTK review;
- Android emulator and representative physical-device review;
- keyboard navigation and visible focus;
- TalkBack/desktop assistive-technology behavior where applicable;
- 200% text or supported large-text equivalent;
- Reduced Motion;
- Reduced Transparency / effects-free fallback;
- Increased Contrast and Forced Colors or the closest supported native equivalent;
- localization and RTL;
- narrow/resized desktop windows, phone portrait, tablet, orientation and applicable posture changes;
- touch/pointer target behavior;
- loading, error, unavailable, restricted, offline, and degraded states;
- representative performance and sustained-use behavior;
- upgrade and rollback from the immediate `1.5.1` baseline.

## Production rule

Browser remains Development and non-production-approved while these acceptance and broader Browser release gates remain incomplete. Passing source checks or central Glaze qualification is not sufficient for production or Anchor qualification.

## Revision history

| Version | Date | Status | Change |
|---|---|---|---|
| v1.1 | 2026-09-25 | Development | Migrated the Browser source target to GLAZE UI V1.6 / 1.6.0, recorded exact accepted source and qualification anchors, added V1.6 presentation/authority requirements, and documented the current Android/Linux source-adoption tranche plus remaining Browser-local acceptance. |
| v1.0 | 2026-09-16 | Development | Established the previous V1.5.1 adoption record and acceptance boundary. |

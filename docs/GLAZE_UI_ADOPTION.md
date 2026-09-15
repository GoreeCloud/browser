# GoreeCloud Browser — Glaze UI Adoption

GoreeCloud Browser tracks the **latest approved Stable Glaze UI release**.

## Current baseline

- Glaze UI Stable version: `1.4.1`
- Canonical repository: `GoreeCloud/goreecloud-glaze-ui`
- Current signed Stable authority revision: `4fab9da0fad2e5c974e0e66ec88632c61745751c`
- Immediate shared rollback baseline: `1.4.0`
- Browser policy: `latest-approved-stable`
- Browser conformance state: **migration required / not yet accepted**
- Browser production eligibility from this documentation update alone: **No**

This is a moving adoption target, not a permanent pin. Any newer Stable Glaze UI promotion makes Browser migration-required until Browser-owned surfaces, native mappings, automated checks, and application-specific acceptance evidence are updated.

Older Browser documentation that named Glaze UI `2.2.0`, V1.3, or V1.4.0 as the current target is not current release authority. Historical Browser mappings remain useful implementation and rollback history only; they do not establish current conformance.

## Glaze UI V1.4.1 design requirements

Browser must preserve the V1.4 Optical Intelligence model and current V1.4.1 hardening rules on every Browser-owned surface:

- neutral glass remains the material foundation;
- optical adaptation is contextual, bounded, and subordinate to meaning, accessibility, privacy, security, and task completion;
- readable and critical-decision content remains solid where appropriate;
- glazed material is reserved for bounded interaction, navigation, search, command, control, and feedback chrome;
- Content-Aware Frost and other optical behavior must not reduce text or control readability;
- Semantic Blur Protection must preserve information hierarchy and actionable state;
- environmental tinting, warmth, chromatic depth, and color-memory behavior remain bounded and decorative;
- Forced Colors and Reduced Transparency collapse optical behavior to an accessible solid mode;
- Increased Contrast suppresses decorative optical behavior where required and strengthens clarity;
- optical/context behavior must not require telemetry, analytics, camera access, microphone access, or remote environmental sensing;
- shared Glaze qualification must never be treated as Browser-local rendered, accessibility, device, performance, or workflow acceptance.

Browser must not claim that Glaze optical behavior itself provides Privacy Shield authorization, Wardveil security state, Everkeep continuity, Mesh coordination, Identity authorization, Search authority, or successful Browser workflow state.

## Browser-owned scope

The requirement covers Browser-owned user-facing surfaces, including:

- application shell and window chrome;
- omnibox and search/navigation suggestions;
- tabs, tab groups, windows, and session surfaces;
- New Tab and Home;
- settings and permissions UI;
- Downloads and Advanced Download Manager;
- Library, Bookmarks, History, and read-later surfaces;
- Private Browsing landing and state presentation;
- Reader Mode and Browser-owned print/PDF controls;
- Media Hover and destination-selection surfaces;
- context menus and Browser-owned sheets/popovers;
- Wayfinder features;
- Proxy, DNS, Network, privacy, and security settings owned by Browser;
- GoreeCloud Search and GoreeCloud Index invocation surfaces;
- Browser-owned Wardveil, Privacy Shield, Everkeep, Identity, Mesh, Sync, Vault, DNS, and Network integration presentation.

OS-owned, certificate, permission, engine-critical, and Developer Tools surfaces may preserve required native or engine presentation where replacement would reduce security, accessibility, or compatibility. That exception cannot be used to leave ordinary GoreeCloud-owned UI on a superseded design baseline.

## Migration requirements

The V1.4.1 migration must include more than documentation replacement. Browser must audit and update, where applicable:

1. shared tokens and material definitions;
2. Browser-native Android mappings;
3. Linux/desktop Glaze presentation mappings;
4. web/native hybrid surfaces owned by Browser;
5. optical-state fallback behavior;
6. Reduced Transparency, Increased Contrast, Forced Colors or closest platform-equivalent behavior;
7. Reduced Motion behavior;
8. focus, hover, pressed, selected, disabled, loading, warning, and error states;
9. touch, keyboard, mouse, stylus, remote/far-view, and accessibility target sizing where supported;
10. layout behavior across supported form factors, safe areas, and text scaling;
11. localization and RTL behavior;
12. performance on representative hardware;
13. Browser-local regression review for the V1.4.1 hardening delta and immediate V1.4.0 rollback path.

No legacy or previously mapped Glaze version may be relabeled as V1.4.1 merely because its appearance is similar.

## Optical privacy and security boundary

Glaze V1.4.1 permits the bounded local deterministic optical adaptation inherited from V1.4. Browser must source any contextual input through an explicit Browser-owned adapter and must subject that adapter to applicable Privacy Shield and Wardveil review before use.

Browser must not introduce camera, microphone, remote analytics, cross-site tracking, browsing-history profiling, or hidden telemetry merely to drive visual effects.

Environmental memory tint is decorative state only. It must not become identity evidence, security evidence, behavior profiling, or a durable cross-site fingerprint.

## Search and Index surfaces

Browser search and Index integration must follow [`SEARCH_INDEX_INTEGRATION.md`](SEARCH_INDEX_INTEGRATION.md).

The omnibox and other Browser-owned search/navigation surfaces are Browser UI and therefore require Browser-local Glaze V1.4.1 acceptance even though GoreeCloud Search or GoreeCloud Index may own the underlying query authority.

## Acceptance requirements

A Browser revision may claim current Glaze conformance only after repository-local evidence demonstrates the applicable V1.4.1 contract across supported platform families.

At minimum, acceptance should cover:

- exact source/token mapping validation against the signed Stable Glaze authority;
- representative rendered/native visual review;
- keyboard navigation and visible focus;
- TalkBack/screen-reader semantics where supported;
- 200% text or supported large-text equivalent;
- Reduced Motion;
- Reduced Transparency / solid fallback;
- Increased Contrast;
- Forced Colors or closest supported platform equivalent;
- RTL and localization;
- touch and pointer target behavior;
- orientation and form-factor behavior;
- representative performance and sustained-use behavior;
- upgrade and rollback behavior from the V1.4.0 baseline.

Shared Glaze V1.4.1 Stable qualification does not auto-certify Browser. Browser-local acceptance remains separately required.

## Production rule

A superseded or invented Glaze UI version cannot satisfy Browser production readiness. Central Glaze Stable status also does not automatically certify Browser.

Browser remains non-Stable wherever required V1.4.1 migration or Browser-specific acceptance evidence is incomplete.

## Upgrade and rollback process

For every future Stable Glaze promotion, Browser must:

1. record the new semantic version and exact reviewed Glaze revision;
2. audit changed design-system contracts;
3. update Browser-local mappings and metadata;
4. run Browser-specific automated and runtime acceptance;
5. validate representative supported clients;
6. preserve the previous accepted Browser mapping as a rollback reference;
7. remain migration-required until the new current-Stable evidence is accepted.

Rollback history is diagnostic and recovery evidence only. It never makes an older Glaze baseline current.

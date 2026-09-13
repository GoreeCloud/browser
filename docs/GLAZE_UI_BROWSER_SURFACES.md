# GoreeCloud Browser — GLAZE UI V1.4 Surface Model

This document maps **GLAZE UI V1.4 / `1.4.0`** into Browser-owned product surfaces. It is an implementation specification, not a claim that every surface has completed application acceptance. Exact shared Stable source authority: `84cb3db4884042f0fa25ed6d475a127fb110f596`.

## Governing browser rule

**Web content is untrusted content. Browser chrome is trusted application UI. Optical adaptation must never let page appearance impersonate browser, security, privacy, origin, permission, download-safety, or certificate state.**

Environmental Color Memory and page-derived decorative tinting are therefore disabled for trusted Browser chrome. Human/manual/physical-device optical validation remains a V1.4.1 follow-up and is not represented as passed V1.4.0 evidence.

## Browser chrome

The primary Browser frame uses Glaze semantic layout, spacing, color, material, iconography, motion, focus, interaction, and bounded Optical Intelligence contracts while Chromium/Blink remains the web-engine layer.

### Material hierarchy

- **Canvas** — root application/background plane.
- **Surface** — high-legibility content and control regions.
- **Soft Glaze** — low-emphasis contextual treatment where readability remains deterministic.
- **Glaze** — primary bounded interaction chrome such as navigation/search capsules.
- **Deep Glaze** — stronger separation for sheets, panels, menus, and other layered UI.
- **Live Glaze** — bounded active-context surfaces whose motion/translucency safely degrades under accessibility preferences.

Reduced Transparency or readability constraints must collapse translucent treatments to a suitable solid Surface rather than preserving a visual effect at the expense of legibility.

## V1.4 Optical Intelligence

Browser may use V1.4 optical adaptation only from approved **Browser-owned, non-content presentation inputs**. The following are prohibited as implicit trusted-chrome optical inputs:

- webpage pixels or dominant colors;
- CSS/theme colors;
- favicons or favicon colors;
- page images/video frames;
- origin/domain identity;
- query or history content;
- document text;
- certificate/security state; and
- Privacy Shield/Wardveil state.

Trusted-chrome Environmental Color Memory influence is **0%**. Optical appearance cannot be used as an origin trust signal.

Forced Colors and Reduced Transparency take precedence and must provide an effects-free solid path. Increased Contrast must strengthen clarity/focus rather than enabling decorative adaptation. Existing Android effects-free rendering remains a valid first-class V1.4 path.

## Appearance, clarity, and expression

Browser may expose Glaze Appearance (`system`, `light`, `dark`, `deep-dark`), Clarity (`clear`, `balanced`, `solid`), and Expression (`calm`, `balanced`, `expressive`) where supported. These axes may alter presentation and motion intensity but never change security, privacy, origin identity, persistence, availability, authorization, or workflow meaning.

## Address and search

The unified address/search surface is a primary Navigation Capsule candidate. It must preserve at least 48 px interactive targets, visible focus, keyboard-first and touch-first operation, bounded overflow, and complete idle/hover/pressed/selected/expanded/loading/invalid/disabled/read-only states where applicable.

Search results and suggestions remain governed by GoreeCloud Search and applicable Browser/search contracts. Glaze UI governs presentation and interaction only.

The omnibox may display authoritative origin/security information, but V1.4 material color/tint must never substitute for explicit origin, certificate, connection, permission, or Wardveil evidence.

## Tabs and connected transformation

Tab reorder, detach, restore, grouping, and transition flows may use Connected Transformation where task identity remains continuous. Motion must be interruptible, preserve tab identity, and provide reduced-motion substitutions. Tabs must distinguish active, inactive, focus, loading, attention, pinned, grouped, suspended, and closing states without color-only meaning.

## Navigation controls

Back, forward, reload/stop, home, site information, privacy/security state, bookmarks, downloads, profile, and menu controls use governed Glaze iconography and minimum target contracts. General targets are at least 48 px; Touch Assistance/far-view targets are at least 56 px where applicable.

## New Tab, Home, and settings

New Tab and Home are first-party GoreeCloud surfaces and use responsive Glaze composition rather than Chromium-themed pages. Settings use semantic grouping, progressive disclosure, search, stable navigation, responsive panes, and accessible relationships while preserving Privacy Shield, Wardveil Security, Everkeep, Mesh, Identity, Vault, DNS, Network, Sync, and Search authority boundaries.

Glaze UI must never make a planned, unavailable, unknown, expired, or degraded service appear active or healthy.

## Downloads, Library, and Browser utilities

Downloads, Bookmarks, history/library experiences, saved content, Reader Mode, Clipboard Manager, media actions, Network/DNS and proxy tooling use consistent collection, selection, progress, error, completion, filtering, sorting, and bulk-action patterns. Destructive operations require explicit interaction treatment.

Live Surfaces may present bounded current activity, but animation, blur, tint, warmth, or translucency cannot substitute for state labels or authoritative completion evidence.

## Private Browsing

Private Browsing uses the same current Glaze system while remaining unmistakably distinct in persistence expectations. Visual differentiation must not create unsupported privacy claims. Privacy Shield remains authoritative for privacy behavior.

Private mode must not unlock page-derived visual adaptation of trusted chrome. A page cannot make ordinary browsing appear private, or private browsing appear more/less protected, through color or optical influence.

## Security and permissions

Browser-owned security presentation consumes Wardveil, engine, certificate, and applicable platform evidence-backed state. Certificate details, operating-system dialogs, engine-critical warnings, and platform permission surfaces may retain native or engine-required presentation when replacement would reduce security, accessibility, or compatibility.

Security truth always outranks Glaze optical expression.

## Responsive behavior

Glaze layout classes map Browser composition rather than simply scaling controls:

- **Compact** — phone and narrow-window composition.
- **Medium** — tablet and small-window composition with selective secondary panes.
- **Expanded** — desktop composition with persistent Browser chrome and richer parallel context.
- **Wide** — large desktop/TV-capable composition where additional space improves task context.

Safe areas, system bars, window controls, virtual keyboards, display cutouts, foldable hinges, gesture regions, localization, directionality, and bounded intrinsic overflow must be handled explicitly.

## Production acceptance

Compiling against V1.4 metadata is not production acceptance. Representative Browser task flows must demonstrate current-Stable behavior across supported platforms, layout/input/accessibility modes, relevant form-factor constraints, and the trusted-chrome/page-content separation before Browser can use that evidence for release qualification.

Applicable V1.4.1 human/manual/device optical validation remains outstanding until actual evidence is recorded.

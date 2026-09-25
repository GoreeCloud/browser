# GoreeCloud Browser — Glaze UI Surface Model

This document maps GLAZE UI V1.6 / 1.6.0 into Browser-owned product surfaces. Canonical GoreeCloud lifecycle is Anchor; retained Stable terminology refers only to the compatible consumer release channel. It is an implementation specification, not a claim that every surface has completed acceptance.

## Browser chrome

The primary Browser frame uses Glaze semantic layout, spacing, color, material, iconography, motion, focus, and interaction contracts while Chromium/Blink remains the web-engine layer.

### Material hierarchy

Browser maps the V1.6 material roles into native platform primitives:

- **Canvas** — root application/background plane.
- **Solid / Raised** — high-legibility reading, settings, warning, and durable content surfaces.
- **Functional Glass** — bounded navigation, omnibox, toolbar, and transient-control surfaces.
- **Clear Glass** — specialized presentation only where media/background context remains readable.
- **Overlay** — explicit layered separation for menus, popovers, dialogs, and sheets.

The inherited Browser names Soft Glaze, Glaze, Deep Glaze, and Live Glaze remain compatibility mappings where they are already implemented; they do not override the V1.6 clarity and accessibility rules.

Reduced Transparency, unsupported backdrop effects, runtime pressure, or readability constraints must collapse optional effects toward solid/raised presentation without losing hierarchy, labels, focus, state, or task continuity.

## Appearance, clarity, and expression

Browser may expose Glaze Appearance (`system`, `light`, `dark`, `deep-dark`), Clarity (`clear`, `balanced`, `solid`), and Expression (`calm`, `balanced`, `expressive`) where supported. These axes may alter presentation and motion intensity but never change security, privacy, persistence, availability, or workflow meaning.

## Address and search

The unified address/search surface is a primary Navigation Capsule candidate. It must preserve at least 48 px interactive targets, visible focus, keyboard-first and touch-first operation, bounded overflow, and complete idle/hover/pressed/selected/expanded/loading/invalid/disabled/read-only states where applicable.

Search results and suggestions remain governed by GoreeCloud Search authority. Glaze UI governs presentation and interaction only.

## Tabs and connected transformation

Tab reorder, detach, restore, grouping, and transition flows may use Connected Transformation where task identity remains continuous. Motion must be interruptible, preserve tab identity, and provide reduced-motion substitutions. Tabs must distinguish active, inactive, focus, loading, attention, pinned, grouped, suspended, and closing states without color-only meaning.

## Navigation controls

Back, forward, reload/stop, home, site information, privacy/security state, bookmarks, downloads, profile, and menu controls use governed Glaze iconography and minimum target contracts. General targets are at least 48 px; TV targets are at least 56 px.

## New Tab, Home, and settings

New Tab and Home are first-party GoreeCloud surfaces and use responsive Glaze composition rather than Chromium-themed pages. Settings use semantic grouping, progressive disclosure, search, stable navigation, responsive panes, and accessible relationships while preserving Privacy Shield, Wardveil Security, Everkeep, Mesh, Identity, Vault, DNS, Network, Sync, and Search authority boundaries.

Glaze UI must never make a planned, unavailable, unknown, expired, or degraded service appear active or healthy.

## Downloads, Library, and Browser utilities

Downloads, Bookmarks, history/library experiences, saved content, Reader Mode, Clipboard Manager, media actions, Network/DNS and proxy tooling use consistent collection, selection, progress, error, completion, filtering, sorting, and bulk-action patterns. Destructive operations require explicit interaction treatment.

Live Surfaces may present bounded current activity, but animation or translucency cannot substitute for state labels or authoritative completion evidence.

## Private Browsing

Private Browsing uses the same current Glaze system while remaining unmistakably distinct in persistence expectations. Visual differentiation must not create unsupported privacy claims. Privacy Shield remains authoritative for privacy behavior.

## Security and permissions

Browser-owned security presentation consumes Wardveil evidence-backed state. Certificate details, operating-system dialogs, engine-critical warnings, and platform permission surfaces may retain native or engine-required presentation when replacement would reduce security, accessibility, or compatibility.

## Responsive behavior

Glaze layout classes map Browser composition rather than simply scaling controls:

- **Compact** — phone and narrow-window composition.
- **Medium** — tablet and small-window composition with selective secondary panes.
- **Expanded** — desktop composition with persistent Browser chrome and richer parallel context.
- **Wide** — large desktop/TV-capable composition where additional space improves task context.

Safe areas, system bars, window controls, virtual keyboards, display cutouts, foldable hinges, gesture regions, localization, directionality, and bounded intrinsic overflow must be handled explicitly.

## Production acceptance

Compiling against GLAZE UI V1.6 / 1.6.0 metadata is not production acceptance. Representative Browser task flows must demonstrate current-Stable behavior across supported platforms, layout/input/accessibility modes, and relevant form-factor constraints before Browser can use that evidence for release qualification.


## Linux GTK stabilization mapping

The GTK/X11 beta shell uses a Browser-owned V1.6 native mapping rather than framework-default chrome. Primary navigation is grouped, the omnibox remains the dominant navigation capsule, privacy/security entry points remain neutral until authoritative state is supplied, and secondary tools use an accessible overflow surface. Browser-owned New Tab, Home, Settings, Private, and tool surfaces use structured solid/raised presentation rather than raw centered status text.

Symbolic theme icons are used only when the local GTK icon theme provides them; compact local text glyphs remain the deterministic fallback so missing theme assets cannot create blank controls. Tooltips and accessible names remain authoritative labels.

This source mapping is implementation evidence only. Native rendered review, narrow-window behavior, assistive-technology review, large-text, localization/RTL, reduced-effects, performance, and representative-device acceptance remain required.

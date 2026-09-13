# GoreeCloud Browser — GLAZE UI V1.4 Adoption

Status: **Migration in progress / Development**  
Current Stable target: **GLAZE UI V1.4 (`1.4.0`)**  
Canonical repository: `GoreeCloud/goreecloud-glaze-ui`  
Exact Stable merged source authority: `84cb3db4884042f0fa25ed6d475a127fb110f596`  
Browser policy: `latest-approved-stable`  
Production eligible from this migration alone: **No**

GoreeCloud Browser tracks the latest approved Stable Glaze UI release. The previous repository state contained split design-system authority: Android Browser chrome targeted the retired `2.2.0` line while shared desktop/native C++ metadata still targeted `2.0.0`, and the Platform Contract compatibility floor was pinned to `1.1.0`. V1.4 migration eliminates that drift and establishes one exact design-system source authority across Browser-owned Android and desktop/native presentation contracts.

This is migration evidence, not automatic Browser acceptance. A central Glaze Stable promotion never certifies Browser by itself.

## Preserved material and interaction contracts

Browser retains the established material hierarchy of Canvas, Surface, Soft Glaze, Glaze, Deep Glaze, and Live Glaze; native/platform authority; readable solid content; bounded Glaze interaction chrome; Balanced clarity; Calm expression; visible focus/pressed behavior; semantic accessibility labels; light/dark adaptation; vector Browser icons; and effects-free operation that does not require blur, transparency, or animation.

General touch-oriented targets remain at least 48 dp. The 56 dp Touch Assistance/far-view floor remains represented where applicable. Browser does not claim a product-wide Touch Assistance runtime preference is wired merely because that contract exists in source.

Appearance, material, motion, optical adaptation, and color are presentation contracts only. They cannot manufacture Wardveil security state, engine/certificate trust state, Privacy Shield privacy state, Everkeep continuity state, Mesh coordination state, Identity authorization, or Browser workflow success.

## V1.4 Optical Intelligence trust boundary

Browser treats rendered webpage content as an **untrusted content plane** and Browser-owned chrome as a **trusted application plane**.

V1.4 must never create a spoofing or observation path from web content into trusted chrome. Therefore the Browser-specific optical policy prohibits all of the following from driving Browser-owned trusted chrome appearance:

- webpage pixels or dominant colors;
- CSS/theme colors;
- favicon colors;
- document text or media frames;
- origin/domain identity;
- browsing history or query text;
- certificate/security indicators;
- privacy indicators; or
- page-provided metadata.

Environmental Color Memory influence for trusted Browser chrome is intentionally **0%**. Decorative page-derived tinting is disabled. Optical appearance can never imply a secure origin, trusted certificate, Wardveil verdict, Privacy Shield state, private-browsing state, download safety, or successful permission/authorization result.

`GlazeBrowserOptics` on Android and `BrowserGlazeOpticalPolicy` in shared native C++ encode this boundary explicitly.

## Accessibility precedence

V1.4 optical behavior remains subordinate to accessibility and semantic truth:

1. Forced Colors or closest native/platform equivalent.
2. Reduced Transparency / effects-free operation.
3. Increased Contrast and visible focus.
4. Origin/security/privacy truth and critical Browser state.
5. Task legibility and interaction target requirements.
6. Decorative material/optical expression.

On Android, Reduced Transparency and Forced Colors resolve to `SOLID_ACCESSIBLE` with zero blur and full semantic protection. Increased Contrast raises protection while decorative tint remains disabled.

The existing Android style layer remains effects-free by default. V1.4 does not make blur/transparency a dependency for usability.

## System Shell and authority classification

Browser-owned chrome remains **Application** scope. Local Browser menu and omnibox/search behavior are not relabeled as Universal Search, Control Center, System Panel, or Critical System UI.

GoreeCloud Search remains the search/current-information provider relationship for non-URL query workflows where the Browser product contract invokes it. Glaze UI presentation does not transfer search/indexing authority.

## System Glaze budget

The Android contract retains at most one dominant Glaze panel and up to three small floating Glaze controls for ordinary mapped composition. Current Browser mapping remains inside that bound.

Web/content reading planes remain solid. Soft Glaze remains bounded to transient Browser-owned interaction chrome such as the omnibox and menu sheet.

## Cross-platform authority

The exact current Glaze authority is now recorded in both major Browser-owned source boundaries:

- Android: `apps/android/app/src/main/java/io/goreecloud/browser/GlazeContract.kt`
- Android optical trust policy: `apps/android/app/src/main/java/io/goreecloud/browser/GlazeBrowserOptics.kt`
- Desktop/native shared contract: `include/goreecloud/browser/glaze.hpp`
- Desktop/native production acceptance requirements: `include/goreecloud/browser/glaze_acceptance.hpp`

Platform renderers may differ in implementation, but they may not silently target different Glaze lifecycle versions or weaken the page/trusted-chrome boundary.

## Browser-owned scope

The requirement covers browser chrome, navigation, unified address/search, tabs, New Tab, Home, Settings, Downloads, Library, Bookmarks, Private Browsing landing surfaces, Browser-owned print/PDF controls, media actions, Clipboard Manager, Reader Mode, context menus, Proxy Manager, Network and DNS settings, memory controls, Wayfinder features, and GoreeCloud service integrations.

OS-owned, certificate, permission, engine-critical, and Developer Tools surfaces may preserve required native or engine presentation where replacement would reduce security, accessibility, or compatibility. That exception cannot be used to keep ordinary GoreeCloud-owned UI on a superseded design language.

## V1.4.1 human-validation boundary

By the shared Glaze UI V1.4 release decision, human validation and human verification are assigned to V1.4.1 rather than blocking the shared V1.4.0 lifecycle. Browser does **not** represent these checks as passed V1.4.0 evidence.

Applicable follow-up includes subjective optical-quality review, manual assistive-technology review, representative physical-device/platform validation, and representative real-device performance/power/thermal qualification. Browser product release requirements remain independent and may be stricter.

Historical Browser visual/device evidence against earlier Glaze versions remains historical evidence only and must not be relabeled as V1.4 acceptance.

## Acceptance still required

Before Browser may claim current-Stable Glaze conformance, applicable Browser-owned task flows must establish repository-local acceptance for supported platform families, layout classes, input modalities, accessibility modes, localization/directionality cases, safe-area/hinge constraints, and representative hardware.

For Android, source tests and a successful CI APK are necessary but not sufficient. Remaining gates include rendered V1.4 review, TalkBack/accessibility semantics, 200% text/reflow, Reduced Motion, Reduced Transparency/effects-free behavior, Increased Contrast, Forced Colors or closest platform equivalent, RTL/localization, Touch Assistance mapping where supported, orientation/form-factor behavior, trusted-chrome spoof-resistance review, representative-device behavior, and performance.

For desktop/Linux, the shared V1.4 contract and static assertions are source evidence only. Real GTK/native rendering, keyboard/focus behavior, effects-free fallbacks, mixed input, window/safe-area behavior, localization, and real runtime acceptance remain separate gates.

## Upgrade and rollback process

For each future Glaze UI Stable promotion, Browser must record the new semantic version and exact reviewed source revision, audit changed contracts, update Android and shared native mappings together, run Browser-specific acceptance, validate representative supported clients, and remain Development/nonconformant wherever required evidence is incomplete.

V1.3 / `1.3.0` is the immediate shared Glaze rollback baseline. Historical Browser 2.x mappings remain useful only for repository history and diagnosis; they are not current lifecycle authority and cannot establish present conformance.

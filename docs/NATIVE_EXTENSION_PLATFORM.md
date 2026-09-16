---
title: "GoreeCloud Browser — Native Extension Platform"
version: "v0.1"
status: "Proposed"
classification: "Internal"
document_type: "Browser Native Extension Platform Supporting Specification"
project: "GoreeCloud Browser"
repository: "GoreeCloud/goreecloud-browser"
canonical_path: "docs/NATIVE_EXTENSION_PLATFORM.md"
authoritative_scope: "Planned native extension-platform architecture and requirements"
created: "2026-09-16"
last_updated: "2026-09-16"
---

# GoreeCloud Browser — Native Extension Platform

> **Status and evidence boundary:** This document defines proposed and planned GoreeCloud Browser extension-platform architecture, security boundaries, user controls, developer tooling, packaging, APIs, update behavior, and acceptance direction. It does **not** claim that the native extension runtime, `.gcex` package format, APIs, developer SDK, management surfaces, or any other capability described here is implemented, accepted, production-approved, or Stable.

GoreeCloud Browser should include its own **native extension platform**, designed specifically around GoreeCloud Browser's architecture, privacy model, security controls, Glaze UI, and broader GoreeCloud ecosystem.

The browser should **not depend on external browser-extension ecosystems or extension compatibility layers**. GoreeCloud Browser extensions should use a dedicated GoreeCloud extension API, manifest format, permission model, runtime, packaging format, and developer tooling.

## 1. Core Principles

The native extension platform should be:

- Native to GoreeCloud Browser.
- Decentralized in distribution.
- Permission-based and least-privilege.
- Sandboxed and process-isolated.
- Transparent about behavior, permissions, networking, storage, and updates.
- Developer-friendly without mandatory registration.
- User-controlled at installation, runtime, update, and removal.
- Deeply integrated with Browser capabilities and Glaze UI without giving extensions unrestricted Browser or operating-system authority.

Security enforcement and distribution control must remain separate. GoreeCloud Browser should strongly enforce extension boundaries without requiring GoreeCloud to operate a centralized extension marketplace.

## 2. Native Extension Architecture

Extensions should run through a built-in **GoreeCloud Browser Extension Runtime**.

The runtime should provide controlled interfaces for interacting with:

- Tabs.
- Windows.
- Pages.
- Navigation.
- Downloads.
- Bookmarks.
- History.
- Reader features.
- Browser actions.
- Context menus.
- Search.
- Site permissions.
- Networking.
- Cookies and site storage.
- Notifications.
- Keyboard shortcuts.
- Side panels.
- Glaze UI components.
- GoreeCloud Browser settings.

Extensions should never receive unrestricted access to the browser or operating system simply because they are installed.

Each API surface should expose only the minimum capability required for its declared purpose. Privileged operations should remain mediated by Browser-owned policy, permission, profile, privacy-context, and security boundaries.

## 3. No Extension Store or Extension Hub

GoreeCloud should **not operate a centralized extension marketplace, store, catalog, repository, or discovery hub**.

Users remain responsible for obtaining extensions from developers, organizations, repositories, or other sources they trust.

The browser should concentrate on **secure installation and management**, rather than extension distribution.

Users should be able to install an extension from:

- A local extension package.
- A local development directory.
- A downloaded extension package.
- A trusted network location.
- A direct developer-provided package.
- An administrator-managed deployment source.

The browser should clearly show where an installed extension originated and preserve that provenance in the extension-management interface.

## 4. Custom Extensions and Developer Mode

Users should be able to create and install their own extensions without registering with GoreeCloud or publishing them anywhere.

A built-in **Developer Mode** should support:

- Loading unpacked extensions.
- Reloading an extension after changes.
- Viewing extension logs.
- Inspecting extension processes.
- Testing permissions.
- Inspecting storage.
- Debugging background tasks.
- Debugging page scripts.
- Viewing API calls.
- Viewing runtime errors.
- Simulating permission changes.
- Packaging completed extensions.

This makes GoreeCloud Browser useful for personal automation, organizational tools, experimentation, development, accessibility improvements, and specialized workflows.

Developer Mode must not silently weaken unrelated Browser security boundaries. Development trust state, source, permissions, and runtime behavior should remain visible.

## 5. GoreeCloud Extension Package

GoreeCloud Browser should define its own extension package format.

The planned package format is:

**`.gcex` — GoreeCloud Extension Package**

A package could contain:

```text
extension.gcex
├── manifest
├── scripts/
├── pages/
├── styles/
├── assets/
├── locales/
└── signatures/
```

The package format should be documented openly so anyone can build compatible extensions.

The package format should support deterministic validation of structure, manifest content, declared capabilities, package integrity, signatures when present, and compatibility metadata before installation.

## 6. Extension Manifest

Every extension should contain a GoreeCloud-native manifest declaring what the extension is and what capabilities it requests.

The manifest could define:

- Extension ID.
- Name.
- Version.
- Description.
- Author.
- Homepage.
- Minimum browser version.
- Entry points.
- Requested permissions.
- Allowed websites.
- Background capabilities.
- UI integrations.
- Keyboard shortcuts.
- Update information.
- Content Security Policy.
- Extension dependencies.
- Network permissions.
- Storage requirements.
- Signature information.

Capabilities not declared in the manifest should be unavailable to the extension.

Manifest validation should fail safely on malformed, unsupported, ambiguous, or incompatible declarations rather than silently broadening capability.

## 7. Permission-Based Security

Extension permissions should follow **least privilege**.

Instead of giving an extension broad browser access, GoreeCloud Browser should expose narrowly scoped permissions such as:

- Read current page.
- Modify current page.
- Access selected websites.
- Access all websites.
- Read tabs.
- Create tabs.
- Read browsing history.
- Modify bookmarks.
- Manage downloads.
- Access clipboard.
- Display notifications.
- Store extension data.
- Perform network requests.
- Add context-menu actions.
- Add browser-interface components.

Permissions should be understandable to ordinary users rather than represented only as vague technical capabilities.

Permission grants should remain independently revocable, scope-aware, and visible after installation.

## 8. Per-Site Permissions

Users should be able to determine where an extension operates.

For every extension, **Site Access** should support:

- Never.
- Only when clicked.
- Current website.
- Selected websites.
- All websites.

Users should also be able to create permanent allowlists and blocklists.

Site rules should be scoped to the appropriate extension and profile and must not silently grant access to unrelated origins.

## 9. Temporary Permissions

Sensitive permissions should optionally be granted temporarily.

Supported grant lifetimes should include:

- Once.
- Until the tab closes.
- Until the website closes.
- Until the browser closes.
- For one hour.
- Always.

Temporary authorization should expire automatically when its declared scope ends.

This model should reduce unnecessary permanent extension privileges and make one-time or task-specific access practical.

## 10. Extension Isolation

Each extension should run inside an isolated execution environment.

Extensions should be separated from:

- Other extensions.
- Browser internals.
- User profiles.
- Unrelated websites.
- System files.
- GoreeCloud services.
- Authentication credentials.

One compromised extension should not automatically compromise the browser or other extensions.

Isolation should remain enforceable across background work, page scripts, extension UI, storage, networking, and process lifecycle.

## 11. Extension Process Controls

The browser's process manager should display extension resource usage, including:

- CPU usage.
- Memory usage.
- Network activity.
- Storage usage.
- Background activity.
- Active scripts.

Users should be able to terminate an extension without restarting the browser.

Terminating an extension should end or suspend its active runtime work without granting it an alternate path to continue background execution.

## 12. Network Transparency

A privacy-focused extension manager should expose extension networking activity.

For example:

```text
Network Activity

Connections today: 18

example.net        12 requests
api.example.org     4 requests
cdn.example.net     2 requests
```

Users should be able to inspect and block individual destinations.

Network reporting should be scoped to extension activity and should not misrepresent ordinary webpage or Browser-owned traffic as extension traffic.

## 13. Extension Privacy Dashboard

Every extension should have a privacy dashboard showing:

- Permissions granted.
- Permissions denied.
- Websites accessed.
- Network destinations contacted.
- Data stored.
- Background activity.
- Last active time.
- Permission changes.
- Update history.

This dashboard should make extension behavior materially more transparent than a simple enabled/disabled view.

## 14. Permission Activity Log

GoreeCloud Browser should maintain a local extension activity history.

Example:

```text
10:14 AM — Read page contents
10:14 AM — Accessed clipboard
10:16 AM — Connected to example.net
10:21 AM — Wrote 4 KB to extension storage
```

Logging should remain local unless the user explicitly exports it.

Activity history should be bounded, privacy-conscious, and configurable so observability does not itself become unnecessary collection.

## 15. Extension Firewall

Users should be able to apply network policies specifically to extensions.

Controls should include:

- Allow network access.
- Block all network access.
- Allow selected domains.
- Block selected domains.
- Local-network access.
- Background-network access.
- Metered-network access.

An extension that only modifies page appearance, for example, should be able to operate with network access disabled entirely.

The extension firewall should apply independently from ordinary website networking and Browser-owned service traffic.

## 16. Private Browsing Controls

Extensions should be disabled in private browsing by default.

Users may explicitly permit individual extensions to run in private sessions.

The permission should clearly explain whether the extension can:

- Read private pages.
- Modify private pages.
- Store information.
- Make network connections.

Private-session information should not automatically become normal extension history.

Private extension state should remain isolated from normal-profile activity unless an explicit, user-authorized capability defines otherwise.

## 17. Profiles

Extension installations and permissions should be profile-specific.

A user might have:

```text
Personal
├── Extension A
├── Extension B
└── Extension C

Work
├── Extension D
└── Extension E

Research
└── No extensions
```

This prevents unnecessary extension access across unrelated browser identities.

Installation, permission, storage, update, activity, and private-browsing state should respect profile boundaries.

## 18. Glaze UI Integration

Extensions should be able to integrate naturally with Glaze UI without unrestricted access to the browser interface.

Supported extension surfaces could include:

- Toolbar actions.
- Address-bar actions.
- Context-menu actions.
- Side panels.
- Popovers.
- Settings panels.
- New-tab widgets.
- Page overlays.
- Developer panels.

Extension interfaces should inherit approved Glaze UI primitives such as:

- Typography.
- Spacing.
- Shapes.
- Transparency.
- Translucency.
- Blur.
- Motion.
- Theme colors.
- Accessibility settings.
- Light and dark appearance.

This should make extensions feel native while maintaining visual consistency.

Extension UI APIs should expose approved composition primitives rather than unrestricted mutation of Browser-owned chrome.

## 19. Browser UI Protection

Extensions should not be permitted to silently imitate sensitive browser interfaces.

Protected surfaces should include:

- Permission dialogs.
- Authentication prompts.
- Security warnings.
- Browser settings.
- Extension installation prompts.
- Privacy indicators.

The browser should visually distinguish extension-controlled UI from browser-controlled security UI.

Extensions must not be able to suppress or replace protected Browser security indicators in a way that could mislead users about who controls the surface.

## 20. Signed and Unsigned Extensions

GoreeCloud Browser should support both signed and locally developed extensions.

Supported trust states should include:

**Verified Signature**  
Package integrity and developer signature successfully verified.

**Signed**  
Package signature is valid but has not received additional verification.

**Local Development**  
Loaded directly from a development directory.

**Unsigned**  
No cryptographic signature is present.

Unsigned extensions should remain installable because GoreeCloud does not depend on a centralized distribution service, but the browser should clearly communicate their trust state.

Trust state must not be represented as proof that an extension is safe, privacy-preserving, or appropriate for a particular user merely because its package signature is valid.

## 21. Update Control

Extensions should not require a centralized GoreeCloud service to receive updates.

Supported update modes should include:

- Manual updates.
- Developer-provided update source.
- Local update package.
- Organization-managed updates.
- Automatic updates from a user-approved source.

Users should be able to disable automatic updating per extension.

The update source should remain visible and independently configurable where applicable.

## 22. Update Transparency

Before installing an update, GoreeCloud Browser should detect newly requested permissions.

Example:

```text
Extension Update

Version 2.4 → 2.5

New permissions requested:

• Read browsing history
• Access *.example.net
• Display notifications

[Review Update] [Reject] [Install]
```

Permission escalation should never happen silently.

An update that requests new or broader sensitive access should remain pending until the user or authorized administrator approves the changed capability scope.

## 23. Extension Rollback

Previous extension versions should optionally be retained locally.

If an update creates problems, users should be able to select:

**Restore Previous Version**

The browser should also be able to automatically disable an extension if repeated crashes begin immediately after an update.

Rollback should restore the package version without silently restoring broader permissions that the user has since revoked.

## 24. Extension Emergency Disable

Users should have a browser-wide:

**Disable All Extensions**

control.

Additional troubleshooting options should include:

- Restart without extensions.
- Disable recently updated extensions.
- Disable extensions with errors.
- Disable extensions using excessive resources.

Emergency controls should remain available even when an extension is crashing or misbehaving.

## 25. Extension Compatibility Information

The extension manager should clearly display:

- Installed version.
- Required browser version.
- Supported APIs.
- Missing capabilities.
- Deprecated APIs.
- Compatibility warnings.

Compatibility information should distinguish hard incompatibility from warnings or optional unavailable features.

## 26. API Versioning

The GoreeCloud Extension API should be versioned independently from the browser.

For example:

```text
Extension API 1
Extension API 2
Extension API 3
```

Older API versions should remain supported for defined compatibility periods while newer APIs evolve.

Browser release version and Extension API version must remain separate identities.

## 27. Deprecation Model

APIs should not disappear unexpectedly.

A structured process should include:

1. API marked deprecated.
2. Developer warning introduced.
3. Migration documentation published.
4. Compatibility period maintained.
5. API eventually removed.

Extensions should receive warnings well before compatibility is lost.

Removal should occur only after the documented compatibility period and migration path have been honored, except where an urgent security boundary requires earlier action under a separately documented security process.

## 28. Declarative APIs

Whenever practical, GoreeCloud Browser should favor **declarative extension APIs** instead of giving extensions unrestricted scripting capabilities.

For example, an extension could describe:

> Block requests matching these rules.

rather than receiving direct access to every network request.

Declarative APIs should reduce attack surface while still supporting powerful extensions and auditable behavior.

## 29. Capability Tokens

Sensitive extension privileges should internally support temporary capability tokens or an equivalent bounded authorization mechanism.

An extension requesting access to the current page could receive authorization only for:

```text
Profile: Personal
Tab: 27
Origin: example.net
Permission: page.read
Lifetime: tab session
```

The authorization should become invalid automatically after its scope expires.

Capability authorization should bind at least the extension identity, profile, target object or origin, permission, and lifetime required by the operation.

## 30. Extension Storage

Extensions should receive dedicated isolated storage.

Supported storage classes could include:

- Temporary.
- Session.
- Local.
- Profile synchronized.
- Encrypted.
- Cache.

Extensions should not automatically gain access to arbitrary browser databases.

Storage classes should have explicit lifecycle, profile, synchronization, privacy-context, and encryption semantics where applicable.

## 31. Storage Limits

Users should be able to inspect and limit extension storage.

Example:

```text
Storage

Extension data: 14.7 MB
Cache: 38.2 MB

Storage limit: 100 MB
```

Quota exhaustion should fail predictably without allowing an extension to consume unbounded Browser or system storage.

## 32. Extension Backup and Synchronization

Extension configuration should optionally participate in GoreeCloud backup and synchronization.

This may include:

- Installed-extension list.
- Settings.
- Permissions.
- Site rules.
- Keyboard shortcuts.

Actual extension packages should remain locally sourced rather than automatically uploaded as ordinary synchronized Browser state.

Backup or synchronization must preserve profile boundaries, user intent, trust/source information, and the independent authority of the applicable GoreeCloud backup or synchronization service.

## 33. Enterprise and Organization Deployment

Organizations should be able to deploy extensions without requiring a public extension service.

Administrative policies should be able to:

- Install extensions.
- Pin versions.
- Configure permissions.
- Restrict websites.
- Prevent removal.
- Disable particular extension capabilities.
- Define trusted signing authorities.
- Configure update locations.

Managed policy should remain visible to affected users and should distinguish administrator-enforced state from user-controlled state.

## 34. Developer SDK

GoreeCloud Browser should provide a dedicated **GoreeCloud Extension SDK** containing:

- API documentation.
- Type definitions.
- Packaging utilities.
- Manifest validation.
- Permission validation.
- Testing tools.
- Debugging utilities.
- Example extensions.
- Migration tools between GoreeCloud Extension API versions.

The SDK should be usable without registering a developer account.

SDK tooling should support reproducible validation of extension structure, declared permissions, API compatibility, and package output.

## 35. Command-Line Development Tools

Extension developers should also have access to command-line tooling for:

```text
create
validate
test
run
package
sign
inspect
permissions
```

This allows extension development to work naturally with automated development environments, continuous integration, and local developer workflows.

Command-line tooling should produce non-secret, automation-friendly output and fail clearly on invalid manifests, permissions, packaging, signatures, or compatibility.

## 36. Extension Manager

The browser should include a dedicated:

**Settings → Extensions**

interface.

Each extension could display:

```text
Extension Name
Version 1.8

Status: Enabled
Trust: Signed
Source: Local Package

Permissions
Site Access
Network Access
Storage
Activity
Updates
Keyboard Shortcuts
Developer Information

[Disable] [Remove]
```

The extension manager should be the primary user-facing control surface for installation state, trust/source information, permissions, site access, network policy, storage, activity, updates, shortcuts, troubleshooting, and removal.

## 37. Extension Safety Check

The browser should continuously perform local checks for problematic extension behavior, including:

- Excessive CPU usage.
- Excessive memory usage.
- Repeated crashes.
- Unexpected background activity.
- New network destinations.
- Permission escalation.
- Corrupted packages.
- Invalid signatures.
- Modified extension files.

The browser should be able to warn the user without automatically transmitting extension information elsewhere.

Local safety checks should remain transparent about what was detected and should distinguish observed behavior from unsupported conclusions about developer intent.

## 38. Advanced User Controls

Advanced users should be able to configure:

- Extension process isolation.
- Memory limits.
- CPU limits.
- Storage quotas.
- Network rules.
- Background execution limits.
- Script execution limits.
- Allowed APIs.
- Site-specific overrides.

Advanced controls should not weaken mandatory Browser security boundaries that are not user-overridable.

## 39. Developer Freedom Without Centralized Gatekeeping

The extension platform should deliberately separate **security enforcement from distribution control**.

GoreeCloud Browser can strongly sandbox extensions, require explicit permissions, expose activity, validate package integrity, and protect users without requiring developers to submit extensions to GoreeCloud.

Developers can distribute their work directly.

Users decide what they trust.

GoreeCloud Browser enforces the boundaries.

## 40. Implementation and Acceptance Direction

The native extension platform should be implemented in controlled stages rather than enabling unrestricted scripting first.

A suitable sequencing model is:

1. Define the `.gcex` package structure and versioned manifest schema.
2. Define extension identities, trust/source states, package-integrity validation, and installation provenance.
3. Implement isolated extension process/runtime boundaries without privileged Browser APIs enabled by default.
4. Implement the least-privilege permission model, per-site access, temporary grants, revocation, and capability-scoped authorization.
5. Implement dedicated extension storage classes, quotas, profile isolation, private-browsing defaults, and cleanup semantics.
6. Implement declarative APIs before unrestricted equivalents wherever practical.
7. Add bounded tabs/pages/navigation/bookmarks/history/downloads/search/context-menu/notification/shortcut APIs as their permission and security models are accepted.
8. Add Glaze UI extension surfaces while preserving protected Browser-controlled security UI.
9. Implement extension networking transparency, firewall policy, activity history, privacy dashboard, and process/resource controls.
10. Implement signed/unsigned/local-development trust states, developer packaging/signing tools, and user-controlled decentralized update sources.
11. Add permission-delta review, rollback, crash-triggered disable, emergency disable, compatibility reporting, and deprecation tooling.
12. Add Developer Mode, SDK, CLI tools, debugging, validation, testing, and example-extension support.
13. Add optional backup/synchronization of configuration without treating executable packages as ordinary synchronized state.
14. Add organization-managed deployment and trusted-signing policy without creating a centralized public distribution requirement.
15. Complete platform, security, privacy, accessibility, performance, resource-abuse, update/rollback, compatibility, migration, representative-device, and sustained-use acceptance before production claims.

Production acceptance should include deterministic automated tests, adversarial/fault testing, package-corruption tests, signature tests, permission-escalation tests, site-scope tests, private/profile isolation tests, network-policy tests, storage-quota tests, crash/recovery tests, update/rollback tests, API-version/deprecation tests, UI-spoofing tests, accessibility tests, and representative-device evidence.

No extension capability should be represented as production-safe merely because its API exists. The exact implementation revision must satisfy the applicable security, privacy, reliability, accessibility, compatibility, and resource-governance gates.

# Final Design Principle

The **GoreeCloud Browser Extension Platform** should be:

**native, decentralized, permission-based, sandboxed, transparent, developer-friendly, user-controlled, and deeply integrated with GoreeCloud Browser.**

There is no centralized extension store.

There is no requirement to publish extensions.

There is no requirement to register as a developer.

Users may build their own extensions, obtain extensions directly from developers, inspect their capabilities, control exactly what they can access, and remove them whenever they choose.

The browser provides the **platform and security model**.

Developers provide the extensions.

Users retain control.

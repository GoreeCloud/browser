# Native GoreeCloud Vault Integration

## Status

Planned / incomplete. This document records a required Browser integration boundary; it does not claim implementation, production acceptance, or Stable qualification.

## Product requirement

GoreeCloud Browser must include GoreeCloud Vault as a native first-party credential capability. This must not require installing the GoreeCloud Vault Firefox Extension or another WebExtension inside GoreeCloud Browser.

The native Browser experience is expected to provide secure username/password autofill, inline credential suggestions, password and passphrase generation, save/update prompts, passkey/WebAuthn flows, TOTP access, payment and identity autofill where permitted, Vault quick access, lock/unlock and reauthentication, multiple-account selection, and origin-aware security warnings.

## Authority boundary

GoreeCloud Vault remains authoritative for protected credentials, passkeys, secrets, secure autofill material, encryption, Vault state, and credential authorization. GoreeCloud Browser owns Browser presentation, form-event integration, user prompts, and Browser Settings surfaces.

Browser must use explicit versioned Vault capabilities and must not directly read or reinterpret the Vault database, manufacture a second credential store, persist plaintext credentials as Browser-owned state, or silently downgrade to an unverified credential source.

## Privacy and private browsing

Normal, Private, and Isolated Private contexts require explicit policy. Vault use in private contexts must not silently persist private origin activity, fill events, page metadata, credential-use metadata, or private-window history into ordinary Browser state or GoreeCloud Sync.

## Fail-closed behavior

If the required Vault capability, authorization, unlock state, version contract, or security evidence is unavailable, the native integration must fail closed rather than filling from an unverified source.

## Acceptance

Implementation remains subject to the authoritative GoreeCloud Vault product specification, Browser feature roadmap, GoreeCloud Tasks Management, applicable Privacy Shield and Wardveil requirements, current Glaze UI acceptance, representative website and device testing, passkey/WebAuthn testing, accessibility, localization/RTL, update/rollback, release provenance, and production acceptance.

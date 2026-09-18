# GoreeCloud Browser — Local Health and Readiness Contract

**Status:** Development source contract  
**Contract ID:** `goreecloud.browser.health.v1`  
**Version:** 1  
**Network endpoint:** None  
**Production accepted:** No

## Purpose

This contract gives GoreeCloud Browser a small, privacy-safe operational state model that can later be consumed by an authorized GoreeCloud Manager or GoreeCloud Observability adapter without turning Browser history, URLs, queries, profile identity, credentials, or page content into monitoring payloads.

It deliberately separates:

- **liveness** — whether the Browser process/event loop can report its own state;
- **readiness** — whether core Browser interaction can currently operate;
- **dependency health** — bounded status for optional or required capabilities.

A healthy liveness result is not a production-readiness verdict. A ready local Browser is not a Stable qualification verdict.

## Core readiness

Core readiness requires the local Browser event loop, rendering engine, Browser-owned local state, and Browser UI to be available.

A required dependency that is unavailable, unknown, stale, or of unknown freshness blocks readiness. A required dependency that is degraded degrades readiness.

An optional dependency such as a nonessential service may degrade Browser readiness without making the entire Browser unavailable. Feature-specific actions must still obey their own fail-closed authority gates.

## Privacy boundary

The contract contains only:

- enumerated subsystem identity;
- enumerated health condition;
- enumerated evidence freshness;
- whether that subsystem is required for core readiness;
- enumerated aggregate reason codes.

It has no fields for:

- URLs or origins;
- page titles or page content;
- search queries;
- browsing history;
- form data;
- profile/account names;
- credentials, tokens, cookies, passkeys, or Vault material;
- raw Privacy Shield, Wardveil, Policy, Identity, or Observability evidence.

Future publication adapters must preserve this minimization boundary.

## Manager and Observability boundary

This source contract does **not** create an HTTP endpoint, IPC service, telemetry transport, Manager registration, Observability exporter, alert route, remote health authority, or production monitoring acceptance.

`goreecloud.platform.yaml` therefore keeps `health_endpoint` and `readiness_endpoint` as `null` for the current native application.

A future authorized adapter may publish this bounded contract through an approved local or service interface, but that adapter must be independently authenticated/authorized where applicable and accepted for the exact Browser release.

## Readiness is not release acceptance

This local contract must never be used as evidence that Browser is:

- production approved;
- secure or private in every workflow;
- fully integrated with all nine Integral Platform Systems;
- Release Candidate qualified;
- Stable.

Those remain separate release gates.

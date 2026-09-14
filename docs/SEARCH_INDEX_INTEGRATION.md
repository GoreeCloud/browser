# GoreeCloud Browser — Search and Index Integration

**Status:** Development contract  
**Scope:** Browser omnibox, web search delegation, Index handoff, and authority boundaries

## Product roles

- **GoreeCloud Browser** owns navigation, browser chrome, tabs, page lifecycle, permissions, history, downloads, and executable web-destination handling.
- **GoreeCloud Search** is the sole authoritative Internet/web/current-information search service for approved Browser search entry points.
- **GoreeCloud Index** is the universal/local federated discovery authority. Browser may invoke Index for device/application/content discovery, but Browser must not merge Index and Search into an ambiguous authority boundary.

## Omnibox classification

Browser must classify input before executing a remote search:

1. accepted navigable URL or explicit navigation intent → Browser navigation;
2. non-URL query → GoreeCloud Search delegation;
3. local/universal discovery intent routed to Index → Index handoff;
4. ambiguous or unsafe input → no silent execution until classification and applicable policy checks succeed.

The Browser must not implement a hidden fallback to a third-party search engine when GoreeCloud Search is unavailable.

## Search capability gate

Before a production Browser path delegates a query, Browser must validate one unambiguous Search capability record that is:

- capability `search.query`;
- contract-version compatible;
- current;
- authoritative;
- available;
- explicitly production accepted for Stable/production use;
- bound to the expected endpoint and result limits.

Development builds may use non-production Search evidence only through an explicit Development-only path that cannot be mistaken for Stable acceptance.

## Privacy Shield boundary

A non-URL query is not permission to transmit data. Before remote delegation, Browser must apply the accepted Privacy Shield decision for the specific operation and purpose.

Browser must not attach unrelated local state such as tab inventory, browsing history, downloads, bookmarks, cookies, local Index results, or account identifiers unless a separately specified and authorized feature requires the field.

## Result opening

Search owns search-result generation; Browser owns navigation to a selected result. Before navigation, Browser must validate the destination using Browser-owned URL/security policy. Search result presence is not permission to bypass Browser navigation restrictions.

## Index handoff

When Browser invokes GoreeCloud Index, the handoff is an invocation boundary rather than a transfer of authority. Index remains responsible for provider eligibility, local/remote composition, and source-aware results. Browser remains responsible only for Browser-owned presentation around the invocation and for web navigation after a validated `OpenWeb` action returns.

## Failure behavior

- Search unavailable/incompatible → do not silently switch engines.
- Privacy decision unavailable/denied → do not transmit query remotely.
- Index unavailable → retain ordinary Browser navigation/search behavior without fabricating local results.
- Degraded Search → preserve valid results only where the Search contract allows, while preserving degraded status.
- Invalid result URL → do not navigate.

## Glaze UI

All Browser-owned omnibox, search suggestions, result-opening affordances, Search error states, and Index invocation surfaces must conform to the latest approved Stable Glaze UI release. Search or Index conformance never substitutes for Browser-local Glaze acceptance.

## Stability boundary

Source wiring alone is not Stable evidence. Stable Browser acceptance requires current Search/Index contracts, Privacy Shield enforcement, Wardveil-relevant navigation/security behavior, accessibility, representative runtime validation, and Browser-local release evidence.

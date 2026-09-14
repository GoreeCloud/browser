# GoreeCloud Browser — Search and Index Integration

**Status:** Development contract  
**Scope:** Browser omnibox, web search delegation, Index handoff, and authority boundaries

## Product roles

- **GoreeCloud Browser** owns navigation, browser chrome, tabs, page lifecycle, permissions, history, downloads, and executable web-destination handling.
- **GoreeCloud Search** is the sole authoritative Internet/web/current-information search service for approved Browser search entry points.
- **GoreeCloud Index** is the universal/local federated discovery authority. Browser may invoke Index for device/application/content discovery, but Browser must not merge Index and Search into an ambiguous authority boundary.

## Omnibox classification

Browser must classify input before executing a remote search:

1. accepted navigable HTTP(S) URL or explicit navigation intent → Browser navigation;
2. non-URL query → local GoreeCloud Search intent only;
3. local/universal discovery intent routed to Index → Index handoff;
4. ambiguous, malformed, credential-bearing, unsupported-scheme, or otherwise unsafe input → blocked locally rather than silently executed or converted into a remote Search query.

The Browser must not implement a hidden fallback to a third-party search engine when GoreeCloud Search is unavailable.

The current Android Development shell performs typed Home / Navigate / Search / Blocked classification. Search classification retains only normalized query text; it no longer constructs a query-bearing `?q=` URL as an intermediate value. Search and Blocked inputs stop on Browser-owned local pages and are not transmitted remotely by the current runtime.

## Search capability gate

Before a production Browser path delegates a query, Browser must validate one unambiguous Search capability record that is:

- capability `search.query`;
- contract version `1`;
- current and authoritative;
- explicitly production accepted for Stable/production use;
- bound to `/api/v1/search`;
- POST-capable with POST identified as the preferred first-party method;
- bound to preferred query transport `json_body`;
- bound to `application/json` request and response media types;
- explicit that Privacy Shield authorization is required;
- bound to the accepted request-size and result-count limits.

Development builds may use non-production or legacy-GET Search evidence only through an explicit Development-only path that cannot be mistaken for Stable acceptance.

Production Browser must use the bounded JSON POST request contract so query text is not required to appear in request URLs. GET compatibility in Search is not permission for production Browser to silently downgrade transport privacy.

Browser now contains a transport-neutral `GoreeCloudSearchContract` boundary that independently validates the production capability plus a concrete Privacy Shield authorization reference before producing a POST request description. That object performs no network I/O by itself, so adding it does not enable remote Search prematurely.

## Privacy Shield boundary

A non-URL query is not permission to transmit data. Before remote delegation, Browser must apply the accepted Privacy Shield decision for the specific operation and purpose.

Browser must not attach unrelated local state such as tab inventory, browsing history, downloads, bookmarks, cookies, local Index results, or account identifiers unless a separately specified and authorized feature requires the field.

The current Android Development implementation deliberately fails closed because accepted runtime Privacy Shield evidence and Search capability discovery are not yet wired into a network transport. It does not fabricate approval from endpoint health or network availability.

## Result opening

Search owns search-result generation; Browser owns navigation to a selected result. Before navigation, Browser must validate the destination using Browser-owned URL/security policy. Search result presence is not permission to bypass Browser navigation restrictions.

Current Android navigation validation rejects malformed HTTP(S), missing-host URLs, embedded user-info credentials, and non-HTTP(S) explicit schemes before navigation.

## Index handoff

When Browser invokes GoreeCloud Index, the handoff is an invocation boundary rather than a transfer of authority. Index remains responsible for provider eligibility, local/remote composition, and source-aware results. Browser remains responsible only for Browser-owned presentation around the invocation and for web navigation after a validated `OpenWeb` action returns.

## Failure behavior

- Search unavailable/incompatible → do not silently switch engines.
- Privacy decision unavailable/denied → do not transmit query remotely.
- Search capability non-production, GET-only, query-URL, or media-type incompatible in a production path → do not transmit query remotely.
- Index unavailable → retain ordinary Browser navigation/search behavior without fabricating local results.
- Degraded Search → preserve valid results only where the Search contract allows, while preserving degraded status.
- Invalid result URL → do not navigate.

## Glaze UI

All Browser-owned omnibox, search suggestions, result-opening affordances, Search error states, and Index invocation surfaces must conform to the latest approved Stable Glaze UI release. Search or Index conformance never substitutes for Browser-local Glaze acceptance.

The Android source contract targets Glaze UI V1.4 / `1.4.0`, including V1.4 optical accessibility precedence and an effects-free opaque fallback. This remains source/automated evidence rather than rendered/native-device acceptance.

## Stability boundary

Source wiring alone is not Stable evidence. Stable Browser acceptance requires current Search/Index contracts, Privacy Shield enforcement, Wardveil-relevant navigation/security behavior, accessibility, representative runtime validation, and Browser-local release evidence.

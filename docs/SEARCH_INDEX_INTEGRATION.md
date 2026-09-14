# GoreeCloud Browser — Search and Index Integration

**Status:** Development contract  
**Scope:** Browser omnibox, web search delegation, Index handoff, and authority boundaries

## Product roles

- **GoreeCloud Browser** owns navigation, browser chrome, tabs, page lifecycle, permissions, history, downloads, and executable web-destination handling.
- **GoreeCloud Search** is the sole authoritative Internet/web/current-information search service for approved Browser search entry points.
- **GoreeCloud Index** is the universal/local federated discovery authority. Browser may invoke Index for device/application/content discovery, but Browser must not merge Index and Search into an ambiguous authority boundary.
- **Privacy Shield** remains authoritative for data-operation authorization, capability references, expiry/revocation/replay state, and privacy obligations.
- **GoreeCloud Identity** remains authoritative for authenticated Browser requester identity and service/requester authentication. Browser capability evidence may describe the required authentication contract but cannot manufacture a session, credential, or authenticated transport.

## Omnibox classification

Browser must classify input before executing a remote search:

1. accepted navigable HTTP(S) URL or explicit navigation intent → Browser navigation;
2. non-URL query → local GoreeCloud Search intent only;
3. local/universal discovery intent routed to Index → Index handoff;
4. ambiguous, malformed, credential-bearing, unsupported-scheme, or otherwise unsafe input → blocked locally rather than silently executed or converted into a remote Search query.

The Browser must not implement a hidden fallback to a third-party search engine when GoreeCloud Search is unavailable.

The current Android Development shell performs typed Home / Navigate / Search / Blocked classification. Search classification retains only normalized query text; it does not construct a query-bearing `?q=` URL as an intermediate value. Search and Blocked inputs stop on Browser-owned local pages and are not transmitted remotely by the current runtime.

## Search capability discovery and gate

Search capability discovery is expected from `/api/v1/status` under the `capability_evidence` collection. Browser must select exactly one `search.query` record. Missing or duplicate matching records fail closed.

Before a production Browser path delegates a query, the selected Search capability must be:

- capability `search.query`;
- contract version `1`;
- current and authoritative;
- explicitly production accepted for Stable/production use;
- bound to discovery endpoint `/api/v1/status` and collection `capability_evidence`;
- bound to query endpoint `/api/v1/search`;
- POST-capable with POST identified as the preferred first-party method;
- bound to preferred query transport `json_body`;
- bound to `application/json` request and response media types;
- explicit that Privacy Shield authorization is required;
- bound to authorization scheme `privacy_shield_capability_token_reference`;
- bound to authorization header `X-GoreeCloud-Privacy-Capability`;
- explicit that authorization enforcement is `required`;
- explicit that authenticated requester identity is required;
- bound to authenticated requester authority `goreecloud-identity`;
- bound to authenticated requester scheme `bearer`;
- bound to authenticated requester header `Authorization`;
- bound to the accepted request-size and result-count limits.

Development builds may use non-production or legacy-GET Search evidence only through an explicit Development-only path that cannot be mistaken for Stable acceptance.

The current Search Development service advertises `privacy_authorization_enforcement=not_enforced_development`. Browser therefore rejects that evidence for production delegation even if other fields are structurally compatible.

Production Browser must use the bounded JSON POST request contract so query text is not required to appear in request URLs. GET compatibility in Search is not permission for production Browser to silently downgrade transport privacy.

Browser contains a transport-neutral `GoreeCloudSearchContract` boundary that validates the production capability plus a canonical Privacy Shield capability reference before producing a POST request description. That object performs no network I/O by itself. Its capability gate now also requires the exact GoreeCloud Identity requester-authentication authority/scheme/header metadata above, but the object does not create an Identity session, mint or persist a bearer credential, add an `Authorization` value to a live request, or establish authenticated transport. Those runtime steps remain separate blocked work.

## Privacy Shield decision boundary

A non-URL query is not permission to transmit data. Before remote delegation, Browser must apply the accepted Privacy Shield decision for the specific operation and purpose.

Browser creates a unique Privacy Shield `request_id` before decision acquisition. The canonical Search authorization request identifies:

- requester `goreecloud-browser` with requester type `application`;
- resource `goreecloud.search.query` classified as `query_text`;
- operation `search.query`;
- purpose `internet_search`;
- processing zone `private_goreecloud`;
- destination `https://search.goreecloud.com`;
- retention mode `none`;
- `external_disclosure=false` for the Browser-to-first-party-Search boundary.

The Browser Search authorization adapter validates the canonical Privacy Shield decision response rather than trusting a generic boolean. The returned decision must echo the same `request_id`; authorization evidence created for another request is rejected even if every other field appears compatible.

For private GoreeCloud Search processing, Privacy Shield intentionally returns `ALLOW_WITH_CONSTRAINTS`. Browser accepts only that exact outcome and only the exact obligation set currently supported by the cross-product contract:

- `record_privacy_evidence`;
- `generate_privacy_receipt`;
- `enforce_processing_zone`.

The decision must also:

- permit operation `search.query`;
- permit processing zone `private_goreecloud`;
- permit destination `https://search.goreecloud.com`;
- permit retention mode `none`;
- contain exactly the supported obligation set above—no missing or additional obligation;
- remain unexpired when an expiry is supplied;
- provide a canonical non-empty `psc_*` capability reference.

An unconditional `ALLOW`, a different constrained outcome, or any altered obligation set fails closed. Browser does not weaken a constrained Privacy Shield decision into a generic allow.

Browser forwards only the opaque capability reference through the Search privacy-authorization header. The final Search request-construction boundary validates `psc_*` again, normalizes surrounding whitespace, and rejects embedded whitespace/noncanonical values even if an upstream caller bypasses the canonical decision adapter.

Browser must not attach unrelated local state such as tab inventory, browsing history, downloads, bookmarks, cookies, local Index results, or account identifiers unless a separately specified and authorized feature requires the field.

The current Android Development implementation deliberately remains non-transmitting because Search has not yet advertised server-side Privacy Shield authorization enforcement and authenticated requester identity as production-ready, and Browser has not established an accepted Identity-backed requester credential/transport path. It does not fabricate approval from endpoint health, capability metadata, or network availability.

## Search-side reference verification

Production Browser must not treat possession of a `psc_*` reference as proof that Search will enforce it. Search must advertise and prove server-side reference enforcement and authenticated requester identity.

Search's planned verifier uses Privacy Shield's version-1 capability-reference verification contract. Privacy Shield remains the authority for resolving the opaque reference, validating signed-token state, expiry, revocation, replay policy, requester/resource/purpose/operation/zone/destination/retention claims, and single-use consumption.

Browser does not receive Privacy Shield signing keys and does not send raw signed capability tokens to Search.

The verification consumer identity `goreecloud-search` and the original requester identity `goreecloud-browser` are separate concepts. Search must authenticate itself to Privacy Shield, and Search must independently derive the Browser requester identity from an authenticated GoreeCloud Identity-backed transport/runtime boundary rather than trusting an arbitrary Browser-supplied identity field.

## Result opening

Search owns search-result generation; Browser owns navigation to a selected result. Before navigation, Browser must validate the destination using Browser-owned URL/security policy. Search result presence is not permission to bypass Browser navigation restrictions.

Current Android navigation validation rejects malformed HTTP(S), missing-host URLs, embedded user-info credentials, and non-HTTP(S) explicit schemes before navigation.

## Index handoff

When Browser invokes GoreeCloud Index, the handoff is an invocation boundary rather than a transfer of authority. Index remains responsible for provider eligibility, local/remote composition, and source-aware results. Browser remains responsible only for Browser-owned presentation around the invocation and for web navigation after a validated `OpenWeb` action returns.

## Failure behavior

- Search unavailable/incompatible → do not silently switch engines.
- Search capability discovery missing/duplicated → do not transmit query remotely.
- Privacy decision unavailable, denied, wrong outcome, wrong obligation set, expired, mismatched to the request ID, or missing a canonical capability reference → do not transmit query remotely.
- Search capability non-production, GET-only, query-URL, authorization-unenforced, requester-authentication-unproven, requester authority/scheme/header incompatible, or media-type incompatible in a production path → do not transmit query remotely.
- Accepted requester-authentication metadata without an accepted runtime Identity session/credential/transport → do not transmit query remotely.
- Index unavailable → retain ordinary Browser navigation/search behavior without fabricating local results.
- Degraded Search → preserve valid results only where the Search contract allows, while preserving degraded status.
- Invalid result URL → do not navigate.

## Glaze UI

All Browser-owned omnibox, search suggestions, result-opening affordances, Search error states, and Index invocation surfaces must conform to the latest approved Stable Glaze UI release. Search or Index conformance never substitutes for Browser-local Glaze acceptance.

The Android source contract targets Glaze UI V1.4 / `1.4.0`, including V1.4 optical accessibility precedence and an effects-free opaque fallback. This remains source/automated evidence rather than rendered/native-device acceptance.

## Stability boundary

Source wiring alone is not Stable evidence. Stable Browser acceptance requires current Search/Index contracts, accepted Privacy Shield decision acquisition with request correlation, exact constrained-obligation handling, Search-side versioned capability-reference enforcement, accepted GoreeCloud Identity requester authentication and authenticated transport, Wardveil-relevant navigation/security behavior, accessibility, representative runtime validation, and Browser-local release evidence.

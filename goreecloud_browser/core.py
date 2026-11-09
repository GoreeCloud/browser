"""Pure, Qt-independent browser navigation and privacy helpers.

This module deliberately cannot execute scripts or access the network.
"""
from __future__ import annotations

from dataclasses import dataclass
from ipaddress import ip_address
from urllib.parse import quote_plus, unquote_plus, urlsplit, urlunsplit
import re

STARTPAGE_SEARCH = "https://www.startpage.com/sp/search?query="
INTERNAL_PAGES = {"goree://home": "home", "goree://new-tab": "new-tab"}
TRACKING_PARAMETERS = frozenset({
    "fbclid", "gclid", "dclid", "msclkid", "gbraid", "wbraid",
    "mc_cid", "mc_eid", "igshid", "yclid", "twclid", "ttclid",
    "vero_id", "_hsenc", "_hsmi", "mkt_tok",
})
# An intentionally small, inspectable starter list, not a full adblock engine.
TRACKER_DOMAINS = frozenset({
    "doubleclick.net", "google-analytics.com", "analytics.google.com",
    "googletagmanager.com", "connect.facebook.net", "facebook.net",
    "bat.bing.com", "analytics.tiktok.com",
})
SIGNED_PARAMETERS = frozenset({
    "signature", "sig", "oauth_signature", "x-amz-signature",
    "x-goog-signature", "x-ms-signature", "hmac",
})
SCHEME_RE = re.compile(r"^[a-zA-Z][a-zA-Z0-9+.-]*:")
CONTROL_RE = re.compile(r"[\x00-\x1f\x7f]")
BIDI_FORMAT_RE = re.compile("[\u200e\u200f\u202a-\u202e\u2066-\u2069]")
HOST_LABEL_RE = re.compile(r"^[a-zA-Z0-9_-]+$")
NUMERIC_IPV4_LABEL_RE = re.compile(r"(?:[0-9]+|0[xX][0-9a-fA-F]+)\Z")


@dataclass(frozen=True)
class Destination:
    kind: str  # "url", "home" or "new-tab"
    value: str


def is_ambiguous_numeric_host(host: str) -> bool:
    """Reject noncanonical numeric IP spelling that Qt may remap to another host.

    Chromium treats short, octal and hexadecimal IPv4 forms as numeric IPs.
    These are not DNS names just because ipaddress rejects their spelling.
    """
    candidate = host.rstrip(".")
    if not candidate:
        return False
    # IDNA maps fullwidth digits, hex letters and Unicode dot separators to
    # ASCII. Checking only the raw string missed e.g. "０１７７.0.0.1",
    # which Qt/Chromium navigates to 127.0.0.1.
    try:
        ascii_candidate = candidate.encode("idna").decode("ascii")
    except UnicodeError:
        return False
    if not all(
        NUMERIC_IPV4_LABEL_RE.fullmatch(label)
        for label in ascii_candidate.split(".")
    ):
        return False
    try:
        ip_address(ascii_candidate)
    except ValueError:
        return True
    # A non-ASCII spelling of even a canonical IPv4 is a different, easily
    # misleading visual address. Preserve only the canonical literal form.
    return ascii_candidate != candidate


def _safe_host(host: str) -> bool:
    """Reject ambiguous host encodings before Qt interprets an entered URL.

    For example, Qt decodes %2E within a host into a literal dot, which could
    point a typed address to a different origin than the entered text suggests.
    """
    if not host or "%" in host or is_ambiguous_numeric_host(host):
        return False
    try:
        ip_address(host)
        return True
    except ValueError:
        pass
    dns_host = host[:-1] if host.endswith(".") else host
    try:
        ascii_host = dns_host.encode("idna").decode("ascii")
    except UnicodeError:
        return False
    if not ascii_host or len(ascii_host) > 253:
        return False
    labels = ascii_host.split(".")
    return all(
        0 < len(label) <= 63
        and HOST_LABEL_RE.fullmatch(label)
        and not label.startswith("-") and not label.endswith("-")
        for label in labels
    )


def _host_looks_like_address(candidate: str) -> bool:
    """Decide whether bare text is a host, not a search query."""
    if any(c.isspace() for c in candidate) or not candidate:
        return False
    try:
        parts = urlsplit("//" + candidate)
        host = parts.hostname or ""
        _ = parts.port  # reject invalid ports
    except ValueError:
        return False
    if not host or parts.username is not None or parts.password is not None:
        return False
    if not _safe_host(host):
        return False
    if host.lower() == "localhost":
        return True
    try:
        ip_address(host)
        return True
    except ValueError:
        return "." in host and not host.endswith(".")


def valid_web_url(raw: str) -> bool:
    """Only allow explicit HTTP(S) URLs without credentials or control chars."""
    if (not raw or "\\" in raw or CONTROL_RE.search(raw)
            or BIDI_FORMAT_RE.search(raw) or any(c.isspace() for c in raw)):
        return False
    try:
        parsed = urlsplit(raw)
        _ = parsed.port
        return (
            parsed.scheme.lower() in {"http", "https"}
            and _safe_host(parsed.hostname or "")
            and parsed.username is None
            and parsed.password is None
        )
    except ValueError:
        return False


def destination_for(text: str, *, clean_links: bool = True) -> Destination:
    raw = text.strip()
    if not raw:
        return Destination("new-tab", "goree://new-tab")
    if raw.lower() in INTERNAL_PAGES:
        page = raw.lower()
        return Destination(INTERNAL_PAGES[page], page)
    if len(raw) > 4096 or "\\" in raw or CONTROL_RE.search(raw):
        raise ValueError("Address contains unsupported characters or is too long.")
    if not SCHEME_RE.match(raw):
        try:
            entered_host = urlsplit("//" + raw).hostname or ""
        except ValueError:
            entered_host = ""
        # A typed dotted numeric address must not become a search query or
        # navigate to a different canonical IP after Qt normalizes it.
        numeric_separator = any(char in entered_host for char in ".。．｡")
        if numeric_separator and is_ambiguous_numeric_host(entered_host):
            raise ValueError("Ambiguous numeric IP address; use canonical IPv4.")
    if _host_looks_like_address(raw) and not raw.lower().startswith(("http://", "https://")):
        # HTTP is allowed explicitly; bare hosts always begin with HTTPS.
        url = "https://" + raw
        return Destination("url", clean_tracking_parameters(url) if clean_links else url)
    if SCHEME_RE.match(raw):
        if not valid_web_url(raw):
            raise ValueError("Only HTTP and HTTPS web addresses are supported.")
        return Destination("url", clean_tracking_parameters(raw) if clean_links else raw)
    return Destination("url", STARTPAGE_SEARCH + quote_plus(raw))


def clean_tracking_parameters(url: str) -> str:
    """Strip known click identifiers; never rewrite signed or non-HTTP URLs.

    Preserve the raw bytes/encoding of every surviving query segment even
    when trackers are removed. Decoding and re-encoding a query can change the
    meaning of a signed URL or a server expecting literal '+' and '%20'.
    """
    if not valid_web_url(url):
        return url
    parsed = urlsplit(url)
    if not parsed.query:
        return url
    # Interpret names only; never parse or re-encode surviving query values.
    # Split on literal '&' to preserve encoded ampersands, duplicate keys,
    # empty segments and the original value encoding/order.
    segments = parsed.query.split("&")
    names = [unquote_plus(segment.partition("=")[0]).casefold()
             for segment in segments]
    if any(name in SIGNED_PARAMETERS or name.startswith(("x-amz-", "x-goog-"))
           for name in names):
        return url
    kept = [segment for segment, name in zip(segments, names)
            if not (name.startswith("utm_") or name in TRACKING_PARAMETERS)]
    if len(kept) == len(segments):
        return url
    return urlunsplit(parsed._replace(query="&".join(kept)))


def is_known_tracker(host: str, first_party_host: str) -> bool:
    """Block only well-known third-party tracking hosts; avoid main-page block."""
    host = host.lower().rstrip(".")
    first_party = first_party_host.lower().rstrip(".")
    if not host or host == first_party:
        return False
    return any(host == name or host.endswith("." + name) for name in TRACKER_DOMAINS)

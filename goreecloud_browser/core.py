"""Pure, Qt-independent browser navigation and privacy helpers.

This module deliberately cannot execute scripts or access the network.
"""
from __future__ import annotations

from dataclasses import dataclass
from ipaddress import ip_address
from urllib.parse import parse_qsl, quote_plus, urlencode, urlsplit, urlunsplit
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
HOST_LABEL_RE = re.compile(r"^[a-zA-Z0-9_-]+$")


@dataclass(frozen=True)
class Destination:
    kind: str  # "url", "home" or "new-tab"
    value: str


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
    if host.lower() == "localhost":
        return True
    try:
        ip_address(host)
        return True
    except ValueError:
        pass
    if "." not in host or host.startswith(".") or host.endswith("."):
        return False
    try:
        labels = host.encode("idna").decode("ascii").split(".")
    except UnicodeError:
        return False
    return all(0 < len(label) <= 63 and HOST_LABEL_RE.fullmatch(label) for label in labels)


def valid_web_url(raw: str) -> bool:
    """Only allow explicit HTTP(S) URLs without credentials or control chars."""
    if not raw or "\\" in raw or CONTROL_RE.search(raw) or any(c.isspace() for c in raw):
        return False
    try:
        parsed = urlsplit(raw)
        _ = parsed.port
        return (
            parsed.scheme.lower() in {"http", "https"}
            and bool(parsed.hostname)
            and parsed.username is None
            and parsed.password is None
        )
    except ValueError:
        return False


def destination_for(text: str) -> Destination:
    raw = text.strip()
    if not raw:
        return Destination("new-tab", "goree://new-tab")
    if raw.lower() in INTERNAL_PAGES:
        page = raw.lower()
        return Destination(INTERNAL_PAGES[page], page)
    if len(raw) > 4096 or "\\" in raw or CONTROL_RE.search(raw):
        raise ValueError("Address contains unsupported characters or is too long.")
    if _host_looks_like_address(raw) and not raw.lower().startswith(("http://", "https://")):
        # HTTP is allowed explicitly; bare hosts always begin with HTTPS.
        return Destination("url", clean_tracking_parameters("https://" + raw))
    if SCHEME_RE.match(raw):
        if not valid_web_url(raw):
            raise ValueError("Only HTTP and HTTPS web addresses are supported.")
        return Destination("url", clean_tracking_parameters(raw))
    return Destination("url", STARTPAGE_SEARCH + quote_plus(raw))


def clean_tracking_parameters(url: str) -> str:
    """Strip known click identifiers; never rewrite signed or non-HTTP URLs.

    We preserve original encoding when no change is required.
    """
    if not valid_web_url(url):
        return url
    parsed = urlsplit(url)
    if not parsed.query:
        return url
    pairs = parse_qsl(parsed.query, keep_blank_values=True)
    names = [name.casefold() for name, _ in pairs]
    if any(name in SIGNED_PARAMETERS or name.startswith(("x-amz-", "x-goog-"))
           for name in names):
        return url
    kept = [
        (name, value) for name, value in pairs
        if not (name.casefold().startswith("utm_")
                or name.casefold() in TRACKING_PARAMETERS)
    ]
    if len(kept) == len(pairs):
        return url
    return urlunsplit(parsed._replace(query=urlencode(kept, doseq=True)))


def is_known_tracker(host: str, first_party_host: str) -> bool:
    """Block only well-known third-party tracking hosts; avoid main-page block."""
    host = host.lower().rstrip(".")
    first_party = first_party_host.lower().rstrip(".")
    if not host or host == first_party:
        return False
    return any(host == name or host.endswith("." + name) for name in TRACKER_DOMAINS)

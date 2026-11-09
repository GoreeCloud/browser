"""Built-in domain-subscription filtering for GoreeCloud Browser.

Privacy: only the fixed upstream HTTPS list URLs are fetched. No browsing
history, website URLs, user identifiers, cookies, or tokens are transmitted.
The first launch may contact the two upstream list hosts to populate caches.
No HTML/CSS cosmetic filtering or malicious-file scanning is implied.
"""
from __future__ import annotations
import os
from pathlib import Path
from ipaddress import ip_address
import re
import tempfile
import time
from urllib.parse import urlsplit
from urllib.request import Request, build_opener, HTTPSHandler, HTTPRedirectHandler
import ssl

from goreecloud_browser.core import is_ambiguous_numeric_host


SOURCES = {
    "oisd-small": "https://small.oisd.nl/",
    "stevenblack": "https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts",
}
LIST_MAX_BYTES = 14 * 1024 * 1024
CACHE_AGE_SECONDS = 24 * 60 * 60
DOMAIN = re.compile(r"(?i)^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?(?:\.[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?)+$")
ADBLOCK = re.compile(r"^\|\|([^/^$*#]+)\^$")
HOSTS_ADDR = {"0.0.0.0", "127.0.0.1", "0", "::", "::1"}
MAX_ENTRIES = 300_000


def normalize_domain(value: str) -> str | None:
    host = value.strip().lower().rstrip(".")
    # Chromium's numeric IPv4 aliases (short, octal and hexadecimal) are
    # addresses, not DNS domains, even if ipaddress does not parse them.
    # Reuse the typed-address/Privacy Shield guard rather than maintaining a
    # looser exception for remote subscription entries.
    # The domain parser is on the hot path for large subscriptions.
    # Every numeric IPv4 alias begins with an ASCII digit; ordinary DNS
    # labels can skip the more expensive IDNA numeric check entirely.
    if (len(host) > 253 or not DOMAIN.fullmatch(host)
            or (host[0].isdigit() and is_ambiguous_numeric_host(host))):
        return None
    try:
        ip_address(host)
    except ValueError:
        return host
    return None


def parse_subscription(content: bytes, source: str) -> frozenset[str]:
    """Strict domain-only subset; ignore script, regex, cosmetic or allow rules."""
    if source not in SOURCES or len(content) > LIST_MAX_BYTES:
        raise ValueError("Unknown or oversized filter subscription")
    try:
        text = content.decode("utf-8-sig", "strict")
    except UnicodeError as exc:
        raise ValueError("Filter subscription must be UTF-8 text") from exc
    result = set()
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith(("!", "#", "[", "@@", "//")):
            continue
        candidate = None
        if source == "oisd-small":
            if (match := ADBLOCK.fullmatch(line)) is not None:
                candidate = match.group(1)
            elif DOMAIN.fullmatch(line):
                candidate = line
        else:
            fields = line.split()
            if len(fields) >= 2 and fields[0] in HOSTS_ADDR:
                for token in fields[1:]:
                    domain = normalize_domain(token)
                    if domain:
                        result.add(domain)
                        if len(result) > MAX_ENTRIES:
                            raise ValueError("Filter list exceeds safe domain cap")
            elif len(fields) == 1:
                candidate = fields[0]
        if candidate is not None:
            domain = normalize_domain(candidate)
            if domain:
                result.add(domain)
                if len(result) > MAX_ENTRIES:
                    raise ValueError("Filter list exceeds safe domain cap")
    return frozenset(result)


def is_blocked(host: str, rules: tuple[frozenset[str], ...]) -> bool:
    host = host.lower().strip(".")
    if not normalize_domain(host):
        return False
    if not rules:
        return False
    labels = host.split(".")
    return any(".".join(labels[n:]) in rule
               for n in range(max(0, len(labels) - 1))
               for rule in rules)


def cache_file(root: Path, name: str) -> Path:
    if name not in SOURCES:
        raise ValueError("Unknown filter list")
    return root / f"{name}.txt"


def read_cached(root: Path, name: str) -> frozenset[str]:
    target = cache_file(root, name)
    # Writing already refuses a symlinked cache root; reads must not bypass
    # the same trust boundary through an alias of the cache directory.
    if root.is_symlink() or target.is_symlink() or not target.is_file():
        return frozenset()
    try:
        if target.stat().st_size > LIST_MAX_BYTES:
            return frozenset()
        return parse_subscription(target.read_bytes(), name)
    except (OSError, ValueError):
        return frozenset()


def needs_update(
    root: Path, name: str, now: float | None = None, *,
    loaded_domains: frozenset[str] | None = None,
) -> bool:
    """Refresh stale, corrupt or unsafe caches without waiting a full day.

    The browser passes the rules already parsed at startup, avoiding a second
    full-list parse on the Qt GUI thread. Standalone callers validate on disk.
    """
    path = cache_file(root, name)
    if root.is_symlink() or path.is_symlink() or not path.is_file():
        return True
    try:
        stat = path.stat()
        current = time.time() if now is None else now
        if stat.st_size == 0 or stat.st_size > LIST_MAX_BYTES:
            return True
        # Future filesystem timestamps must not postpone privacy updates.
        if stat.st_mtime > current + 300:
            return True
        if current - stat.st_mtime >= CACHE_AGE_SECONDS:
            return True
        domains = loaded_domains if loaded_domains is not None else read_cached(root, name)
        return len(domains) < 500
    except (OSError, ValueError):
        return True


class SameOriginRedirects(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        old = urlsplit(req.full_url)
        new = urlsplit(newurl)
        if new.scheme != "https" or new.hostname != old.hostname or new.port not in (None, 443):
            raise ValueError("Refusing cross-origin filter subscription redirect")
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def retrieve_subscription(name: str) -> bytes:
    if name not in SOURCES:
        raise ValueError("Unknown filter list")
    request = Request(SOURCES[name], headers={"User-Agent": "GoreeCloudBrowser-FilterFetcher/0.1"})
    opener = build_opener(HTTPSHandler(context=ssl.create_default_context()), SameOriginRedirects())
    with opener.open(request, timeout=15) as response:
        if urlsplit(response.geturl()).scheme != "https":
            raise ValueError("Filter source lost HTTPS")
        result = response.read(LIST_MAX_BYTES + 1)
        if len(result) > LIST_MAX_BYTES:
            raise ValueError("Oversized filter list")
        return result


def save_validated_cache(root: Path, name: str, data: bytes) -> frozenset[str]:
    result = parse_subscription(data, name)
    # A failed host or captive portal must not replace a verified cached list.
    if len(result) < 500:
        raise ValueError("Filter source returned too few valid domains")
    if root.is_symlink():
        raise ValueError("Symlinked filter cache directory rejected")
    root.mkdir(mode=0o700, parents=True, exist_ok=True)
    target = cache_file(root, name)
    if target.is_symlink() or (target.exists() and not target.is_file()):
        raise ValueError("Unsafe filter cache path")
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=root, prefix=".filters-", delete=False) as stream:
            temporary = Path(stream.name)
            os.fchmod(stream.fileno(), 0o600)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        temporary.replace(target)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return result

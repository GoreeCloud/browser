"""Explicit exact-host Privacy Shield exceptions, stored only on-device.

An exception pauses GoreeCloud's built-in domain-based subresource blocking
when the *top-level first-party host* exactly matches. It does not grant site
permissions or change cookies. No wildcards, URL schemes, ports or suffix
matches are accepted; all other sites retain normal protection.
"""
from __future__ import annotations

import json

from goreecloud_browser.filters import normalize_domain

MAX_EXCEPTIONS = 64
MAX_JSON_CHARS = 8192
_BIDI = frozenset("\u200e\u200f\u202a\u202b\u202c\u202d\u202e\u2066\u2067\u2068\u2069")


def exact_host(value: str) -> str:
    if not isinstance(value, str):
        raise ValueError("Enter a website hostname")
    original = value.strip()
    if not original or len(original) > 253 or any(
        ord(c) < 33 or ord(c) == 127 or c in _BIDI for c in original
    ):
        raise ValueError("Enter a valid exact website hostname")
    try:
        ascii_hostname = original.encode("idna").decode("ascii").lower().rstrip(".")
    except UnicodeError as exc:
        raise ValueError("Invalid IDN hostname") from exc
    if not normalize_domain(ascii_hostname):
        raise ValueError("Only exact DNS hostnames are allowed (not URLs, IPs or wildcards)")
    return ascii_hostname


def decode_exceptions(data: str) -> tuple[str, ...]:
    if not isinstance(data, str) or len(data) > MAX_JSON_CHARS:
        return ()
    try:
        parsed = json.loads(data)
    except (TypeError, ValueError):
        return ()
    if not isinstance(parsed, list) or len(parsed) > MAX_EXCEPTIONS:
        return ()
    hosts = []
    for item in parsed:
        try:
            hostname = exact_host(item)
        except ValueError:
            continue
        if hostname not in hosts:
            hosts.append(hostname)
    return tuple(hosts)


def encode_exceptions(hosts: tuple[str, ...]) -> str:
    if len(hosts) > MAX_EXCEPTIONS:
        raise ValueError("Site-exception limit reached")
    validated = [exact_host(host) for host in hosts]
    if len(set(validated)) != len(validated):
        raise ValueError("Duplicate site-exception hostname")
    result = json.dumps(validated, separators=(",", ":"), ensure_ascii=True)
    if len(result) > MAX_JSON_CHARS:
        raise ValueError("Site-exception list exceeds storage limit")
    return result


def add_exception(hosts: tuple[str, ...], value: str) -> tuple[str, ...]:
    hostname = exact_host(value)
    if hostname in hosts:
        raise ValueError("This exact hostname is already an exception")
    if len(hosts) >= MAX_EXCEPTIONS:
        raise ValueError("Site-exception limit reached")
    return hosts + (hostname,)


def remove_exception(hosts: tuple[str, ...], value: str) -> tuple[str, ...]:
    hostname = exact_host(value)
    return tuple(host for host in hosts if host != hostname)


def is_exempt_first_party(host: str, exceptions: frozenset[str]) -> bool:
    if not exceptions:
        return False
    try:
        hostname = exact_host(host)
    except ValueError:
        return False
    return hostname in exceptions

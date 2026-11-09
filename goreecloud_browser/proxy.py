"""Strict, credential-free global Qt WebEngine proxy configuration.

The proxy is application-wide, never per-Webspace. A new value is applied
only on the next browser launch, before any WebEngine profile exists.
"""
from __future__ import annotations
from dataclasses import dataclass
from ipaddress import ip_address
import re

from goreecloud_browser.core import is_ambiguous_numeric_host

MODES = ("system", "direct", "http", "socks5")
HOST_LABEL = re.compile(r"^[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?$")


@dataclass(frozen=True)
class ProxyConfig:
    mode: str = "system"
    host: str = ""
    port: int = 0


def valid_proxy_host(value: str) -> str:
    value = value.strip()
    if not value or len(value) > 253 or any(
        ch.isspace() or ord(ch) < 33 or ord(ch) == 127 for ch in value
    ):
        raise ValueError("Proxy hostname is invalid")
    # Square brackets are legal only as one complete wrapper around a
    # numeric address. str.strip("[]") silently accepted partial brackets.
    if "[" in value or "]" in value:
        if not (
            value.startswith("[") and value.endswith("]")
            and value.count("[") == 1 and value.count("]") == 1
        ):
            raise ValueError("Proxy address brackets are malformed")
        try:
            ip_address(value[1:-1])
        except ValueError as exc:
            raise ValueError("Bracketed proxy address must be an IP") from exc
        return value[1:-1]
    if is_ambiguous_numeric_host(value):
        raise ValueError("Proxy IP must use canonical numeric notation")
    try:
        ip_address(value)
        return value
    except ValueError:
        pass
    try:
        ascii_name = value.encode("idna").decode("ascii")
    except UnicodeError as exc:
        raise ValueError("Proxy hostname is invalid") from exc
    if (not ascii_name or is_ambiguous_numeric_host(ascii_name)
            or not all(HOST_LABEL.fullmatch(x) for x in ascii_name.rstrip(".").split("."))):
        raise ValueError("Proxy hostname must use valid DNS labels")
    return ascii_name.lower().rstrip(".")


def validate_proxy(mode: str, host: str = "", port: int | str = 0) -> ProxyConfig:
    if mode not in MODES:
        raise ValueError("Unknown proxy mode")
    if mode in ("system", "direct"):
        return ProxyConfig(mode)
    validated = valid_proxy_host(host)
    if isinstance(port, bool) or not (isinstance(port, int) or
                                     (isinstance(port, str) and port.isascii() and port.isdecimal())):
        raise ValueError("Proxy port must be 1 to 65535")
    if isinstance(port, str) and len(port) > 5:
        raise ValueError("Proxy port must be 1 to 65535")
    port_number = int(port)
    if not (1 <= port_number <= 65535):
        raise ValueError("Proxy port must be 1 to 65535")
    return ProxyConfig(mode, validated, port_number)

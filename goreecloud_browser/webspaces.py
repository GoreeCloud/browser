"""Validated local Webspace identities for isolated Qt WebEngine profiles.

Webspaces have separate cookies/cache/HTML5 storage, not separate OS processes.
The default space retains the existing v0.1 profile paths for compatibility.
"""
from __future__ import annotations
import json
from pathlib import Path
import re
import secrets
import unicodedata

MAX_WEBSPACES = 16
_ID = re.compile(r"^[0-9a-f]{16}$")
DEFAULT_ID = "default"


def clean_label(value: str) -> str:
    """Canonical visible name for a security-sensitive, isolated Webspace.

    Reject invisible/bidi format controls rather than silently stripping
    them: changing a displayed identity could hide a misleading saved name.
    """
    if not isinstance(value, str):
        raise ValueError("Webspace name must be 1 to 36 visible characters")
    label = unicodedata.normalize("NFC", " ".join(value.split()).strip())
    if (not 1 <= len(label) <= 36
            or any(unicodedata.category(ch) in ("Cc", "Cf", "Cs")
                   or not ch.isprintable() for ch in label)):
        raise ValueError("Webspace name must be 1 to 36 visible characters")
    return label


def decode_spaces(value: str) -> list[dict[str, str]]:
    try:
        parsed = json.loads(value)
    except (ValueError, TypeError):
        return []
    if not isinstance(parsed, list):
        return []
    spaces = []
    seen = {DEFAULT_ID}
    for row in parsed[:MAX_WEBSPACES]:
        if not isinstance(row, dict):
            continue
        ident = row.get("id")
        name = row.get("name")
        if not isinstance(ident, str) or not _ID.fullmatch(ident) or ident in seen:
            continue
        try:
            label = clean_label(name)
        except ValueError:
            continue
        seen.add(ident)
        spaces.append({"id": ident, "name": label})
    return spaces


def encode_spaces(spaces: list[dict[str, str]]) -> str:
    if len(spaces) > MAX_WEBSPACES:
        raise ValueError("Webspace limit reached")
    ids = set()
    validated = []
    for entry in spaces:
        ident = entry["id"]
        if ident in ids or not _ID.fullmatch(ident):
            raise ValueError("Invalid or duplicate Webspace id")
        ids.add(ident)
        validated.append({"id": ident, "name": clean_label(entry["name"])})
    return json.dumps(validated, ensure_ascii=False)


def create_space(spaces: list[dict[str, str]], name: str) -> list[dict[str, str]]:
    label = clean_label(name)
    if len(spaces) >= MAX_WEBSPACES:
        raise ValueError("Webspace limit reached")
    if label.casefold() == "default" or any(row["name"].casefold() == label.casefold() for row in spaces):
        raise ValueError("Webspace name already exists")
    return spaces + [{"id": secrets.token_hex(8), "name": label}]


def storage_paths(root: Path, ident: str) -> tuple[Path, Path]:
    if ident == DEFAULT_ID:
        return root / "web-profile", root / "cache"
    if not _ID.fullmatch(ident):
        raise ValueError("Unsafe Webspace identifier")
    base = root / "spaces" / ident
    return base / "web-profile", base / "cache"

#!/usr/bin/env python3
"""Compare an official Mozilla stable version with the pinned GoreeCloud source.

This script alerts only; it never imports, merges or publishes code.
"""
import argparse
import json
import os
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
LOCK = ROOT / "goreecloud" / "upstream" / "BASELINE.json"
PRODUCT_DETAILS = "https://product-details.mozilla.org/1.0/firefox_versions.json"
VERSION = re.compile(r"^[0-9]{1,3}\.[0-9]{1,3}(?:\.[0-9]{1,3})?$")


def release_tuple(value: str) -> tuple[int, int, int]:
    if not isinstance(value, str) or not VERSION.fullmatch(value):
        raise ValueError(f"invalid stable version: {value!r}")
    parts = tuple(map(int, value.split(".")))
    return (parts + (0,) * 3)[:3]


def newest_from_payload(payload: dict) -> str:
    value = payload.get("LATEST_FIREFOX_VERSION")
    release_tuple(value)
    return value


def read_latest(url: str = PRODUCT_DETAILS) -> str:
    with urllib.request.urlopen(url, timeout=20) as response:
        payload = json.load(response)
    if not isinstance(payload, dict):
        raise ValueError("Mozilla release payload is not an object")
    return newest_from_payload(payload)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", help="GitHub Actions output file")
    parser.add_argument("--fixture-json", help="Local, offline fixture for testing")
    args = parser.parse_args()
    current = json.loads(LOCK.read_text(encoding="utf-8"))["version"]
    release_tuple(current)
    if args.fixture_json:
        latest = newest_from_payload(json.loads(Path(args.fixture_json).read_text(encoding="utf-8")))
    else:
        latest = read_latest()
    changed = release_tuple(latest) > release_tuple(current)
    output = f"current={current}\nlatest={latest}\nneeds_update={str(changed).lower()}\n"
    if args.output:
        with open(args.output, "a", encoding="utf-8") as handle:
            handle.write(output)
    print(output, end="")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Linux preview CI gate for audited Qt WebEngine/Chromium patch metadata.

Tests a *declared runtime baseline*, not CVE remediation or Chromium safety.
An upgrade to another Qt release must be independently reviewed, added here,
and then exercised by the existing native Linux browser acceptance suite.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import sys

try:
    import tomllib
except ModuleNotFoundError:
    tomllib = None


# Grounded in Qt's published 6.11.3 ChromiumVersions table (November 2026)
# and verified PySide6 6.11.3 on the authorized Zorin Linux preview.
# This table is a review checkpoint, NOT an always-current vulnerability feed.
REVIEWED_BASELINES = {
    "6.11.3": ("140.0.7339.264", "155.0.8059.85"),
}
UPSTREAM_REFERENCE = "https://wiki.qt.io/QtWebEngine/ChromiumVersions"


class EngineBaselineError(ValueError):
    """Unreviewed binding/runtime or unexpectedly older engine patch level."""


@dataclass(frozen=True)
class EngineVersions:
    binding: str
    webengine: str
    chromium_base: str
    chromium_security_patch: str


def version_tuple(value: str, count: int) -> tuple[int, ...]:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9]+(?:\.[0-9]+)*", value):
        raise EngineBaselineError(f"Invalid numeric version: {value!r}")
    parts = tuple(int(number) for number in value.split("."))
    if len(parts) != count:
        raise EngineBaselineError(f"Expected {count} version components: {value!r}")
    return parts


def pinned_binding(repo_root: Path) -> str:
    if tomllib is None:
        raise EngineBaselineError("Patch-baseline checker requires Python 3.11+")
    project = tomllib.loads((repo_root / "pyproject.toml").read_text("utf-8"))["project"]
    deps = project["dependencies"]
    if len(deps) != 1 or not isinstance(deps[0], str):
        raise EngineBaselineError("Unexpected direct dependency manifest")
    match = re.fullmatch(r"PySide6==([0-9]+\.[0-9]+\.[0-9]+)", deps[0])
    if not match:
        raise EngineBaselineError("PySide6 must be explicitly version-pinned")
    requirements = [
        line.strip() for line in (repo_root / "requirements.txt").read_text("utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]
    if requirements != [deps[0]]:
        raise EngineBaselineError("requirements.txt and pyproject.toml pins disagree")
    return match.group(1)


def validate_engine(pinned: str, actual: EngineVersions) -> None:
    version_tuple(pinned, 3)
    expected = REVIEWED_BASELINES.get(pinned)
    if expected is None:
        raise EngineBaselineError(
            f"Qt {pinned} has no reviewed Chromium baseline; audit before updating the pin"
        )
    if actual.binding != pinned or actual.webengine != pinned:
        raise EngineBaselineError(
            "Actual PySide6/Qt WebEngine runtime differs from the pinned dependency"
        )
    base, minimum_patch = expected
    if version_tuple(actual.chromium_base, 4) != version_tuple(base, 4):
        raise EngineBaselineError("Unexpected Qt Chromium source/base revision")
    if version_tuple(actual.chromium_security_patch, 4) < version_tuple(minimum_patch, 4):
        raise EngineBaselineError(
            f"Qt Chromium security-patch metadata regressed below {minimum_patch}"
        )


def actual_engine() -> EngineVersions:
    # These public Qt functions are read-only. Import only in the CI runtime
    # check; ordinary non-Qt unit discovery can import and test the helpers.
    from PySide6 import __version__ as pyside_version
    from PySide6.QtWebEngineCore import (
        qWebEngineVersion,
        qWebEngineChromiumVersion,
        qWebEngineChromiumSecurityPatchVersion,
    )

    return EngineVersions(
        binding=pyside_version,
        webengine=qWebEngineVersion(),
        chromium_base=qWebEngineChromiumVersion(),
        chromium_security_patch=qWebEngineChromiumSecurityPatchVersion(),
    )


def main() -> int:
    try:
        repo = Path(__file__).resolve().parents[2]
        pinned = pinned_binding(repo)
        engine = actual_engine()
        validate_engine(pinned, engine)
    except (EngineBaselineError, OSError, KeyError, ImportError) as exc:
        print(f"ENGINE_PATCH_BASELINE_FAILED: {exc}", file=sys.stderr)
        return 1
    print(
        "ENGINE_PATCH_BASELINE_PASS "
        f"PySide6={engine.binding} QtWebEngine={engine.webengine} "
        f"ChromiumBase={engine.chromium_base} "
        f"ChromiumSecurityPatch={engine.chromium_security_patch}"
    )
    print("NOTE: version metadata is not a complete security-patch/CVE audit.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Build a non-authorizing Privacy Shield runtime-evidence draft for GoreeCloud Browser."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
from pathlib import Path

REQUIRED_DIMENSIONS = (
    "content-blocking",
    "tracking-resistance",
    "url-cleaning",
    "privacy-status",
    "user-visible-exceptions",
    "private-browsing-isolation",
    "local-substitution",
    "failure-modes",
    "accessibility-status-accuracy",
    "engine-boundary",
)

PRIVACY_SHIELD_CANDIDATE = "privacy-shield-2.0.0-seal.2"\nPRIVACY_SHIELD_SOURCE = "9b938bb1ded71e8545da7abd7bedfdb0294857ef"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True).strip()


def cef_spec() -> dict:
    raw = subprocess.check_output(["python3", "scripts/bootstrap_cef_linux.py", "--print-spec"], text=True)
    return json.loads(raw)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True)
    parser.add_argument("--runtime-root", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    binary = Path(args.binary)
    runtime_root = Path(args.runtime_root)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    if not binary.is_file():
        raise SystemExit(f"compiled Browser binary missing: {binary}")

    source_revision = git("rev-parse", "HEAD")
    source_tree = git("rev-parse", "HEAD^{tree}")
    spec = cef_spec()
    payload_names = (
        "goreecloud-browser",
        "libcef.so",
        "icudtl.dat",
        "resources.pak",
        "chrome_100_percent.pak",
        "v8_context_snapshot.bin",
    )
    payload = []
    for name in payload_names:
        path = runtime_root / name
        if not path.is_file():
            raise SystemExit(f"CEF runtime payload member missing: {path}")
        payload.append({
            "name": name,
            "sha256": sha256_file(path),
            "size_bytes": path.stat().st_size,
        })

    dimensions = []
    pending = {
        "content-blocking",
        "tracking-resistance",
        "url-cleaning",
        "privacy-status",
        "user-visible-exceptions",
        "private-browsing-isolation",
        "local-substitution",
        "accessibility-status-accuracy",
    }
    for dimension in REQUIRED_DIMENSIONS:
        if dimension == "engine-boundary":
            status = "passed"
            note = "Pinned CEF/Chromium provenance, sandbox-preserving runtime launch, renderer-process observation, HTTPS load, windowless pixels/input/popup/keyboard/clipboard/IME smoke all pass on this exact source."
        elif dimension == "failure-modes":
            status = "partial"
            note = "CI rejects main-frame load errors, relative profile-cache regression, stack corruption, and unexpected runtime exit, but Privacy Shield-specific failure behavior is not yet fully exercised."
        elif dimension in pending:
            status = "pending"
            note = "No exact compiled-Browser runtime evidence has yet been collected for this Privacy Shield dimension."
        else:
            raise AssertionError(dimension)
        dimensions.append({"id": dimension, "status": status, "note": note})

    record = {
        "schema_version": "goreecloud.browser.privacy-shield-runtime-evidence-draft.v1",
        "browser_repository": "GoreeCloud/browser",
        "browser_source_revision": source_revision,
        "browser_source_tree_sha": source_tree,
        "privacy_shield_repository": "GoreeCloud/privacy-shield",
        "privacy_shield_candidate": PRIVACY_SHIELD_CANDIDATE,
        "privacy_shield_source_revision": PRIVACY_SHIELD_SOURCE,
        "github_run_id": os.environ.get("GITHUB_RUN_ID"),
        "github_run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "artifact": {
            "kind": "linux-binary",
            "name": binary.name,
            "sha256": sha256_file(binary),
            "size_bytes": binary.stat().st_size,
            "runtime_version": f"CEF {spec['cef_version']} / Chromium {spec['chromium_version']}",
            "runtime_payload": payload,
        },
        "target": {
            "platform": "linux",
            "os_name": platform.system(),
            "os_version": platform.release(),
            "device_class": "GitHub-hosted Ubuntu 22.04 x86_64 CI runner",
            "engine_family": "chromium-cef",
            "engine_version": spec["chromium_version"],
        },
        "machine_checks": [
            "sandboxed-cef-runtime-launch",
            "renderer-subprocess-observed",
            "https-main-frame-load-200",
            "windowless-frame-pixels",
            "custom-and-standard-cursor",
            "pointer-and-wheel-input",
            "context-menu",
            "popup-rendering",
            "keyboard-input",
            "clipboard-shortcuts",
            "gtk-ime-commit",
        ],
        "dimensions": dimensions,
        "collection_gap_count": sum(1 for item in dimensions if item["status"] != "passed"),
        "strict_privacy_shield_runtime_assessment_required": True,
        "accepted_for_runtime": False,
        "accepted_for_production": False,
        "authorization_effect": False,
        "authority_transfer": False,
    }

    (out / "privacy-shield-runtime-evidence-draft.json").write_text(
        json.dumps(record, indent=2) + "\n"
    )

    manifest = {
        "browser_source_revision": source_revision,
        "browser_source_tree_sha": source_tree,
        "compiled_browser_sha256": record["artifact"]["sha256"],
        "compiled_browser_size_bytes": record["artifact"]["size_bytes"],
        "runtime_payload": payload,
    }
    (out / "artifact-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    shutil.copy2(binary, out / binary.name)

    if record["accepted_for_runtime"] or record["accepted_for_production"]:
        raise SystemExit("draft evidence must remain non-authorizing")
    if record["collection_gap_count"] != 9:
        raise SystemExit("draft must retain nine non-passing Privacy Shield dimensions")
    if [x["id"] for x in dimensions if x["status"] == "passed"] != ["engine-boundary"]:
        raise SystemExit("only engine-boundary may be marked passed by this CI evidence layer")

    print(json.dumps({
        "browser_source_revision": source_revision,
        "compiled_browser_sha256": record["artifact"]["sha256"],
        "collection_gap_count": record["collection_gap_count"],
        "accepted_for_runtime": record["accepted_for_runtime"],
    }, indent=2))


if __name__ == "__main__":
    main()

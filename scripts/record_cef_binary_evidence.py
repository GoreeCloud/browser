#!/usr/bin/env python3
"""Record exact GoreeCloud Browser CEF binary evidence without granting acceptance."""

from __future__ import annotations

import argparse
import hashlib
import json
import runpy
import shutil
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_value(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True).strip()


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise SystemExit(f"Missing {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--installed-runtime", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    build_browser = args.build_dir / "goreecloud-browser"
    build_subprocess = args.build_dir / "goreecloud-browser-subprocess"
    installed_browser = args.installed_runtime / "goreecloud-browser"
    installed_subprocess = args.installed_runtime / "goreecloud-browser-subprocess"

    for path, label in (
        (build_browser, "build Browser binary"),
        (build_subprocess, "build Browser subprocess"),
        (installed_browser, "installed Browser binary"),
        (installed_subprocess, "installed Browser subprocess"),
    ):
        require_file(path, label)

    browser_digest = sha256(build_browser)
    subprocess_digest = sha256(build_subprocess)
    if sha256(installed_browser) != browser_digest:
        raise SystemExit("Installed Browser binary does not match the tested build binary")
    if sha256(installed_subprocess) != subprocess_digest:
        raise SystemExit("Installed Browser subprocess does not match the tested build subprocess")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    retained_browser = args.output_dir / "goreecloud-browser"
    retained_subprocess = args.output_dir / "goreecloud-browser-subprocess"
    shutil.copy2(build_browser, retained_browser)
    shutil.copy2(build_subprocess, retained_subprocess)

    cef = runpy.run_path("scripts/bootstrap_cef_linux.py")
    manifest = {
        "schema_version": 1,
        "repository": "GoreeCloud/browser",
        "source_revision": git_value("rev-parse", "HEAD"),
        "source_tree": git_value("rev-parse", "HEAD^{tree}"),
        "browser_binary": {
            "name": retained_browser.name,
            "sha256": browser_digest,
            "size": retained_browser.stat().st_size,
        },
        "subprocess_binary": {
            "name": retained_subprocess.name,
            "sha256": subprocess_digest,
            "size": retained_subprocess.stat().st_size,
        },
        "runtime": {
            "cef_version": cef["CEF_VERSION"],
            "chromium_version": cef["CHROMIUM_VERSION"],
            "cef_archive_sha256": cef["CEF_ARCHIVE_SHA256"],
        },
        "installed_runtime_byte_match": True,
        "evidence_role": "development-supporting-only",
        "privacy_shield_fr013_accepted": False,
        "production_acceptance": False,
    }

    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (args.output_dir / "SHA256SUMS").write_text(
        f"{browser_digest}  {retained_browser.name}\n"
        f"{subprocess_digest}  {retained_subprocess.name}\n",
        encoding="utf-8",
    )
    print(json.dumps(manifest, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

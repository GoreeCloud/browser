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


def first_existing_file(candidates: tuple[Path, ...], label: str) -> Path:
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    joined = ", ".join(str(candidate) for candidate in candidates)
    raise SystemExit(f"Missing {label}; checked: {joined}")


def retain_binary(source: Path, destination: Path) -> dict[str, object]:
    shutil.copy2(source, destination)
    return {
        "name": destination.name,
        "source_path": str(source),
        "sha256": sha256(destination),
        "size": destination.stat().st_size,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--installed-runtime", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    build_browser = args.build_dir / "goreecloud-browser"
    build_subprocess = first_existing_file(
        (
            args.build_dir / "goreecloud-browser-subprocess",
            args.build_dir / "Release" / "goreecloud-browser-subprocess",
        ),
        "build Browser subprocess",
    )
    installed_browser = args.installed_runtime / "goreecloud-browser"
    installed_subprocess = args.installed_runtime / "goreecloud-browser-subprocess"

    for path, label in (
        (build_browser, "build Browser binary"),
        (build_subprocess, "build Browser subprocess"),
        (installed_browser, "installed Browser binary"),
        (installed_subprocess, "installed Browser subprocess"),
    ):
        require_file(path, label)

    args.output_dir.mkdir(parents=True, exist_ok=True)

    retained = {
        "build_browser": retain_binary(
            build_browser, args.output_dir / "build-goreecloud-browser"
        ),
        "build_subprocess": retain_binary(
            build_subprocess, args.output_dir / "build-goreecloud-browser-subprocess"
        ),
        "installed_browser": retain_binary(
            installed_browser, args.output_dir / "installed-goreecloud-browser"
        ),
        "installed_subprocess": retain_binary(
            installed_subprocess,
            args.output_dir / "installed-goreecloud-browser-subprocess",
        ),
    }

    browser_byte_match = (
        retained["build_browser"]["sha256"] == retained["installed_browser"]["sha256"]
    )
    subprocess_byte_match = (
        retained["build_subprocess"]["sha256"]
        == retained["installed_subprocess"]["sha256"]
    )

    cef = runpy.run_path("scripts/bootstrap_cef_linux.py")
    manifest = {
        "schema_version": 2,
        "repository": "GoreeCloud/browser",
        "source_revision": git_value("rev-parse", "HEAD"),
        "source_tree": git_value("rev-parse", "HEAD^{tree}"),
        "build_runtime": {
            "browser_binary": retained["build_browser"],
            "subprocess_binary": retained["build_subprocess"],
        },
        "installed_runtime": {
            "browser_binary": retained["installed_browser"],
            "subprocess_binary": retained["installed_subprocess"],
        },
        "installation_comparison": {
            "browser_byte_identical": browser_byte_match,
            "subprocess_byte_identical": subprocess_byte_match,
            "note": (
                "CMake installation may rewrite ELF runtime-path metadata. "
                "Both exact build-tree and staged-install binaries are retained "
                "independently so evidence remains truthful when installation "
                "changes binary bytes."
            ),
        },
        "runtime": {
            "cef_version": cef["CEF_VERSION"],
            "chromium_version": cef["CHROMIUM_VERSION"],
            "cef_archive_sha256": cef["CEF_ARCHIVE_SHA256"],
        },
        "evidence_role": "development-supporting-only",
        "privacy_shield_fr013_accepted": False,
        "production_acceptance": False,
    }

    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    checksum_lines = []
    for key in (
        "build_browser",
        "build_subprocess",
        "installed_browser",
        "installed_subprocess",
    ):
        entry = retained[key]
        checksum_lines.append(f'{entry["sha256"]}  {entry["name"]}')
    (args.output_dir / "SHA256SUMS").write_text(
        "\n".join(checksum_lines) + "\n", encoding="utf-8"
    )

    print(json.dumps(manifest, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

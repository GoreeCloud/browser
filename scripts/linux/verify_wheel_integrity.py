#!/usr/bin/env python3
"""Validate an unpublished GoreeCloud Browser wheel against the source tree.

This is an integrity, contents and license-notice acceptance gate, not a
signature, SBOM, malware scanner or independent provenance attestation.
Requires Python 3.11+ (the CI packaging runner uses 3.12).
"""
from __future__ import annotations

import argparse
import base64
import csv
from email import policy
from email.parser import BytesParser
import hashlib
import hmac
from io import StringIO
from pathlib import Path
import re
import stat
import sys
import zipfile

try:
    import tomllib  # Python 3.11+; a build-time tool, not browser runtime code.
except ModuleNotFoundError:
    tomllib = None


class WheelIntegrityError(ValueError):
    """A distribution file failed the local release-integrity contract."""


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise WheelIntegrityError(reason)


def validate_path(name: str, info: zipfile.ZipInfo) -> None:
    parts = name.split("/")
    require(
        name and not name.startswith("/") and "\\" not in name and "\x00" not in name
        and all(part not in ("", ".", "..") for part in parts),
        f"Illegal wheel member path: {name!r}",
    )
    require(not info.is_dir(), f"Unexpected directory entry: {name}")
    mode = (info.external_attr >> 16) & 0xFFFF
    require(stat.S_IFMT(mode) != stat.S_IFLNK, f"Symlink in wheel: {name}")


def recorded_files(record_bytes: bytes, archive: dict[str, bytes],
                   record_path: str) -> None:
    try:
        entries = list(csv.reader(StringIO(record_bytes.decode("utf-8"))))
    except (UnicodeError, csv.Error) as exc:
        raise WheelIntegrityError("Invalid wheel RECORD encoding") from exc
    require(len(entries) == len(archive), "RECORD count does not match wheel members")
    seen: set[str] = set()
    for row in entries:
        require(len(row) == 3, "RECORD must contain three CSV fields")
        name, hashvalue, size = row
        require(name not in seen, f"Duplicate RECORD entry: {name}")
        seen.add(name)
        require(name in archive, f"RECORD references missing file: {name}")
        if name == record_path:
            require(not hashvalue and not size, "RECORD must omit its own hash and size")
            continue
        require(hashvalue.startswith("sha256="), f"Missing SHA-256 RECORD hash: {name}")
        encoded = hashvalue.partition("=")[2]
        require(bool(re.fullmatch(r"[A-Za-z0-9_-]{43}", encoded)),
                f"Malformed SHA-256 RECORD digest: {name}")
        try:
            actual_digest = base64.urlsafe_b64decode(encoded + "=")
        except (ValueError, base64.binascii.Error) as exc:
            raise WheelIntegrityError(f"Invalid RECORD digest for {name}") from exc
        require(hmac.compare_digest(actual_digest, hashlib.sha256(archive[name]).digest()),
                f"RECORD hash mismatch: {name}")
        require(size.isdecimal() and int(size) == len(archive[name]),
                f"RECORD size mismatch: {name}")
    require(seen == set(archive), "RECORD member set differs from wheel")


def verify_wheel(wheel: Path, source_root: Path) -> tuple[int, str]:
    require(tomllib is not None, "Wheel checker requires Python 3.11+")
    root = source_root.resolve(strict=True)
    config = tomllib.loads((root / "pyproject.toml").read_text(encoding="utf-8"))
    project = config["project"]
    package = project["name"].lower().replace("-", "_")
    version = project["version"]
    expected_filename = f"{package}-{version}-py3-none-any.whl"
    require(wheel.name == expected_filename,
            f"Unexpected wheel identity: expected {expected_filename}")
    dist = f"{package}-{version}.dist-info/"
    record = dist + "RECORD"
    license_files = project.get("license-files")
    require(license_files == ["LICENSE", "NOTICE", "docs/THIRD_PARTY.md"],
            "Explicit LICENSE, NOTICE, third-party provenance files required")

    package_root = root / package
    source_files = {}
    for p in package_root.rglob("*"):
        if p.is_symlink():
            raise WheelIntegrityError(f"Source package contains symlink: {p}")
        if p.is_file() and p.suffix in (".py", ".png", ".svg"):
            source_files[p.relative_to(root).as_posix()] = p.read_bytes()
    require(bool(source_files), "No first-party source files found")
    expected_licenses = {}
    for name in license_files:
        path = root / name
        require(not path.is_symlink(), f"License/notice is a symlink: {name}")
        expected_licenses[dist + "licenses/" + name] = path.read_bytes()
    required_meta = {"METADATA", "WHEEL", "entry_points.txt",
                     "top_level.txt", "RECORD"}
    allowed = set(source_files) | set(expected_licenses) | {
        dist + name for name in required_meta
    }

    try:
        with zipfile.ZipFile(wheel) as z:
            members = z.infolist()
            require(len(members) <= 2000, "Too many wheel members")
            require(sum(info.file_size for info in members) <= 64 * 1024 * 1024,
                    "Wheel uncompressed content exceeds verification budget")
            archive: dict[str, bytes] = {}
            for entry in members:
                validate_path(entry.filename, entry)
                require(entry.file_size <= 16 * 1024 * 1024,
                        f"Oversize wheel member: {entry.filename}")
                require(entry.filename not in archive,
                        f"Duplicate ZIP entry: {entry.filename}")
                archive[entry.filename] = z.read(entry)
    except (zipfile.BadZipFile, OSError, RuntimeError) as exc:
        raise WheelIntegrityError("Invalid or unreadable wheel archive") from exc

    require(set(archive) == allowed,
            f"Unexpected or missing packaged files: "
            f"missing={sorted(allowed - set(archive))}, "
            f"unexpected={sorted(set(archive) - allowed)}")
    recorded_files(archive[record], archive, record)

    for name, contents in (source_files | expected_licenses).items():
        require(hmac.compare_digest(archive[name], contents),
                f"Packaged content differs from source: {name}")

    metadata = BytesParser(policy=policy.default).parsebytes(archive[dist + "METADATA"])
    require(metadata.get("Name", "").lower() == project["name"].lower(),
            "METADATA project name mismatch")
    require(metadata.get("Version") == version, "METADATA version mismatch")
    require(metadata.get("License-Expression") == project["license"],
            "License SPDX expression mismatch")
    require(metadata.get("Requires-Python") == project["requires-python"],
            "Requires-Python contract mismatch")
    require(set(metadata.get_all("License-File", [])) == set(license_files),
            "Declared license and third-party notice paths mismatch")
    require(set(metadata.get_all("Requires-Dist", [])) == set(project["dependencies"]),
            "Pinned runtime dependencies mismatch")

    wheel_meta = BytesParser(policy=policy.default).parsebytes(archive[dist + "WHEEL"])
    require("py3-none-any" in wheel_meta.get_all("Tag", []),
            "Wheel must be a pure-Python py3-none-any distribution")
    script = f"{package.replace('_', '-')} = {package}.app:main"
    require(script in archive[dist + "entry_points.txt"].decode("utf-8"),
            "Console entry point does not match native Qt app")
    require(archive[dist + "top_level.txt"].strip() == package.encode(),
            "Wheel exposes unexpected top-level packages")
    return len(archive), hashlib.sha256(wheel.read_bytes()).hexdigest()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wheel", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path("."))
    options = parser.parse_args(argv)
    try:
        n, digest = verify_wheel(options.wheel, options.source_root)
    except (OSError, KeyError, ValueError, zipfile.BadZipFile) as exc:
        print(f"WHEEL_INTEGRITY_FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"WHEEL_INTEGRITY_PASS files={n} sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

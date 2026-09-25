#!/usr/bin/env python3
"""Fetch and verify the pinned Linux CEF binary distribution for GoreeCloud Browser."""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import re
import shutil
import sys
import tarfile
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath

CEF_VERSION = "152.0.6+g708dc14+chromium-152.0.7977.83"
CHROMIUM_VERSION = "152.0.7977.83"
CEF_PLATFORM = "linux64"
CEF_DISTRIBUTION_KIND = "minimal"
CEF_DISTRIBUTION = f"cef_binary_{CEF_VERSION}_{CEF_PLATFORM}_{CEF_DISTRIBUTION_KIND}"
CEF_ARCHIVE = f"{CEF_DISTRIBUTION}.tar.bz2"
CEF_DOWNLOAD_BASE = "https://cef-builds.spotifycdn.com"
CEF_ARCHIVE_SHA1 = "9711b86c105fb590da576fe5a829802f1a79d520"
CEF_ARCHIVE_SHA256 = "daf8c2b6e63787d6a91d666205a8a4521419937eabaf47723738c86aea7135bd"
USER_AGENT = "GoreeCloud-Browser-CEF-Bootstrap/0.1"
PROVENANCE_FILE = ".goreecloud-cef-provenance.json"
_REQUIRED_FILES = (
    "include/cef_app.h",
    "include/cef_version.h",
    "cmake/FindCEF.cmake",
    "Release/libcef.so",
    "Resources/icudtl.dat",
)


class BootstrapError(RuntimeError):
    pass


def _log(message: str) -> None:
    print(f"[GoreeCloud CEF] {message}", file=sys.stderr, flush=True)


def _official_url(filename: str) -> str:
    encoded = urllib.parse.quote(filename, safe="._-")
    return f"{CEF_DOWNLOAD_BASE}/{encoded}"


def _sha1(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _request(url: str):
    return urllib.request.Request(url, headers={"User-Agent": USER_AGENT})


def _fetch_official_sha1() -> str:
    url = _official_url(f"{CEF_ARCHIVE}.sha1")
    _log(f"Fetching official checksum: {url}")
    try:
        with urllib.request.urlopen(_request(url), timeout=30) as response:
            value = response.read(256).decode("ascii", errors="strict").strip().split()[0]
    except Exception as exc:
        raise BootstrapError(f"Unable to fetch official CEF checksum: {exc}") from exc
    if not re.fullmatch(r"[0-9a-fA-F]{40}", value):
        raise BootstrapError("Official CEF checksum response is not a valid SHA-1 digest")
    value = value.lower()
    if value != CEF_ARCHIVE_SHA1:
        raise BootstrapError(
            "Official CEF checksum no longer matches the source-pinned minimal archive "
            f"SHA-1: expected {CEF_ARCHIVE_SHA1}, got {value}. "
            "A reviewed dependency update is required."
        )
    return value


def _download_archive(destination: Path, expected_sha1: str) -> str:
    url = _official_url(CEF_ARCHIVE)
    destination.parent.mkdir(parents=True, exist_ok=True)

    if destination.exists():
        current_sha1 = _sha1(destination)
        current_sha256 = _sha256(destination)
        if (
            current_sha1 == expected_sha1 == CEF_ARCHIVE_SHA1
            and current_sha256 == CEF_ARCHIVE_SHA256
        ):
            _log(f"Using source-pinned verified cached archive: {destination}")
            return current_sha256
        _log("Cached archive hash mismatch; replacing the invalid cache entry.")
        destination.unlink()

    partial = destination.with_suffix(destination.suffix + ".part")
    if partial.exists():
        partial.unlink()

    _log(f"Downloading pinned CEF archive: {url}")
    digest1 = hashlib.sha1()
    digest256 = hashlib.sha256()
    try:
        with urllib.request.urlopen(_request(url), timeout=60) as response, partial.open("wb") as out:
            total = int(response.headers.get("Content-Length", "0") or "0")
            copied = 0
            while True:
                chunk = response.read(1024 * 1024)
                if not chunk:
                    break
                out.write(chunk)
                digest1.update(chunk)
                digest256.update(chunk)
                copied += len(chunk)
                if total and copied % (32 * 1024 * 1024) < len(chunk):
                    _log(
                        f"Downloaded {copied // (1024 * 1024)} / "
                        f"{total // (1024 * 1024)} MiB"
                    )
    except Exception as exc:
        partial.unlink(missing_ok=True)
        raise BootstrapError(f"Unable to download pinned CEF archive: {exc}") from exc

    actual_sha1 = digest1.hexdigest()
    actual_sha256 = digest256.hexdigest()
    if actual_sha1 != expected_sha1 or actual_sha1 != CEF_ARCHIVE_SHA1:
        partial.unlink(missing_ok=True)
        raise BootstrapError(
            f"CEF archive SHA-1 mismatch: expected {CEF_ARCHIVE_SHA1}, got {actual_sha1}"
        )
    if actual_sha256 != CEF_ARCHIVE_SHA256:
        partial.unlink(missing_ok=True)
        raise BootstrapError(
            f"CEF archive SHA-256 mismatch: expected {CEF_ARCHIVE_SHA256}, got {actual_sha256}"
        )
    partial.replace(destination)
    _log("CEF archive verified against the source-pinned SHA-1 and SHA-256 identities.")
    return actual_sha256


def _validate_member(member: tarfile.TarInfo) -> None:
    name = PurePosixPath(member.name)
    if name.is_absolute() or ".." in name.parts:
        raise BootstrapError(f"Unsafe path in CEF archive: {member.name}")
    if member.ischr() or member.isblk() or member.isfifo():
        raise BootstrapError(f"Unsupported special file in CEF archive: {member.name}")
    if member.issym() or member.islnk():
        target = PurePosixPath(member.linkname)
        if target.is_absolute() or ".." in target.parts:
            raise BootstrapError(
                f"Unsafe link target in CEF archive: {member.name} -> {member.linkname}"
            )


def _validate_root(root: Path) -> None:
    missing = [item for item in _REQUIRED_FILES if not (root / item).exists()]
    if missing:
        raise BootstrapError(
            f"CEF root {root} is incomplete; missing: {', '.join(missing)}"
        )


def _read_provenance(root: Path) -> dict | None:
    path = root / PROVENANCE_FILE
    if not path.is_file():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def _cached_root_is_accepted(root: Path) -> bool:
    if not root.is_dir():
        return False
    try:
        _validate_root(root)
    except BootstrapError:
        return False
    provenance = _read_provenance(root)
    if not provenance:
        return False
    return (
        provenance.get("cef_version") == CEF_VERSION
        and provenance.get("chromium_version") == CHROMIUM_VERSION
        and provenance.get("platform") == CEF_PLATFORM
        and provenance.get("distribution") == CEF_DISTRIBUTION_KIND
        and provenance.get("official_sha1") == CEF_ARCHIVE_SHA1
        and provenance.get("archive_sha256") == CEF_ARCHIVE_SHA256
    )


def _extract_archive(archive: Path, cache_dir: Path, root: Path, *, refresh: bool) -> None:
    if root.exists():
        if not refresh:
            raise BootstrapError(
                f"Existing CEF root is not accepted: {root}. "
                "Re-run with --refresh to replace this known cache directory."
            )
        _log(f"Removing stale pinned CEF cache root: {root}")
        shutil.rmtree(root)

    _log(f"Extracting {archive.name}")
    try:
        with tarfile.open(archive, mode="r:bz2") as bundle:
            members = bundle.getmembers()
            for member in members:
                _validate_member(member)
            bundle.extractall(cache_dir, members=members)
    except (OSError, tarfile.TarError) as exc:
        if root.exists():
            shutil.rmtree(root)
        raise BootstrapError(f"Unable to extract CEF archive: {exc}") from exc

    _validate_root(root)


def _write_provenance(
    root: Path, archive: Path, official_sha1: str, archive_sha256: str
) -> None:
    record = {
        "schema": 1,
        "product": "GoreeCloud Browser",
        "cef_version": CEF_VERSION,
        "chromium_version": CHROMIUM_VERSION,
        "platform": CEF_PLATFORM,
        "distribution": CEF_DISTRIBUTION_KIND,
        "archive": CEF_ARCHIVE,
        "archive_url": _official_url(CEF_ARCHIVE),
        "checksum_url": _official_url(f"{CEF_ARCHIVE}.sha1"),
        "official_sha1": official_sha1,
        "archive_sha256": archive_sha256,
        "verified_at_utc": datetime.now(timezone.utc).isoformat(),
        "archive_path": str(archive.resolve()),
    }
    (root / PROVENANCE_FILE).write_text(
        json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def _ensure_host_supported() -> None:
    if sys.platform != "linux":
        raise BootstrapError("This bootstrap currently supports Linux only")
    machine = platform.machine().lower()
    if machine not in {"x86_64", "amd64"}:
        raise BootstrapError(
            f"Pinned CEF platform {CEF_PLATFORM} requires x86_64 Linux; detected {machine}"
        )


def _spec() -> dict:
    return {
        "cef_version": CEF_VERSION,
        "chromium_version": CHROMIUM_VERSION,
        "platform": CEF_PLATFORM,
        "distribution": CEF_DISTRIBUTION_KIND,
        "archive": CEF_ARCHIVE,
        "archive_url": _official_url(CEF_ARCHIVE),
        "checksum_url": _official_url(f"{CEF_ARCHIVE}.sha1"),
        "archive_sha1": CEF_ARCHIVE_SHA1,
        "archive_sha256": CEF_ARCHIVE_SHA256,
    }


def bootstrap(cache_dir: Path, *, refresh: bool = False) -> Path:
    _ensure_host_supported()
    cache_dir = cache_dir.expanduser().resolve()
    root = cache_dir / CEF_DISTRIBUTION
    archive = cache_dir / CEF_ARCHIVE

    if _cached_root_is_accepted(root) and not refresh:
        _log(f"Using accepted pinned CEF root: {root}")
        return root

    official_sha1 = _fetch_official_sha1()
    _log(f"Official CEF archive SHA-1: {official_sha1}")
    archive_sha256 = _download_archive(archive, official_sha1)
    _log(f"Observed CEF archive SHA-256: {archive_sha256}")
    _extract_archive(archive, cache_dir, root, refresh=refresh)
    _write_provenance(root, archive, official_sha1, archive_sha256)
    _log(f"Pinned CEF runtime prepared: {root}")
    return root


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare the pinned GoreeCloud Browser CEF Linux distribution."
    )
    parser.add_argument(
        "--cache-dir",
        type=Path,
        default=Path(".cache/cef"),
        help="Cache/extraction directory (default: .cache/cef)",
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Replace the known pinned extracted cache root after re-verification.",
    )
    parser.add_argument(
        "--print-spec",
        action="store_true",
        help="Print the pinned source specification without network access.",
    )
    parser.add_argument(
        "--print-root",
        action="store_true",
        help="Print only the accepted extracted CEF root on stdout.",
    )
    args = parser.parse_args()

    if args.print_spec:
        print(json.dumps(_spec(), indent=2, sort_keys=True))
        return 0

    try:
        root = bootstrap(args.cache_dir, refresh=args.refresh)
    except BootstrapError as exc:
        print(f"GoreeCloud CEF bootstrap failed: {exc}", file=sys.stderr)
        return 1

    if args.print_root:
        print(root)
    else:
        print(json.dumps({**_spec(), "cef_root": str(root)}, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

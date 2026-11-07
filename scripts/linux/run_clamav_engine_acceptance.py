#!/usr/bin/env python3
"""Run real ClamAV + Qt integration without installing antivirus on the host.

Downloads Ubuntu packages from already configured, authenticated APT indices;
checks SHA-256 against the signed-index package metadata before extraction;
runs the optional harmless EICAR test against a disposable local .hsb
signature. Never runs apt install, freshclam, a daemon or real malware.
Requires Linux/Ubuntu compatible APT metadata and a trusted source checkout.
"""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile


PACKAGES = ("clamav", "libclamav12", "libmspack0t64")


def run(arguments: list[str], *, cwd: Path | None = None,
        timeout: int = 120) -> str:
    return subprocess.check_output(
        arguments, cwd=cwd, text=True, timeout=timeout,
        stderr=subprocess.STDOUT,
    )


def deb_metadata(path: Path) -> dict[str, str]:
    metadata = run(["dpkg-deb", "-f", str(path)], timeout=30)
    return dict(line.split(": ", 1) for line in metadata.splitlines()
                if ": " in line)


def expected_digest(package: str, version: str) -> str:
    records = run(["apt-cache", "show", package], timeout=30).split("\n\n")
    for record in records:
        lines = dict(line.split(": ", 1) for line in record.splitlines()
                     if ": " in line)
        if lines.get("Package") == package and lines.get("Version") == version:
            value = lines.get("SHA256", "")
            if len(value) == 64 and all(c in "0123456789abcdef" for c in value):
                return value
    raise RuntimeError(f"No valid APT-index SHA-256 for {package} {version}")


def verify_packages(root: Path) -> list[Path]:
    archives: list[Path] = []
    for package in PACKAGES:
        matches = sorted(root.glob(package + "_*.deb"))
        if len(matches) != 1:
            raise RuntimeError(f"Expected one package archive for {package}")
        archive = matches[0]
        metadata = deb_metadata(archive)
        if metadata.get("Package") != package:
            raise RuntimeError(f"Archive package identity mismatch: {package}")
        expected = expected_digest(package, metadata.get("Version", ""))
        sha = hashlib.sha256()
        with archive.open("rb") as source:
            for block in iter(lambda: source.read(1024 * 1024), b""):
                sha.update(block)
        if sha.hexdigest() != expected:
            raise RuntimeError(f"APT-index package hash mismatch: {package}")
        print(f"VERIFIED_APT_SHA256 {package} {expected[:16]}", flush=True)
        archives.append(archive)
    return archives


def main() -> int:
    if not sys.platform.startswith("linux"):
        raise RuntimeError("Only Linux is supported")
    if os.geteuid() == 0:
        raise RuntimeError("Run as an unprivileged user; no root install needed")

    checkout = Path(__file__).resolve().parents[2]
    assert (checkout / "tests" / "test_linux_real_clamav_smoke.py").is_file()
    with tempfile.TemporaryDirectory(prefix="gcb-clamav-engine-ci-") as scratch:
        work = Path(scratch)
        cache = os.environ.get("GC_BROWSER_CLAMAV_DEB_CACHE", "")
        if cache:
            # Verified against signed APT metadata just like network downloads.
            source = Path(cache).resolve()
            for name in PACKAGES:
                matches = list(source.glob(name + "_*.deb"))
                if len(matches) != 1:
                    raise RuntimeError(f"Missing cached APT archive: {name}")
                (work / matches[0].name).write_bytes(matches[0].read_bytes())
        else:
            print("DOWNLOADING_UBUNTU_APT_ARCHIVES_NO_INSTALL", flush=True)
            result = run(["apt", "download", *PACKAGES], cwd=work, timeout=120)
            print(result[:1000], flush=True)

        archives = verify_packages(work)
        extracted = work / "root"
        extracted.mkdir(mode=0o700)
        for archive in archives:
            run(["dpkg-deb", "-x", str(archive), str(extracted)], timeout=45)

        executable = extracted / "usr" / "bin" / "clamscan"
        libraries = sorted(extracted.glob("usr/lib/**/libclamav.so.12"))
        if not executable.is_file() or len(libraries) != 1:
            raise RuntimeError("Expected ClamAV binary and its shared library")
        certs = work / "certs"
        certs.mkdir(mode=0o700)
        env = os.environ.copy()
        env.update({
            "GC_BROWSER_REAL_CLAMSCAN": str(executable),
            "CVD_CERTS_DIR": str(certs),
            "GC_BROWSER_QT_SMOKE": "1",
            "GC_BROWSER_FILTER_NETWORK": "off",
            "QT_QPA_PLATFORM": "offscreen",
            "QTWEBENGINE_CHROMIUM_FLAGS": "--disable-gpu",
            "LD_LIBRARY_PATH": (
                str(libraries[0].parent)
                + (":" + env["LD_LIBRARY_PATH"]
                   if env.get("LD_LIBRARY_PATH") else "")
            ),
        })
        print("RUNNING_REAL_CLAMAV_EICAR_AND_LIMIT_QT_ACCEPTANCE", flush=True)
        result = subprocess.run(
            [sys.executable, "-m", "unittest", "discover", "-s", "tests",
             "-p", "test_linux_real_clamav_smoke.py", "-v"],
            cwd=checkout, env=env, timeout=90, check=False,
        )
        return result.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError,
            subprocess.TimeoutExpired) as error:
        print(f"REAL_CLAMAV_ACCEPTANCE_BLOCKED: {error}", file=sys.stderr)
        raise SystemExit(1)

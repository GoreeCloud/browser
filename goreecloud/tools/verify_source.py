#!/usr/bin/env python3
"""Verify Firefox stable source before import; independent from the installed keyring."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
LOCK = ROOT / "goreecloud/upstream/BASELINE.json"


def run(*args: str) -> str:
    result = subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    return result.stdout + result.stderr


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path)
    parser.add_argument("signature", type=Path)
    parser.add_argument("key", type=Path, help="Mozilla release KEY fetched from the authoritative source")
    args = parser.parse_args()
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    digest = hashlib.sha256()
    with args.archive.open("rb") as reader:
        for chunk in iter(lambda: reader.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    actual = digest.hexdigest()
    if actual != lock["source_sha256"]:
        raise SystemExit(f"FAIL: archive SHA-256 mismatch ({actual})")
    with tempfile.TemporaryDirectory(prefix="goreecloud-firefox-gpg-") as home:
        Path(home).chmod(0o700)
        run("gpg", "--homedir", home, "--batch", "--import", str(args.key))
        fingerprints = run("gpg", "--homedir", home, "--batch", "--with-colons", "--fingerprint")
        if "fpr:::::::::" + lock["gpg_primary_fingerprint"] + ":" not in fingerprints:
            raise SystemExit("FAIL: Mozilla release-key fingerprint mismatch")
        status = run("gpg", "--homedir", home, "--batch", "--status-fd", "1",
                     "--verify", str(args.signature), str(args.archive))
        marker = "[GNUPG:] VALIDSIG " + lock["gpg_signing_subkey"] + " "
        if marker not in status:
            raise SystemExit("FAIL: expected signing subkey did not validate")
    print(f"PASS: Firefox {lock['version']} Mozilla signature and SHA-256 ({actual})")


if __name__ == "__main__":
    main()

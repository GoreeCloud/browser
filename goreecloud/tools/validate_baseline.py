#!/usr/bin/env python3
"""Read-only structural verification. This is not a full-browser release test."""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
lock = json.loads((ROOT / "goreecloud/upstream/BASELINE.json").read_text())
integration = json.loads((ROOT / "goreecloud/integrations/STATUS.json").read_text())
assert re.fullmatch(r"\d+\.\d+(?:\.\d+)?", lock["version"])
assert len(lock["source_sha256"]) == 64 and re.fullmatch("[0-9a-f]{64}", lock["source_sha256"])
assert lock["gpg_primary_fingerprint"] == "14F26682D0916CDD81E37B6D61B7B526D98F0353"
assert lock["gpg_signing_subkey"] == "827E658608679618CD349F93678E455D76767AA3"
assert lock["distribution_status"] == "not-accepted"
assert integration["runtime"] == "Gecko candidate"
assert integration["design"]["version"] == "1.7.0"
assert integration["design"]["state"] == "target-only"
assert all(item["enabled"] is False and item["state"] == "pending" for item in integration["integrations"])
assert (ROOT / "LICENSE").exists()
assert (ROOT / "browser/config/version.txt").read_text().strip() == lock["version"]
print("PASS: signed-source pin, version, license existence and fail-closed integration registry (structural only)")

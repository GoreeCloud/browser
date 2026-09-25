#!/usr/bin/env python3
"""Fail-closed source guard for Browser Android launcher identity resources.

This proves repository source provenance and structure. APK packaging is checked
separately in CI, and physical launcher rendering remains a device acceptance
requirement.
"""

from pathlib import Path
import hashlib
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
APP = ROOT / "apps" / "android" / "app" / "src" / "main"
ANDROID = "{http://schemas.android.com/apk/res/android}"

failures: list[str] = []


def fail(message: str) -> None:
    failures.append(message)


def git_blob_sha1(path: Path) -> str | None:
    try:
        content = path.read_bytes()
    except OSError as exc:
        fail(f"{path.relative_to(ROOT)} is unreadable: {exc}")
        return None
    blob = b"blob " + str(len(content)).encode("ascii") + bytes([0]) + content
    return hashlib.sha1(blob).hexdigest()


manifest_path = APP / "AndroidManifest.xml"
try:
    manifest = ET.parse(manifest_path).getroot()
except (OSError, ET.ParseError) as exc:
    print(f"Browser Android launcher identity FAILED: unreadable manifest: {exc}", file=sys.stderr)
    raise SystemExit(1)

application = manifest.find("application")
if application is None:
    fail("AndroidManifest.xml has no application element")
else:
    if application.get(ANDROID + "icon") != "@mipmap/ic_launcher":
        fail("application android:icon must be @mipmap/ic_launcher")
    if application.get(ANDROID + "roundIcon") != "@mipmap/ic_launcher_round":
        fail("application android:roundIcon must be @mipmap/ic_launcher_round")

launcher_found = False
for activity in manifest.findall("application/activity"):
    for intent_filter in activity.findall("intent-filter"):
        actions = {
            node.get(ANDROID + "name")
            for node in intent_filter.findall("action")
        }
        categories = {
            node.get(ANDROID + "name")
            for node in intent_filter.findall("category")
        }
        if (
            "android.intent.action.MAIN" in actions
            and "android.intent.category.LAUNCHER" in categories
        ):
            launcher_found = True
            break
if not launcher_found:
    fail("no MAIN/LAUNCHER activity is declared")

expected_adaptive_icons = {
    APP / "res" / "mipmap-anydpi-v26" / "ic_launcher.xml": False,
    APP / "res" / "mipmap-anydpi-v26" / "ic_launcher_round.xml": False,
    APP / "res" / "mipmap-anydpi-v33" / "ic_launcher.xml": True,
    APP / "res" / "mipmap-anydpi-v33" / "ic_launcher_round.xml": True,
}

for path, requires_monochrome in expected_adaptive_icons.items():
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        fail(f"{path.relative_to(ROOT)} is unreadable: {exc}")
        continue

    if root.tag != "adaptive-icon":
        fail(f"{path.relative_to(ROOT)} must use adaptive-icon")

    background = root.find("background")
    foreground = root.find("foreground")
    monochrome = root.find("monochrome")
    if background is None or background.get(ANDROID + "drawable") != "@drawable/ic_launcher_background":
        fail(f"{path.relative_to(ROOT)} must use @drawable/ic_launcher_background")
    if foreground is None or foreground.get(ANDROID + "drawable") != "@drawable/ic_launcher_foreground":
        fail(f"{path.relative_to(ROOT)} must use @drawable/ic_launcher_foreground")
    if requires_monochrome and (
        monochrome is None
        or monochrome.get(ANDROID + "drawable") != "@drawable/ic_launcher_monochrome"
    ):
        fail(f"{path.relative_to(ROOT)} must use @drawable/ic_launcher_monochrome")

# These are the reviewed Android packaging derivatives of the canonical Browser
# compass identity. Changing any of them requires deliberate provenance review.
expected_launcher_derivative_blobs = {
    "ic_launcher_background.xml": "309126e575584a383a9790db0d261146bd25a68c",
    "ic_launcher_foreground.xml": "7e079b05722053fefb44c18251c0a3d16d95ea0b",
    "ic_launcher_monochrome.xml": "5334b483977a67904f883a4006eabfd994a421fe",
}

for drawable, expected_blob in expected_launcher_derivative_blobs.items():
    path = APP / "res" / "drawable" / drawable
    actual_blob = git_blob_sha1(path)
    if actual_blob is not None and actual_blob != expected_blob:
        fail(
            f"{path.relative_to(ROOT)} does not match the reviewed Browser "
            "Android packaging derivative"
        )

# Browser-local branding files must remain exact consumer copies of the current
# canonical GoreeCloud branding assets. This is source provenance only; it does
# not establish launcher/device visual acceptance.
expected_branding_blobs = {
    ROOT / "assets" / "branding" / "goreecloud-browser.svg":
        "3f502e5a6dae8ff3cb86dfdc3ae5d2eb2e8b54e1",
    ROOT / "assets" / "branding" / "goreecloud-browser-monochrome.svg":
        "9f2b1312fc0898d29f84b34affc818e7f308b53f",
}

for path, expected_blob in expected_branding_blobs.items():
    actual_blob = git_blob_sha1(path)
    if actual_blob is not None and actual_blob != expected_blob:
        fail(
            f"{path.relative_to(ROOT)} does not match the pinned canonical "
            "GoreeCloud Browser branding asset"
        )
    try:
        root = ET.parse(path).getroot()
        if root.attrib.get("viewBox") != "0 0 64 64":
            fail(f"{path.relative_to(ROOT)} has unexpected canonical geometry")
    except (OSError, ET.ParseError) as exc:
        fail(f"{path.relative_to(ROOT)} is invalid SVG/XML: {exc}")

if failures:
    print("Browser Android launcher identity FAILED:", file=sys.stderr)
    for message in failures:
        print(f"- {message}", file=sys.stderr)
    raise SystemExit(1)

print("Browser Android launcher identity source guard passed")

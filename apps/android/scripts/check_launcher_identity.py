#!/usr/bin/env python3
"""Fail-closed source guard for Browser Android launcher identity resources.

This proves only repository source structure. APK packaging is checked separately in CI,
and physical launcher rendering remains a device acceptance requirement.
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

expected = {
    APP / "res" / "mipmap-anydpi-v26" / "ic_launcher.xml": False,
    APP / "res" / "mipmap-anydpi-v26" / "ic_launcher_round.xml": False,
    APP / "res" / "mipmap-anydpi-v33" / "ic_launcher.xml": True,
    APP / "res" / "mipmap-anydpi-v33" / "ic_launcher_round.xml": True,
}

for path, requires_monochrome in expected.items():
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

expected_launcher_derivative_blobs = {
    "ic_launcher_background.xml": "309126e575584a383a9790db0d261146bd25a68c",
    "ic_launcher_foreground.xml": "7e079b05722053fefb44c18251c0a3d16d95ea0b",
    "ic_launcher_monochrome.xml": "5334b483977a67904f883a4006eabfd994a421fe",
}

for drawable, expected_blob in expected_launcher_derivative_blobs.items():
    path = APP / "res" / "drawable" / drawable
    if not path.is_file():
        fail(f"missing launcher drawable: {path.relative_to(ROOT)}")
        continue
    try:
        drawable_bytes = path.read_bytes()
    except OSError as exc:
        fail(f"launcher drawable is unreadable: {path.relative_to(ROOT)}: {exc}")
        continue
    git_blob_bytes = b"blob " + str(len(drawable_bytes)).encode("ascii") + bytes([0]) + drawable_bytes
    actual_blob = hashlib.sha1(git_blob_bytes).hexdigest()
    if actual_blob != expected_blob:
        fail(
            f"{path.relative_to(ROOT)} does not match the reviewed canonical Browser "
            "Android packaging derivative"
        )


# Prevent a stale or separately redrawn Browser mark from passing as approved
# central artwork. This proves bytes/provenance, not launcher visual acceptance.
approved_asset = ROOT / "assets" / "branding" / "goreecloud-browser.svg"
expected_central_blob = "3f502e5a6dae8ff3cb86dfdc3ae5d2eb2e8b54e1"
try:
    approved_bytes = approved_asset.read_bytes()
    approved_xml = ET.fromstring(approved_bytes)
    git_blob_bytes = b"blob " + str(len(approved_bytes)).encode("ascii") + bytes([0]) + approved_bytes
    actual_blob = hashlib.sha1(git_blob_bytes).hexdigest()
    if actual_blob != expected_central_blob:
        fail("vendored Browser artwork does not match pinned canonical branding-assets blob")
    if approved_xml.attrib.get("viewBox") != "0 0 64 64":
        fail("vendored Browser artwork has unexpected canonical geometry")
    approved_paths = {node.attrib.get("d") for node in approved_xml.iter() if node.tag.endswith("}path")}
    monochrome_xml = ET.parse(ROOT / "assets" / "branding" / "goreecloud-browser-monochrome.svg").getroot()
    mono_paths = {node.attrib.get("d") for node in monochrome_xml.iter() if node.tag.endswith("}path")}
    if not approved_paths or not mono_paths or approved_paths != mono_paths:
        fail("monochrome Browser derivative must preserve the approved needle path")
except (OSError, ET.ParseError, UnicodeError) as exc:
    fail(f"Browser branding source is missing or invalid: {exc}")

if failures:
    print("Browser Android launcher identity FAILED:", file=sys.stderr)
    for message in failures:
        print(f"- {message}", file=sys.stderr)
    raise SystemExit(1)

print("Browser Android launcher identity source guard passed")

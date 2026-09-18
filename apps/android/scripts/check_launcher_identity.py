#!/usr/bin/env python3
"""Fail-closed source guard for Browser Android launcher identity resources.

This proves only repository source structure. APK packaging is checked separately in CI,
and physical launcher rendering remains a device acceptance requirement.
"""

from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[4]
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
    if background is None or not background.get(ANDROID + "drawable"):
        fail(f"{path.relative_to(ROOT)} is missing an adaptive background drawable")
    if foreground is None or not foreground.get(ANDROID + "drawable"):
        fail(f"{path.relative_to(ROOT)} is missing an adaptive foreground drawable")
    if requires_monochrome and (monochrome is None or not monochrome.get(ANDROID + "drawable")):
        fail(f"{path.relative_to(ROOT)} is missing Android 13+ monochrome artwork")

for drawable in ("ic_launcher_background.xml", "ic_launcher_foreground.xml", "ic_launcher_monochrome.xml"):
    path = APP / "res" / "drawable" / drawable
    if not path.is_file():
        fail(f"missing launcher drawable: {path.relative_to(ROOT)}")

if failures:
    print("Browser Android launcher identity FAILED:", file=sys.stderr)
    for message in failures:
        print(f"- {message}", file=sys.stderr)
    raise SystemExit(1)

print("Browser Android launcher identity source guard passed")

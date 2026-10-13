#!/usr/bin/env python3
"""Generate deterministic, original candidate Gecko branding for Linux/GTK.

This is a developmental GoreeCloud candidate, NOT the approved brand release.
Use --check for a non-mutating integrity test.
"""
import argparse
import math
from pathlib import Path
import struct
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "browser/branding/goreecloud"
MPL = "# This Source Code Form is subject to the terms of the Mozilla Public\n# License, v. 2.0. If a copy of the MPL was not distributed with this\n# file, You can obtain one at https://mozilla.org/MPL/2.0/.\n"
SVG = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" role="img" aria-label="GoreeCloud Browser candidate icon">
  <rect width="128" height="128" rx="29" fill="#182638"/>
  <path d="M92 40a39 39 0 1 0 7 39H68V64h46a55 55 0 1 1-12-36Z" fill="#47D4BF"/>
</svg>
"""
WORDMARK = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 420 64" role="img" aria-label="GoreeCloud Browser">
  <rect width="64" height="64" rx="15" fill="#182638"/>
  <path d="M46 21a20 20 0 1 0 4 19H34v-9h25a28 28 0 1 1-8-16Z" fill="#47D4BF"/>
  <text x="78" y="42" font-family="sans-serif" font-size="31" font-weight="650" fill="currentColor">GoreeCloud</text>
</svg>
"""


def png(data, width, height):
    payload = bytearray()
    for row in data:
        payload.append(0)
        for color in row:
            payload.extend(color)
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(bytes(payload), 9)) + chunk(b"IEND", b"")


def icon(size, private=False):
    bg = (24, 38, 56, 255)
    fg = (71, 212, 191, 255) if not private else (165, 145, 246, 255)
    pixels = []
    for y in range(size):
        row = []
        for x in range(size):
            u, v = (x + .5) / size, (y + .5) / size
            if min(u, v, 1 - u, 1 - v) < .10 and any(
                math.hypot(u - a, v - b) > .10 for a, b in
                [(max(.10, min(.90, u)), max(.10, min(.90, v))) ]
            ):
                base = (0, 0, 0, 0)
            else:
                base = bg
            r = math.hypot(u - .5, v - .5)
            angle = math.atan2(v - .5, u - .5)
            ring = .27 < r < .36 and not (-.56 < angle < .56)
            bar = .49 < v < .60 and .50 < u < .79
            row.append(fg if ring or bar else base)
        pixels.append(row)
    return png(pixels, size, size)


def ico(png_bytes):
    return struct.pack("<HHH", 0, 1, 1) + struct.pack("<BBBBHHII", 64, 64, 0, 0, 1, 32, len(png_bytes), 22) + png_bytes


def generated():
    files = {}
    for size in (16, 22, 24, 32, 48, 64, 128, 256):
        files[f"default{size}.png"] = icon(size)
    logo = icon(128)
    logo_big = icon(256)
    files.update({
        "moz.build": (MPL + '\nDIRS += ["content", "locales"]\nDIST_SUBDIR = "browser"\nexport("DIST_SUBDIR")\ninclude("../branding-common.mozbuild")\nFirefoxBranding()\n').encode(),
        "configure.sh": (MPL + '\nMOZ_APP_DISPLAYNAME="GoreeCloud Browser"\nMOZ_MACBUNDLE_ID="com.goreecloud.browser"\n').encode(),
        "locales/moz.build": (MPL + '\nJAR_MANIFESTS += ["jar.mn"]\n').encode(),
        "locales/jar.mn": (MPL + '\n#filter substitution\n[localization] @AB_CD@.jar:\n  branding (en-US/**/*.ftl)\n\n@AB_CD@.jar:\n% locale branding @AB_CD@ %locale/branding/\n  locale/branding/brand.properties (en-US/brand.properties)\n').encode(),
        "locales/en-US/brand.properties": b"brandShorterName=GoreeCloud Browser\nbrandShortName=GoreeCloud Browser\nbrandFullName=GoreeCloud Browser\n",
        "locales/en-US/brand.ftl": b"-brand-shorter-name = GoreeCloud Browser\n-brand-short-name = GoreeCloud Browser\n-brand-shortcut-name = GoreeCloud Browser\n-brand-full-name = GoreeCloud Browser\n-brand-product-name = GoreeCloud Browser\n-vendor-short-name = GoreeCloud\ntrademarkInfo = { \" \" }\n",
        "pref/firefox-branding.js": b'// GoreeCloud Gecko development build: no Mozilla update or onboarding URLs.\npref("startup.homepage_override_url", "");\npref("startup.homepage_welcome_url", "");\npref("startup.homepage_welcome_url.additional", "");\npref("app.update.url.manual", "");\npref("app.update.url.details", "");\n// Development privacy defaults. No production acceptance implied.\npref("app.normandy.enabled", false);\npref("app.shield.optoutstudies.enabled", false);\npref("browser.discovery.enabled", false);\npref("browser.newtabpage.activity-stream.showSponsored", false);\npref("browser.newtabpage.activity-stream.showSponsoredTopSites", false);\npref("browser.newtabpage.activity-stream.telemetry.privatePing.enabled", false);\n',
        "content/moz.build": (MPL + '\nJAR_MANIFESTS += ["jar.mn"]\n').encode(),
        "content/about-logo.svg": SVG.encode(),
        "content/about-wordmark.svg": WORDMARK.encode(),
        "content/firefox-wordmark.svg": WORDMARK.encode(),
        "content/document_pdf.svg": SVG.encode(),
        "content/about-logo.png": logo,
        "content/about-logo@2x.png": logo_big,
        "content/about-logo-private.png": icon(128, True),
        "content/about-logo-private@2x.png": icon(256, True),
        "content/about.png": logo_big,
        "content/aboutDialog.css": b'/* Development GTK brand surface; not Glaze acceptance evidence. */\n@media (prefers-color-scheme: dark) {\n  #aboutDialogContainer { background-color: #182638; color: #f4f7fa; }\n}\n@media (forced-colors: active) {\n  #aboutDialogContainer { background-color: Canvas; color: CanvasText; }\n}\n',
    })
    for name in ("document.ico", "firefox.ico", "newwindow.ico", "newtab.ico", "pbmode.ico"):
        files[name] = ico(files["default64.png"])
    files["content/jar.mn"] = (MPL + """
browser.jar:
% content branding %content/branding/ contentaccessible=yes
  content/branding/about.png
  content/branding/about-logo.png
  content/branding/about-logo.svg
  content/branding/about-logo@2x.png
  content/branding/about-wordmark.svg
  content/branding/about-logo-private.png
  content/branding/about-logo-private@2x.png
  content/branding/document.ico (../document.ico)
  content/branding/document_pdf.svg
  content/branding/firefox-wordmark.svg
  content/branding/icon16.png (../default16.png)
  content/branding/icon32.png (../default32.png)
  content/branding/icon48.png (../default48.png)
  content/branding/icon64.png (../default64.png)
  content/branding/icon128.png (../default128.png)
  content/branding/aboutDialog.css
""").encode()
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--output", type=Path, default=DEST)
    args = parser.parse_args()
    files = generated()
    for path, data in sorted(files.items()):
        target = args.output / path
        if args.check:
            if not target.is_file() or target.read_bytes() != data:
                raise SystemExit("FAIL: missing or modified generated asset: " + str(target))
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    print(f"PASS: {len(files)} deterministic GTK/Gecko brand assets " + ("verified" if args.check else "generated"))


if __name__ == "__main__":
    main()

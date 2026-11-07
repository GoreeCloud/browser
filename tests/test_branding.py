"""Browser family icon packaging and provenance tests (no Qt required)."""
import hashlib
import json
from pathlib import Path
import struct
import unittest

from goreecloud_browser.branding import browser_icon_path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "branding" / "asset-index.json").read_text(encoding="utf-8"))


class BrowserBrandingTests(unittest.TestCase):
    def test_approved_svg_source_is_exact(self):
        path = ROOT / "goreecloud_browser" / "assets" / "prism-tabs-v5.svg"
        source = path.read_bytes()
        blob = hashlib.sha1(b"blob " + str(len(source)).encode() + b"\0" + source).hexdigest()
        self.assertEqual(blob, "c82371aa6fadd45eb26610bf4907e64122d15b12")
        self.assertEqual(blob, MANIFEST["source_git_blob_sha"])
        self.assertIn(b"Prism Tabs v5", source)

    def test_packaged_png_sizes_and_provenance(self):
        for size in (48, 256):
            with self.subTest(size=size):
                path = browser_icon_path(size)
                data = path.read_bytes()
                self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
                self.assertEqual(struct.unpack(">II", data[16:24]), (size, size))
                self.assertEqual(
                    hashlib.sha256(data).hexdigest(),
                    MANIFEST["files"][f"icon-{size}.png"]["sha256"],
                )

    def test_does_not_resolve_arbitrary_paths(self):
        with self.assertRaises(ValueError):
            browser_icon_path(0)


if __name__ == "__main__":
    unittest.main()

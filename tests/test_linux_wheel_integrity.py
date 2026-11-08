"""Host-independent wheel-integrity acceptance and deliberately corrupted wheels."""
import base64
import csv
import hashlib
from io import StringIO
from pathlib import Path
import tempfile
import sys
import unittest
import warnings
import zipfile

from scripts.linux.verify_wheel_integrity import (
    WheelIntegrityError, verify_wheel,
)


@unittest.skipUnless(sys.version_info >= (3, 11), "Packaging checker requires Python 3.11+")
class WheelIntegrityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="browser-wheel-integrity-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / "goreecloud_browser" / "assets").mkdir(parents=True)
        (self.root / "docs").mkdir()
        (self.root / "pyproject.toml").write_text(
            '[project]\nname = "goreecloud-browser"\nversion = "0.2.7.dev0"\n'
            'license = "Apache-2.0"\nrequires-python = ">=3.10"\n'
            'license-files = ["LICENSE", "NOTICE", "docs/THIRD_PARTY.md"]\n'
            'dependencies = ["PySide6==6.11.3"]\n',
            encoding="utf-8",
        )
        self.origins = {
            "goreecloud_browser/__init__.py": b'__version__ = "0.2.7.dev0"\n',
            "goreecloud_browser/assets/app-icon-48.png": b"\x89PNG\r\nfixture",
            "LICENSE": b"Apache License Version 2.0\n",
            "NOTICE": b"GoreeCloud Browser copyright 2026\n",
            "docs/THIRD_PARTY.md": b"Qt remains separately licensed\n",
        }
        for name, data in self.origins.items():
            file = self.root / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(data)
        self.dist = "goreecloud_browser-0.2.7.dev0.dist-info/"
        self.wheel = self.root / "goreecloud_browser-0.2.7.dev0-py3-none-any.whl"
        self.members = {
            "goreecloud_browser/__init__.py": self.origins["goreecloud_browser/__init__.py"],
            "goreecloud_browser/assets/app-icon-48.png":
                self.origins["goreecloud_browser/assets/app-icon-48.png"],
        }
        for path in ("LICENSE", "NOTICE", "docs/THIRD_PARTY.md"):
            self.members[self.dist + "licenses/" + path] = self.origins[path]
        self.members.update({
            self.dist + "METADATA": (
                "Metadata-Version: 2.4\n"
                "Name: goreecloud-browser\n"
                "Version: 0.2.7.dev0\n"
                "License-Expression: Apache-2.0\n"
                "Requires-Python: >=3.10\n"
                "Requires-Dist: PySide6==6.11.3\n"
                "License-File: LICENSE\n"
                "License-File: NOTICE\n"
                "License-File: docs/THIRD_PARTY.md\n\n"
            ).encode(),
            self.dist + "WHEEL": b"Wheel-Version: 1.0\nRoot-Is-Purelib: true\nTag: py3-none-any\n",
            self.dist + "entry_points.txt": (
                b"[console_scripts]\ngoreecloud-browser = goreecloud_browser.app:main\n"
            ),
            self.dist + "top_level.txt": b"goreecloud_browser\n",
        })
        self.write_wheel()

    def write_wheel(self, *, mutate_record=None, entries=None):
        source = dict(self.members)
        if entries is not None:
            source = dict(entries)
        buffer = StringIO()
        writer = csv.writer(buffer, lineterminator="\n")
        for name, data in source.items():
            h = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).decode().rstrip("=")
            writer.writerow([name, f"sha256={h}", str(len(data))])
        writer.writerow([self.dist + "RECORD", "", ""])
        record = buffer.getvalue().encode()
        if mutate_record:
            record = mutate_record(record)
        with zipfile.ZipFile(self.wheel, "w", compression=zipfile.ZIP_DEFLATED) as z:
            for name, data in source.items():
                z.writestr(name, data)
            z.writestr(self.dist + "RECORD", record)

    def test_complete_untampered_wheel(self):
        files, sha = verify_wheel(self.wheel, self.root)
        self.assertEqual(files, len(self.members) + 1)
        self.assertEqual(len(sha), 64)

    def test_rejects_payload_tampered_without_record_change(self):
        with zipfile.ZipFile(self.wheel) as z:
            items = {name: z.read(name) for name in z.namelist()}
        items["goreecloud_browser/__init__.py"] = b"print('malicious replacement')\n"
        with zipfile.ZipFile(self.wheel, "w") as z:
            for name, data in items.items():
                z.writestr(name, data)
        with self.assertRaisesRegex(WheelIntegrityError, "RECORD hash mismatch"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_notice_omitted_even_with_valid_record(self):
        source = dict(self.members)
        source.pop(self.dist + "licenses/NOTICE")
        self.write_wheel(entries=source)
        with self.assertRaisesRegex(WheelIntegrityError, "missing packaged files"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_rewritten_code_with_valid_record(self):
        source = dict(self.members)
        source["goreecloud_browser/__init__.py"] = b"import os\nos.system('whoami')\n"
        self.write_wheel(entries=source)
        with self.assertRaisesRegex(WheelIntegrityError, "differs from source"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_stale_third_party_notice(self):
        source = dict(self.members)
        source[self.dist + "licenses/docs/THIRD_PARTY.md"] = b"Missing Qt attribution\n"
        self.write_wheel(entries=source)
        with self.assertRaisesRegex(WheelIntegrityError, "differs from source"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_unlisted_module_even_with_valid_record(self):
        source = dict(self.members)
        source["goreecloud_browser/unexpected.py"] = b"# unreviewed code\n"
        self.write_wheel(entries=source)
        with self.assertRaisesRegex(WheelIntegrityError, "unexpected"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_path_traversal_inside_wheel(self):
        source = dict(self.members)
        source["../escape.py"] = b"payload"
        self.write_wheel(entries=source)
        with self.assertRaisesRegex(WheelIntegrityError, "Illegal wheel member"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_symlink_in_source_tree(self):
        origin = self.root / "goreecloud_browser" / "unreviewed.py"
        origin.symlink_to(self.root / "NOTICE")
        with self.assertRaisesRegex(WheelIntegrityError, "Source package contains symlink"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_symlinked_legal_notice(self):
        notice = self.root / "NOTICE"
        notice.unlink()
        notice.symlink_to(self.root / "LICENSE")
        with self.assertRaisesRegex(WheelIntegrityError, "License/notice is a symlink"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_dependency_metadata_mismatch(self):
        source = dict(self.members)
        source[self.dist + "METADATA"] = source[self.dist + "METADATA"].replace(
            b"PySide6==6.11.3", b"PySide6"
        )
        self.write_wheel(entries=source)
        with self.assertRaisesRegex(WheelIntegrityError, "Pinned runtime dependencies"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_malformed_record(self):
        self.write_wheel(mutate_record=lambda b: b.replace(b"sha256=", b"md5=", 1))
        with self.assertRaisesRegex(WheelIntegrityError, "Missing SHA-256"):
            verify_wheel(self.wheel, self.root)

    def test_rejects_duplicate_zip_member(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            with zipfile.ZipFile(self.wheel, "a") as z:
                z.writestr("goreecloud_browser/__init__.py", b"duplicate")
        with self.assertRaisesRegex(WheelIntegrityError, "Duplicate ZIP entry"):
            verify_wheel(self.wheel, self.root)


if __name__ == "__main__":
    unittest.main()

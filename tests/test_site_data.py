import os
from pathlib import Path
import sys
import tempfile
import unittest

from goreecloud_browser.site_data import (
    ProfileLock, RESET_MARKER, apply_pending_site_reset, schedule_site_reset,
)


@unittest.skipUnless(sys.platform.startswith("linux"), "Linux-only profile lock")
class SiteDataTests(unittest.TestCase):
    def test_lock_prevents_second_writer_and_releases(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            first = ProfileLock(root)
            try:
                with self.assertRaises(BlockingIOError):
                    ProfileLock(root)
            finally:
                first.close()
            next_process = ProfileLock(root)
            next_process.close()

    def test_reset_removes_only_managed_webengine_storage(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "web-profile").mkdir()
            (root / "web-profile" / "cookies.db").write_text("cookie")
            (root / "cache").mkdir()
            (root / "cache" / "image.bin").write_bytes(b"content")
            (root / "bookmarks.json").write_text("keep")
            lock = ProfileLock(root)
            try:
                self.assertTrue(schedule_site_reset(root))
                self.assertFalse(schedule_site_reset(root))
                self.assertTrue(apply_pending_site_reset(root))
                self.assertFalse(apply_pending_site_reset(root))
            finally:
                lock.close()
            self.assertFalse((root / "web-profile").exists())
            self.assertFalse((root / "cache").exists())
            self.assertFalse((root / RESET_MARKER).exists())
            self.assertEqual((root / "bookmarks.json").read_text(), "keep")

    def test_invalid_reset_marker_preserves_site_data(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "web-profile").mkdir()
            (root / "web-profile" / "saved").write_text("keep")
            (root / RESET_MARKER).write_text("untrusted request")
            lock = ProfileLock(root)
            try:
                with self.assertRaises(ValueError):
                    apply_pending_site_reset(root)
                with self.assertRaises(ValueError):
                    schedule_site_reset(root)
            finally:
                lock.close()
            self.assertTrue((root / "web-profile" / "saved").exists())

    def test_symlink_target_rejected_before_any_deletion(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            outside = root / "unrelated"
            outside.mkdir()
            (outside / "private").write_text("preserve")
            (root / "web-profile").symlink_to(outside, target_is_directory=True)
            (root / "cache").mkdir()
            (root / "cache" / "cache.txt").write_text("preserve")
            lock = ProfileLock(root)
            try:
                self.assertTrue(schedule_site_reset(root))
                with self.assertRaises(ValueError):
                    apply_pending_site_reset(root)
            finally:
                lock.close()
            self.assertEqual((outside / "private").read_text(), "preserve")
            self.assertEqual((root / "cache" / "cache.txt").read_text(), "preserve")
            self.assertTrue((root / RESET_MARKER).exists())

    def test_webspaces_reset_preserves_registry_and_unrelated_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            space = root / "spaces" / "1234567890abcdef"
            (space / "web-profile").mkdir(parents=True)
            (space / "web-profile" / "cookie.db").write_bytes(b"cookie")
            (space / "cache").mkdir()
            (space / "cache" / "one").write_text("cached")
            (root / "bookmarks.json").write_text("keep")
            lock = ProfileLock(root)
            try:
                schedule_site_reset(root)
                self.assertTrue(apply_pending_site_reset(root))
            finally:
                lock.close()
            self.assertFalse((space / "web-profile").exists())
            self.assertFalse((space / "cache").exists())
            self.assertEqual((root / "bookmarks.json").read_text(), "keep")

    def test_symlinked_webspace_target_blocks_reset(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            out = root / "other"
            out.mkdir()
            (out / "secret").write_text("preserve")
            space = root / "spaces" / "1234567890abcdef"
            space.mkdir(parents=True)
            (space / "web-profile").symlink_to(out, target_is_directory=True)
            (root / "web-profile").mkdir()
            (root / "web-profile" / "cookie").write_text("must-stay")
            lock = ProfileLock(root)
            try:
                schedule_site_reset(root)
                with self.assertRaises(ValueError):
                    apply_pending_site_reset(root)
            finally:
                lock.close()
            self.assertEqual((out / "secret").read_text(), "preserve")
            self.assertEqual((root / "web-profile" / "cookie").read_text(), "must-stay")

    def test_marker_permissions_are_private(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            schedule_site_reset(root)
            self.assertEqual((root / RESET_MARKER).stat().st_mode & 0o777, 0o600)


if __name__ == "__main__":
    unittest.main()

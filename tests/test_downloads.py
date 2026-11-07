"""File publication tests: no browser, no internet, no user data."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from goreecloud_browser.downloads import (
    new_staging, publish_complete_file, suggested_filename, validate_target,
)


class DownloadsTests(unittest.TestCase):
    def test_filename_normalization(self):
        self.assertEqual(suggested_filename(r"..\folder\report.pdf"), "report.pdf")
        self.assertEqual(suggested_filename("../../download.txt"), "download.txt")
        self.assertEqual(suggested_filename(".\u202E"), "download")
        self.assertEqual(suggested_filename("a" * 300), "a" * 160)

    def test_private_stage_and_publish_new_file(self):
        stage, staged = new_staging()
        with tempfile.TemporaryDirectory() as dest:
            try:
                self.assertEqual(Path(stage.name).stat().st_mode & 0o777, 0o700)
                staged.write_bytes(b"innocent content")
                destination = Path(dest) / "file.txt"
                self.assertEqual(publish_complete_file(staged, destination), destination)
                self.assertEqual(destination.read_bytes(), b"innocent content")
                self.assertEqual(destination.stat().st_mode & 0o777, 0o600)
            finally:
                stage.cleanup()
        self.assertFalse(Path(stage.name).exists())

    def test_existing_file_is_never_replaced(self):
        stage, staged = new_staging()
        with tempfile.TemporaryDirectory() as dest:
            try:
                staged.write_bytes(b"new file")
                target = Path(dest) / "existing.txt"
                target.write_bytes(b"preserve me")
                with self.assertRaises(FileExistsError):
                    publish_complete_file(staged, target)
                self.assertEqual(target.read_bytes(), b"preserve me")
            finally:
                stage.cleanup()

    def test_destination_created_during_publish_remains_intact(self):
        stage, staged = new_staging()
        with tempfile.TemporaryDirectory() as dest:
            try:
                staged.write_bytes(b"untrusted bytes")
                target = Path(dest) / "race.txt"
                original_link = os.link

                def competing_writer(src, dst, **kwargs):
                    target.write_bytes(b"competing safe file")
                    return original_link(src, dst, **kwargs)

                with patch("goreecloud_browser.downloads.os.link", side_effect=competing_writer):
                    with self.assertRaises(FileExistsError):
                        publish_complete_file(staged, target)
                self.assertEqual(target.read_bytes(), b"competing safe file")
            finally:
                stage.cleanup()

    def test_symlink_source_and_destination_are_rejected(self):
        with tempfile.TemporaryDirectory() as scratch:
            root = Path(scratch)
            file = root / "outside"
            file.write_bytes(b"private")
            fake_stage = root / "fake"
            fake_stage.symlink_to(file)
            destination = root / "new.txt"
            with self.assertRaises(ValueError):
                publish_complete_file(fake_stage, destination)
            destination.symlink_to(file)
            with self.assertRaises(FileExistsError):
                validate_target(destination)
            self.assertEqual(file.read_bytes(), b"private")

    def test_parent_symlink_is_refused(self):
        with tempfile.TemporaryDirectory() as scratch:
            root = Path(scratch)
            real = root / "real"
            real.mkdir()
            alias = root / "alias"
            alias.symlink_to(real, target_is_directory=True)
            with self.assertRaises(ValueError):
                validate_target(alias / "example.txt")

    def test_exdev_fallback_is_atomic_and_no_overwrite(self):
        stage, staged = new_staging()
        with tempfile.TemporaryDirectory() as dest:
            try:
                staged.write_bytes(b"full downloaded payload")
                target = Path(dest) / "cross-device.bin"
                real_link = os.link
                calls = []

                def exdev_first(src, dst, **kw):
                    calls.append((Path(src), Path(dst)))
                    if len(calls) == 1:
                        raise OSError(18, "Cross-device link")
                    return real_link(src, dst, **kw)

                with patch("goreecloud_browser.downloads.os.link", side_effect=exdev_first):
                    publish_complete_file(staged, target)
                self.assertEqual(target.read_bytes(), b"full downloaded payload")
                self.assertEqual(target.stat().st_mode & 0o777, 0o600)
                self.assertEqual(list(Path(dest).glob(".goreecloud-download-*")), [])
            finally:
                stage.cleanup()

    def test_no_relative_targets(self):
        with self.assertRaises(ValueError):
            validate_target(Path("relative.txt"))


if __name__ == "__main__":
    unittest.main()

"""Pure desktop-entry contract checks (no actual installation required)."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from scripts.linux.install_user import (
    APP_ID, MARKER, desktop_entry, desktop_exec_arg, desktop_icon_value,
)
from scripts.linux.uninstall_user import uninstall


class LinuxDesktopTests(unittest.TestCase):
    @unittest.skipUnless(sys.platform.startswith("linux"), "Linux-only desktop integration")
    def test_desktop_entry_is_valid(self):
        path = Path("/tmp/test goreecloud browser/bin/python")
        icon_path = Path("/tmp/test goreecloud browser/runtime/icon.png")
        entry = desktop_entry(path, icon_path)
        self.assertIn('Exec="/tmp/test goreecloud browser/bin/python" -m goreecloud_browser', entry)
        self.assertIn(f"Icon={icon_path}\n", entry)
        self.assertNotIn("Icon=web-browser", entry)
        self.assertIn(MARKER, entry)
        self.assertNotIn("x-scheme-handler/http", entry)
        self.assertIn("Terminal=false", entry)
        if os.path.isfile("/usr/bin/desktop-file-validate"):
            with tempfile.TemporaryDirectory() as tmp:
                dest = Path(tmp) / (APP_ID + ".desktop")
                dest.write_text(entry, encoding="utf-8")
                subprocess.run(["desktop-file-validate", str(dest)], check=True)

    def test_uninstaller_preserves_unrelated_data(self):
        with tempfile.TemporaryDirectory() as home:
            base = Path(home)
            runtime = base / "goreecloud-browser" / "runtime"
            py = runtime / "venv" / "bin" / "python"
            py.parent.mkdir(parents=True)
            py.write_text("program", encoding="utf-8")
            launcher = base / "applications" / (APP_ID + ".desktop")
            launcher.parent.mkdir()
            launcher.write_text(desktop_entry(py), encoding="utf-8")
            unrelated = base / "user-bookmarks.txt"
            unrelated.write_text("keep", encoding="utf-8")
            self.assertTrue(uninstall(base))
            self.assertFalse(launcher.exists())
            self.assertFalse(runtime.exists())
            self.assertEqual(unrelated.read_text(encoding="utf-8"), "keep")
            self.assertFalse(uninstall(base))

    def test_uninstaller_rejects_unmarked_launcher(self):
        with tempfile.TemporaryDirectory() as home:
            base = Path(home)
            runtime = base / "goreecloud-browser" / "runtime"
            runtime.mkdir(parents=True)
            launcher = base / "applications" / (APP_ID + ".desktop")
            launcher.parent.mkdir()
            launcher.write_text("[Desktop Entry]\nType=Application\nName=Other App\n", encoding="utf-8")
            with self.assertRaises(ValueError):
                uninstall(base)
            self.assertTrue(runtime.is_dir())
            self.assertTrue(launcher.is_file())

    def test_reject_relative_paths(self):
        with self.assertRaises(ValueError):
            desktop_exec_arg(Path("relative/python"))
        with self.assertRaises(ValueError):
            desktop_icon_value(Path("relative/icon.png"))
        with self.assertRaises(ValueError):
            desktop_icon_value(Path("/tmp/test\\nbad/icon.png"))
        self.assertEqual(desktop_icon_value(Path("/tmp/normal/browser/runtime/icon.png")),
                         "/tmp/normal/browser/runtime/icon.png")

    def test_reject_injection_characters(self):
        for path in (Path("/tmp/foo%u"), Path("/tmp/foo\nType=Link"),
                     Path('/tmp/foo" --bad'), Path("/tmp/foo\\bad")):
            with self.subTest(path=path), self.assertRaises(ValueError):
                desktop_exec_arg(path)


if __name__ == "__main__":
    unittest.main()

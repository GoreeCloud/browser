"""Opt-in real ClamAV engine acceptance on disposable local EICAR fixtures.

Unlike the normal CI test-double, this executes an actual ClamAV binary through
GoreeCloud's asynchronous Qt QProcess download gate. Does not install software,
fetch signatures or handle live malware. EICAR is a harmless antivirus test
pattern; an explicit local .hsb signature is used for offline reproducibility.

Set GC_BROWSER_REAL_CLAMSCAN=/absolute/path/to/verified/clamscan and
GC_BROWSER_QT_SMOKE=1. For a user-scoped extracted Ubuntu package, set
LD_LIBRARY_PATH and CVD_CERTS_DIR appropriately. This test never enables
ClamAV's unofficial test-only database in the browser's production path.
"""
import hashlib
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    sys.platform.startswith("linux")
    and os.environ.get("GC_BROWSER_QT_SMOKE") == "1"
    and bool(os.environ.get("GC_BROWSER_REAL_CLAMSCAN")),
    "Opt-in only: verified local real ClamAV engine required",
)
class RealClamAVSmoke(unittest.TestCase):
    def test_async_real_engine_eicar_control_and_scan_limit(self):
        from PySide6.QtCore import QEventLoop, QTimer
        from PySide6.QtWidgets import QApplication, QMessageBox
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.downloads import new_staging
        from goreecloud_browser.malware_scan import scanner_arguments

        scanner = Path(os.environ["GC_BROWSER_REAL_CLAMSCAN"])
        self.assertTrue(scanner.is_absolute() and scanner.is_file())
        self.assertTrue(os.access(scanner, os.X_OK))

        # A published, harmless 68-byte test pattern, NOT actual malware.
        eicar = (
            b"X5O!P%@AP[4\\PZX54(P^)7CC)7}"
            b"$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*"
        )
        self.assertEqual(len(eicar), 68)
        with tempfile.TemporaryDirectory(prefix="gcb-real-clamav-accept-") as temp:
            root = Path(temp)
            database = root / "offline-tests.hsb"
            database.write_text(
                f"{hashlib.sha256(eicar).hexdigest()}:68:"
                "GoreeCloud.EICAR-IntegrationTest.UNOFFICIAL\n",
                encoding="ascii",
            )
            database.chmod(0o600)
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(root / "config"),
                "XDG_DATA_HOME": str(root / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                browser = BrowserWindow()
                browser.show()
                try:
                    def scan_case(name, payload, max_size=None):
                        stage, staged = new_staging()
                        staged.write_bytes(payload)
                        destination = root / f"{name}.bin"
                        wanted = len(browser._download_history) + 1

                        def args_for(path):
                            original = scanner_arguments(path)
                            extra = [f"--database={database}"]
                            if max_size:
                                extra.append(f"--max-filesize={max_size}")
                            return [*original[:-1], *extra, original[-1]]

                        with patch("goreecloud_browser.app.find_clamscan",
                                   return_value=str(scanner)), \
                             patch("goreecloud_browser.app.scanner_arguments",
                                   side_effect=args_for), \
                             patch.object(QMessageBox, "warning",
                                          return_value=QMessageBox.StandardButton.Ok):
                            browser.gate_download(
                                wanted + 200, stage, staged, destination, len(payload)
                            )
                            loop = QEventLoop()

                            def poll():
                                if (len(browser._download_history) >= wanted
                                        and not browser._scan_pending):
                                    loop.quit()
                                else:
                                    QTimer.singleShot(90, poll)

                            QTimer.singleShot(90, poll)
                            QTimer.singleShot(15000, loop.quit)
                            loop.exec()

                        self.assertEqual(len(browser._download_history), wanted,
                                         f"Scanner did not finish: {name}")
                        self.assertFalse(browser._scan_pending)
                        self.assertFalse(Path(stage.name).exists())
                        return destination, browser._download_history[-1]["status"]

                    clean, status = scan_case("clean", b"Harmless local file\n")
                    self.assertEqual(clean.read_bytes(), b"Harmless local file\n")
                    self.assertIn("no detection", status)

                    alert, status = scan_case("eicar", eicar)
                    self.assertFalse(alert.exists(), "EICAR fixture was published")
                    self.assertIn("ClamAV alert", status)

                    too_large, status = scan_case(
                        "over-limit", b"Z" * 8192, max_size="1K"
                    )
                    self.assertFalse(too_large.exists(),
                                     "ClamAV scan-limit skip was published")
                    self.assertIn("scanning failed", status)
                finally:
                    browser.close()
                    app.processEvents()
                    destroy_qobject(browser)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

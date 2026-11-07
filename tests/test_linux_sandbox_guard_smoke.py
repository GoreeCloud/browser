"""Linux native refusal of explicitly disabled Qt WebEngine sandbox.

The rejected configurations must fail before browser QSettings/profile/cache
creation. A safe configuration must still construct a normal native window.
Test uses an isolated temporary XDG home and never visits external sites.
"""
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux Qt sandbox guard smoke",
)
class SandboxNativeSmoke(unittest.TestCase):
    def test_unsafe_flags_refused_before_profile_and_safe_browser_starts(self):
        from PySide6.QtWidgets import QApplication
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.sandbox import WebEngineSecurityError

        with tempfile.TemporaryDirectory(prefix="gcb-sandbox-guard-") as tmp:
            settings = Path(tmp) / "config"
            data = Path(tmp) / "data"
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(settings),
                "XDG_DATA_HOME": str(data),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                window = None
                try:
                    with patch.dict(os.environ, {
                        "QTWEBENGINE_DISABLE_SANDBOX": "1",
                    }):
                        with self.assertRaises(WebEngineSecurityError):
                            BrowserWindow()
                    # Preflight cannot create persistent state or touch
                    # profile paths when Qt's sandbox has been disabled.
                    self.assertFalse(settings.exists())
                    self.assertFalse(data.exists())

                    with patch.object(sys, "argv", [
                        "goreecloud-browser", "--webEngineArgs", "--no-sandbox",
                    ]):
                        with self.assertRaises(WebEngineSecurityError):
                            BrowserWindow()
                    self.assertFalse(data.exists())

                    with patch.object(sys, "argv", ["goreecloud-browser"]):
                        window = BrowserWindow()
                    window.show()
                    app.processEvents()
                    self.assertEqual(window.current().current_address(), "goree://new-tab")
                    self.assertTrue(window.profile.persistentStoragePath())
                finally:
                    if window is not None:
                        window.close()
                        app.processEvents()
                        destroy_qobject(window)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)

    def test_packaged_entrypoint_reports_unsafe_configuration(self):
        import subprocess

        program = (
            "import sys; from unittest.mock import patch; "
            "from goreecloud_browser.app import main; "
            "sys.argv=['goreecloud-browser','--webEngineArgs','--no-sandbox']; "
            "p=patch('goreecloud_browser.app.QMessageBox.critical',return_value=None); "
            "m=p.start(); result=main(); "
            "assert result==2 and m.called; "
            "print('ENTRYPOINT_SANDBOX_REFUSED'); p.stop()"
        )
        with tempfile.TemporaryDirectory(prefix="gcb-unsafe-entrypoint-") as tmp:
            environment = dict(os.environ)
            environment["XDG_CONFIG_HOME"] = str(Path(tmp) / "config")
            environment["XDG_DATA_HOME"] = str(Path(tmp) / "data")
            environment.pop("QTWEBENGINE_DISABLE_SANDBOX", None)
            completed = subprocess.run(
                [sys.executable, "-c", program],
                env=environment, capture_output=True, text=True, timeout=15,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr[-500:])
            self.assertIn("ENTRYPOINT_SANDBOX_REFUSED", completed.stdout)
            self.assertIn("--no-sandbox", completed.stderr)
            self.assertFalse((Path(tmp) / "data").exists())



if __name__ == "__main__":
    unittest.main()

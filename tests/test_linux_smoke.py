"""Linux-only Qt integration smoke with isolated XDG paths and loopback HTTP.

Opt in with GC_BROWSER_QT_SMOKE=1 after installing PySide6.
This test MUST NEVER access a real browser profile.
"""
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import sys
import tempfile
from threading import Thread
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Linux Qt integration smoke is opt-in",
)
class LinuxQtSmoke(unittest.TestCase):
    def test_native_pages_local_web_and_clean_shutdown(self):
        with tempfile.TemporaryDirectory(prefix="gcb-native-smoke-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "config"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
            }):
                self.run_isolated_smoke()

    def run_isolated_smoke(self):
        from PySide6.QtCore import QEventLoop, QTimer
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import QWebEngineProfile
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.site_data import RESET_MARKER, schedule_site_reset

        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                body = b"<html><head><title>Local Integration OK</title></head><body>OK</body></html>"
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def log_message(self, *_args):
                pass

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        Thread(target=server.serve_forever, daemon=True).start()
        app = QApplication.instance() or QApplication([])
        window = BrowserWindow()
        try:
            window.show()
            self.assertEqual(window.tabs.count(), 1)
            self.assertEqual(window.current().current_address(), "goree://new-tab")
            window.show_home()
            self.assertEqual(window.current().current_address(), "goree://home")
            self.assertEqual(
                window.profile.persistentPermissionsPolicy(),
                QWebEngineProfile.PersistentPermissionsPolicy.StoreInMemory,
            )
            self.assertEqual(
                window.profile.persistentCookiesPolicy(),
                QWebEngineProfile.PersistentCookiesPolicy.NoPersistentCookies,
            )
            with self.assertRaises(RuntimeError):
                BrowserWindow()  # a second writer must never use the active profile
            window.new_tab()
            self.assertEqual(window.tabs.count(), 2)
            loaded = []
            event_loop = QEventLoop()
            tab = window.current()

            def on_load(ok):
                loaded.append(ok)
                event_loop.quit()

            tab.view.loadFinished.connect(on_load)
            window.navigate(
                f"http://127.0.0.1:{server.server_port}/?utm_source=test&q=keep"
            )
            QTimer.singleShot(12000, event_loop.quit)
            event_loop.exec()
            self.assertEqual(loaded, [True], "Local page did not finish loading")
            self.assertEqual(tab.view.title(), "Local Integration OK")
            self.assertEqual(
                tab.current_address(),
                f"http://127.0.0.1:{server.server_port}/?q=keep",
            )
            site_root = window.site_storage
            site_storage = site_root / "web-profile"
            site_storage.mkdir(parents=True, exist_ok=True)
            (site_storage / "test-only-file").write_text("remove")
            self.assertTrue(schedule_site_reset(site_root))
        finally:
            window.close()
            app.processEvents()
            destroy_qobject(window)
            server.shutdown()
            server.server_close()

        # Qt profile is no longer running; next startup consumes the reset marker.
        reopened = BrowserWindow()
        try:
            self.assertEqual(reopened.current().current_address(), "goree://new-tab")
            self.assertFalse((site_root / "web-profile" / "test-only-file").exists())
            self.assertFalse((site_root / RESET_MARKER).exists())
        finally:
            reopened.close()
            app.processEvents()
            destroy_qobject(reopened)
            app.quit()
            app.processEvents()
            destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

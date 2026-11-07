"""Loopback-only verification that Qt WebEngine honors a saved global HTTP proxy."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import tempfile
from threading import Thread
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Qt proxy integration test",
)
class ProxySmoke(unittest.TestCase):
    def test_http_proxy_applied_before_webengine_profile(self):
        from PySide6.QtWidgets import QApplication
        from PySide6.QtCore import QEventLoop, QTimer, QSettings
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        requests_seen = []
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                requests_seen.append(self.path)
                body = b"<html><head><title>Proxy Reached</title></head><body>OK</body></html>"
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def log_message(self, *_args):
                pass

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)

        with tempfile.TemporaryDirectory(prefix="gcb-proxy-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                settings = QSettings("GoreeCloud", "GoreeCloudBrowser")
                settings.setValue("proxy_mode", "http")
                settings.setValue("proxy_host", "127.0.0.1")
                settings.setValue("proxy_port", server.server_port)
                settings.sync()
                w = BrowserWindow()
                try:
                    w.show()
                    tab = w.current()
                    outcomes = []
                    event = QEventLoop()

                    def complete(ok):
                        outcomes.append(ok)
                        event.quit()

                    tab.view.loadFinished.connect(complete)
                    w.navigate("http://proxy-only.invalid/through-proxy")
                    QTimer.singleShot(14000, event.quit)
                    event.exec()
                    self.assertIn(True, outcomes)
                    self.assertIn("http://proxy-only.invalid/through-proxy", requests_seen)
                    self.assertEqual(tab.view.title(), "Proxy Reached")
                    self.assertEqual(w.proxy_config.mode, "http")
                finally:
                    w.close()
                    app.processEvents()
                    destroy_qobject(w)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

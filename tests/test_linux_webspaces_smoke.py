"""Real Linux Webspace cookie isolation using a loopback HTTP server only."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import tempfile
from threading import Thread
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux Webspace isolation smoke",
)
class WebspaceIsolationSmoke(unittest.TestCase):
    def test_cookie_boundary_by_real_http_request(self):
        from PySide6.QtCore import QEventLoop, QTimer
        from PySide6.QtWidgets import QApplication
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        requests_seen = []
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                requests_seen.append((self.path, self.headers.get("Cookie", "")))
                body = b"<html><head><title>Local Webspace</title></head><body>Local test</body></html>"
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                if self.path == "/set-default":
                    self.send_header("Set-Cookie", "space_token=default; Path=/; SameSite=Lax")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                try:
                    self.wfile.write(body)
                except (BrokenPipeError, ConnectionResetError):
                    pass
            def log_message(self, *_args):
                pass

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        base = f"http://127.0.0.1:{server.server_port}"

        with tempfile.TemporaryDirectory(prefix="gcb-webspace-isolation-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                browser = BrowserWindow()
                try:
                    browser.show()
                    default_tab = browser.current()
                    ident = browser.add_webspace("Work")
                    # Do not allow storage to escape via a symlinked parent.
                    outside = Path(tmp) / "outside"
                    outside.mkdir()
                    symlink = browser.site_storage / "spaces"
                    symlink.symlink_to(outside, target_is_directory=True)
                    with self.assertRaises(ValueError):
                        browser.get_profile(ident)
                    self.assertEqual(list(outside.iterdir()), [])
                    symlink.unlink()
                    work_tab = browser.new_tab(space_id=ident)
                    self.assertIsNot(work_tab.profile, browser.profile)
                    self.assertNotEqual(
                        work_tab.profile.persistentStoragePath(),
                        browser.profile.persistentStoragePath(),
                    )

                    def visit(tab, url):
                        loop = QEventLoop()
                        results = []
                        def done(ok):
                            results.append(ok)
                            loop.quit()
                        tab.view.loadFinished.connect(done)
                        try:
                            browser.navigate(url, tab=tab)
                            QTimer.singleShot(10000, loop.quit)
                            loop.exec()
                        finally:
                            tab.view.loadFinished.disconnect(done)
                        self.assertIn(True, results, f"Failed local URL load: {url}")
                        # Cookie-store updates are async; allow the Qt event loop
                        # to dispatch the write before the next local request.
                        settle = QEventLoop()
                        QTimer.singleShot(150, settle.quit)
                        settle.exec()

                    visit(default_tab, base + "/set-default")
                    visit(work_tab, base + "/from-work")
                    visit(default_tab, base + "/from-default")
                    matching = dict(requests_seen)
                    self.assertNotIn("space_token", matching.get("/from-work", ""))
                    self.assertIn("space_token=default", matching.get("/from-default", ""))
                    self.assertEqual(browser.webspace_name(ident), "Work")
                    self.assertEqual(work_tab.space_id, ident)
                finally:
                    browser.close()
                    app.processEvents()
                    destroy_qobject(browser)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

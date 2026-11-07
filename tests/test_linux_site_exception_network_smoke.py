"""Actual Qt WebEngine HTTP test of exact-host Privacy Shield exceptions.

Uses only a loopback HTTP server and RFC-reserved .invalid hostnames.
CI sets process-only Chromium host-resolver rules mapping those names to
127.0.0.1 and disables the proxy. No OS DNS or hosts-file changes.
"""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import tempfile
from threading import Thread
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux WebEngine site-exception HTTP acceptance",
)
class RealSiteExceptionsSmoke(unittest.TestCase):
    def test_real_browser_blocks_allows_and_restores_one_host(self):
        from PySide6.QtCore import QEventLoop, QTimer
        from PySide6.QtWidgets import QApplication
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        main_host = "news.browser-test.invalid"
        asset_host = "ads.browser-test.invalid"
        requests_seen = []
        gif = (
            b"GIF89a\x01\x00\x01\x00\x80\x00\x00\x00\x00\x00\xff\xff\xff!"
            b"\xf9\x04\x01\x00\x00\x00\x00,\x00\x00\x00\x00\x01\x00\x01"
            b"\x00\x00\x02\x02D\x01\x00;"
        )

        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                requests_seen.append((self.headers.get("Host", ""), self.path))
                if self.path.startswith("/pixel?step="):
                    body = gif
                    mime = "image/gif"
                else:
                    step = self.path.strip("/").split("/", 1)[0]
                    body = (
                        f'<html><head><title>Local {step}</title></head>'
                        f'<body><img src="http://{asset_host}:'
                        f'{server.server_port}/pixel?step={step}"></body></html>'
                    ).encode("utf-8")
                    mime = "text/html"
                self.send_response(200)
                self.send_header("Content-Type", mime)
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

        with tempfile.TemporaryDirectory(prefix="gcb-real-site-exceptions-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                browser = BrowserWindow()
                try:
                    browser.show()
                    browser.interceptor.block_trackers = False
                    browser.interceptor.domain_rules = (frozenset({asset_host}),)
                    tab = browser.current()
                    base = f"http://{main_host}:{server.server_port}"

                    def visit(step):
                        completed = []
                        event = QEventLoop()

                        def on_done(ok):
                            completed.append(ok)
                            event.quit()

                        tab.view.loadFinished.connect(on_done)
                        try:
                            browser.navigate(base + "/" + step)
                            if not completed:
                                QTimer.singleShot(5000, event.quit)
                                event.exec()
                            self.assertIn(True, completed, f"Failed local page load: {step}")
                            # Give an image resource time to reach the local
                            # server after the main document's load finished.
                            settle = QEventLoop()
                            QTimer.singleShot(800, settle.quit)
                            settle.exec()
                        finally:
                            tab.view.loadFinished.disconnect(on_done)
                        self.assertIn(
                            (f"{main_host}:{server.server_port}", "/" + step),
                            requests_seen,
                        )

                    visit("blocked")
                    self.assertNotIn(
                        (f"{asset_host}:{server.server_port}", "/pixel?step=blocked"),
                        requests_seen,
                    )
                    browser.add_site_exception(main_host)
                    visit("allowed")
                    self.assertIn(
                        (f"{asset_host}:{server.server_port}", "/pixel?step=allowed"),
                        requests_seen,
                    )
                    browser.remove_site_exception(main_host)
                    visit("restored")
                    self.assertNotIn(
                        (f"{asset_host}:{server.server_port}", "/pixel?step=restored"),
                        requests_seen,
                    )
                finally:
                    browser.close()
                    app.processEvents()
                    destroy_qobject(browser)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

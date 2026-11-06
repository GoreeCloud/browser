"""Linux Qt integration check against an isolated local HTTP server.

Opt in with GC_BROWSER_QT_SMOKE=1 only after installing PySide6.
No remote websites are contacted.
"""
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import sys
from threading import Thread
import unittest


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Linux Qt integration smoke is opt-in",
)
class LinuxQtSmoke(unittest.TestCase):
    def test_native_pages_local_web_and_clean_shutdown(self):
        from PySide6.QtCore import QEventLoop, QTimer
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import QWebEngineProfile
        from goreecloud_browser.app import BrowserWindow

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
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
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
        finally:
            window.close()
            app.processEvents()
            server.shutdown()
            server.server_close()


if __name__ == "__main__":
    unittest.main()

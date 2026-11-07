"""Local-only Qt WebEngine navigation integrity checks for the Linux preview.

Run opt-in using GC_BROWSER_QT_SMOKE=1. No public network is accessed.
Each run uses temporary XDG profile paths and an ephemeral loopback server.
"""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import sys
import tempfile
from threading import Thread
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Linux native WebEngine integration smoke is opt-in",
)
class NetworkNavigationSmoke(unittest.TestCase):
    def test_signed_get_redirect_post_and_unsupported_scheme(self):
        from PySide6.QtCore import QEventLoop, QTimer, QUrl
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import QWebEnginePage
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_args):
                pass

            def respond(self, title):
                body = (
                    f"<html><head><title>{title}</title></head>"
                    f"<body>{title}</body></html>"
                ).encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                try:
                    self.wfile.write(body)
                except (BrokenPipeError, ConnectionResetError):
                    # WebEngine may cancel a superseded navigation before the
                    # loopback response is fully delivered.
                    pass

            def do_GET(self):
                self.server.requests.append(("GET", self.path, b""))
                path = urlsplit(self.path).path
                if path == "/redirect":
                    self.send_response(302)
                    self.send_header(
                        "Location",
                        "/final?utm_source=redirect&q=a%20b&literal=a%2Bb",
                    )
                    self.send_header("Content-Length", "0")
                    self.end_headers()
                elif path == "/form":
                    body = (
                        b"<html><head><title>Form</title></head><body>"
                        b"<form id='post' method='POST' "
                        b"action='/submit?utm_source=stay&item=2'>"
                        b"<input name='payload' value='hello world'>"
                        b"</form></body></html>"
                    )
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    try:
                        self.wfile.write(body)
                    except (BrokenPipeError, ConnectionResetError):
                        # WebEngine may cancel an in-flight local navigation.
                        pass
                else:
                    self.respond("Local Network OK")

            def do_POST(self):
                length = int(self.headers.get("Content-Length", "0"))
                body = self.rfile.read(length)
                self.server.requests.append(("POST", self.path, body))
                self.respond("POST OK")

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.requests = []
        Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        base = f"http://127.0.0.1:{server.server_port}"
        with tempfile.TemporaryDirectory(prefix="gcb-network-smoke-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "config"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
            }):
                app = QApplication.instance() or QApplication([])
                window = BrowserWindow()
                try:
                    window.show()
                    view = window.current().view

                    def await_load(trigger):
                        outcomes = []
                        loop = QEventLoop()

                        def done(ok):
                            outcomes.append(ok)
                            # Record all native page load results and wait
                            # for the completed local navigation.
                            if ok:
                                loop.quit()

                        view.loadFinished.connect(done)
                        try:
                            trigger()
                            QTimer.singleShot(12000, loop.quit)
                            loop.exec()
                        finally:
                            view.loadFinished.disconnect(done)
                        self.assertIn(True, outcomes, f"Local load never completed: {outcomes}")

                    # Server-driven redirects are NOT sanitized. The address
                    # must not misleadingly appear cleaned when the request
                    # containing the parameter has already reached the server.
                    await_load(lambda: window.navigate(base + "/redirect"))
                    redirect_path = "/final?utm_source=redirect&q=a%20b&literal=a%2Bb"
                    self.assertEqual(
                        view.url().toString(QUrl.ComponentFormattingOption.FullyEncoded),
                        base + redirect_path,
                    )
                    self.assertIn(("GET", redirect_path, b""), server.requests)

                    # Typed/pasted address is sanitized BEFORE the HTTP request
                    # without changing the surviving raw query encodings.
                    await_load(lambda: window.navigate(
                        base + "/typed?utm_source=typed&q=a%20b&literal=a%2Bb"
                    ))
                    clean_path = "/typed?q=a%20b&literal=a%2Bb"
                    self.assertIn(("GET", clean_path, b""), server.requests)
                    self.assertEqual(window.current().current_address(), base + clean_path)
                    self.assertNotIn(
                        ("GET", "/typed?utm_source=typed&q=a%20b&literal=a%2Bb", b""),
                        server.requests,
                    )

                    # Users may disable cleaning; their chosen URL is preserved.
                    window.set_preference("clean_links", False)
                    optout = "/optout?utm_source=stay&item=2"
                    await_load(lambda: window.navigate(base + optout))
                    self.assertIn(("GET", optout, b""), server.requests)
                    window.set_preference("clean_links", True)

                    # Signed URLs must remain byte-for-byte intact including tracking keys.
                    signed = "/signed?X-Amz-Signature=abc%2Bdef&utm_source=stay"
                    await_load(lambda: window.navigate(base + signed))
                    self.assertIn(("GET", signed, b""), server.requests)

                    # Form POST must be passed through unchanged by the browser.
                    await_load(lambda: window.navigate(base + "/form"))
                    await_load(lambda: view.page().runJavaScript(
                        "document.getElementById('post').submit()"
                    ))
                    self.assertEqual(view.title(), "POST OK")
                    self.assertTrue(
                        any(method == "POST"
                            and path == "/submit?utm_source=stay&item=2"
                            and body == b"payload=hello+world"
                            for method, path, body in server.requests),
                        "Form POST query/body were changed or suppressed",
                    )

                    page = view.page()
                    self.assertFalse(page.acceptNavigationRequest(
                        QUrl("file:///etc/passwd"),
                        QWebEnginePage.NavigationType.NavigationTypeLinkClicked,
                        True,
                    ))
                    self.assertFalse(page.acceptNavigationRequest(
                        QUrl("javascript:alert('bad')"),
                        QWebEnginePage.NavigationType.NavigationTypeLinkClicked,
                        True,
                    ))
                finally:
                    window.close()
                    app.processEvents()
                    destroy_qobject(window)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

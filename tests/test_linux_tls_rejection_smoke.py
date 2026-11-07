"""Linux Qt WebEngine rejects a self-signed HTTPS certificate by default.

Only a local HTTPS server is contacted. The leaf certificate/key are generated
inside a disposable temp directory and are never installed or trusted.
"""
import os
from pathlib import Path
import shutil
import ssl
import subprocess
import sys
import tempfile
from threading import Thread
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux local certificate rejection smoke",
)
class TLSRejectionSmoke(unittest.TestCase):
    def test_explicit_certificate_rejection_blocks_local_https_page(self):
        if shutil.which("openssl") is None:
            self.fail("OpenSSL is required for security acceptance; no test was run")

        from PySide6.QtCore import QEventLoop, QTimer
        from PySide6.QtWidgets import QApplication
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        received_pages = []
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                received_pages.append(self.path)
                payload = (
                    b"<html><head><title>INVALID CERT PAGE LOADED</title></head>"
                    b"<body>The browser must reject this content.</body></html>"
                )
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)

            def log_message(self, *_args):
                pass

        with tempfile.TemporaryDirectory(prefix="gcb-local-tls-rejection-") as tmp:
            cert = Path(tmp) / "ephemeral.crt"
            key = Path(tmp) / "ephemeral.key"
            subprocess.run([
                "openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                "-days", "1", "-subj", "/CN=localhost",
                "-addext", "subjectAltName=DNS:localhost,IP:127.0.0.1",
                "-keyout", str(key), "-out", str(cert),
            ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
               timeout=15, check=True)
            self.assertTrue(cert.is_file() and key.is_file())
            server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.load_cert_chain(str(cert), str(key))
            server.socket = context.wrap_socket(server.socket, server_side=True)
            Thread(target=server.serve_forever, daemon=True).start()
            self.addCleanup(server.server_close)
            self.addCleanup(server.shutdown)

            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "browser-config"),
                "XDG_DATA_HOME": str(Path(tmp) / "browser-data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                browser = BrowserWindow()
                try:
                    browser.show()
                    tab = browser.current()
                    page = tab.view.page()
                    errors = []
                    completed = []
                    event = QEventLoop()

                    # Observe independently; BrowserPage must reject rather
                    # than accept, and signal consumers must not override it.
                    page.certificateError.connect(
                        lambda err: errors.append(
                            (err.type().name, err.isOverridable())
                        )
                    )
                    page.loadFinished.connect(
                        lambda ok: (completed.append(ok), event.quit())
                    )
                    browser.navigate(
                        f"https://127.0.0.1:{server.server_port}/invalid-leaf"
                    )
                    if not completed:
                        QTimer.singleShot(12000, event.quit)
                        event.exec()
                    self.assertTrue(errors, "Qt did not surface the TLS trust failure")
                    self.assertTrue(any(allowed for _, allowed in errors),
                                    "The test certificate should be overridable")
                    self.assertIn(False, completed, "Invalid certificate unexpectedly loaded")
                    self.assertFalse(any(completed), "HTTPS content load succeeded unexpectedly")
                    self.assertNotEqual(page.title(), "INVALID CERT PAGE LOADED")
                    self.assertEqual(received_pages, [],
                                     "Untrusted HTTPS content was delivered to the page")
                finally:
                    browser.close()
                    app.processEvents()
                    destroy_qobject(browser)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

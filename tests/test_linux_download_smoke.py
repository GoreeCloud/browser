"""Linux-only real Qt WebEngine download smoke against an isolated loopback server."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import tempfile
from threading import Thread
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in native Linux download integration test",
)
class LinuxDownloadsSmoke(unittest.TestCase):
    def test_download_staged_then_published_without_clobber(self):
        from PySide6.QtCore import QTimer
        from PySide6.QtWidgets import QApplication, QFileDialog, QMessageBox
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        content = b"GoreeCloud Browser local download test payload\x00\x01"
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                self.send_response(200)
                self.send_header("Content-Type", "application/octet-stream")
                self.send_header("Content-Disposition", 'attachment; filename="../../bad-name.bin"')
                self.send_header("Content-Length", str(len(content)))
                self.end_headers()
                try:
                    self.wfile.write(content)
                except (BrokenPipeError, ConnectionResetError):
                    pass

            def log_message(self, *_args):
                pass

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)

        with tempfile.TemporaryDirectory(prefix="gcb-download-test-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "profile"),
            }):
                app = QApplication([])
                window = BrowserWindow()
                target = Path(tmp) / "saved-file.bin"
                original = Path(tmp) / "do-not-overwrite.bin"
                original.write_bytes(b"existing file")
                prompts = []
                done = []
                second_started = []
                staged_directories = []
                expected_prompts = [str(target), str(original)]

                def save_dialog(*_args, **_kwargs):
                    prompts.append(True)
                    return expected_prompts[len(prompts) - 1], ""

                def observe_download(_request):
                    for _, temporary, _, _ in window._pending_downloads.values():
                        staged_directories.append(Path(temporary.name))

                def poll_result():
                    if target.exists() and not window._pending_downloads and not second_started:
                        second_started.append(True)
                        window.navigate(f"http://127.0.0.1:{server.server_port}/second")
                    elif len(prompts) == 2 and not window._pending_downloads:
                        finish()
                        return
                    QTimer.singleShot(120, poll_result)

                def finish():
                    done.append(True)
                    window.close()
                    app.quit()

                with patch.object(QFileDialog, "getSaveFileName", side_effect=save_dialog), \
                        patch("goreecloud_browser.app.find_clamscan", return_value=None), \
                        patch.object(QMessageBox, "question",
                                     return_value=QMessageBox.StandardButton.Yes), \
                        patch.object(QMessageBox, "warning", return_value=QMessageBox.StandardButton.Ok):
                    window.profile.downloadRequested.connect(observe_download)
                    window.show()
                    window.navigate(f"http://127.0.0.1:{server.server_port}/first")
                    QTimer.singleShot(100, poll_result)
                    QTimer.singleShot(15000, finish)
                    app.exec()

                self.assertEqual(len(prompts), 2, "Expected user prompts for both downloads")
                self.assertTrue(done, "Browser download test did not exit cleanly")
                self.assertEqual(target.read_bytes(), content)
                self.assertEqual(original.read_bytes(), b"existing file")
                self.assertEqual(target.stat().st_mode & 0o777, 0o600)
                self.assertEqual(window._pending_downloads, {})
                self.assertTrue(any(row["status"] == "Saved — NOT SCANNED"
                                    for row in window._download_history))
                self.assertTrue(staged_directories)
                self.assertTrue(all(not path.exists() for path in staged_directories))
                app.processEvents()
                destroy_qobject(window)
                destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

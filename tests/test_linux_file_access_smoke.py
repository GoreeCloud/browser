"""Real Linux WebEngine local-file separation regression.

A disposable file:// HTML page tries to inspect a neighboring disposable
file:// document. A test-only control profile demonstrates the unsafe access
with Qt's default setting; the GoreeCloud-secured profile must block it.
No real user files, remote websites, or browser profiles are accessed.
"""
import os
from pathlib import Path
import sys
import tempfile
import unittest


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Qt local-file isolation acceptance",
)
class LocalFileIsolationSmoke(unittest.TestCase):
    def test_local_html_cannot_read_other_local_file(self):
        from PySide6.QtCore import QEventLoop, QTimer, QUrl
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import (
            QWebEnginePage, QWebEngineProfile, QWebEngineSettings,
        )
        from PySide6.QtWebEngineWidgets import QWebEngineView
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.web_security import configure_content_security

        with tempfile.TemporaryDirectory(prefix="gcb-local-file-isolation-") as tmp:
            root = Path(tmp)
            (root / "neighbor.html").write_text(
                "<!doctype html><title>FIXTURE_READABLE</title><p>dummy fixture</p>",
                encoding="utf-8",
            )
            (root / "index.html").write_text(
                '<!doctype html><title>Top Level</title>'
                '<iframe id="neighbor" src="neighbor.html"></iframe>',
                encoding="utf-8",
            )
            app = QApplication([])
            try:
                def read_neighbor(configure):
                    # Separate OTR profiles for the two local test cases; no
                    # existing persistent Webspace or user browser profile.
                    profile = QWebEngineProfile(app)
                    view = QWebEngineView()
                    page = QWebEnginePage(profile)
                    view.setPage(page)
                    if configure:
                        configure_content_security(profile)
                    else:
                        # Deliberate test-only positive control: Qt's default
                        # local-file permission on an empty disposable profile.
                        self.assertTrue(profile.settings().testAttribute(
                            QWebEngineSettings.WebAttribute.LocalContentCanAccessFileUrls
                        ))
                    try:
                        view.show()
                        finished = []
                        wait = QEventLoop()
                        page.loadFinished.connect(
                            lambda ok: (finished.append(ok), wait.quit())
                        )
                        view.load(QUrl.fromLocalFile(str(root / "index.html")))
                        if not finished:
                            QTimer.singleShot(8000, wait.quit)
                            wait.exec()
                        self.assertIn(True, finished, "Local fixture failed to load")
                        settle = QEventLoop()
                        QTimer.singleShot(700, settle.quit)
                        settle.exec()

                        result = []
                        event = QEventLoop()
                        page.runJavaScript(
                            "(()=>{try{return document.getElementById('neighbor')"
                            ".contentDocument?.title || 'NOT_ACCESSIBLE'}"
                            "catch(e){return 'BLOCKED'}})()",
                            lambda value: (result.append(value), event.quit()),
                        )
                        if not result:
                            QTimer.singleShot(5000, event.quit)
                            event.exec()
                        self.assertTrue(result, "No JavaScript result from fixture")
                        return result[-1]
                    finally:
                        view.close()
                        app.processEvents()
                        destroy_qobject(view)
                        destroy_qobject(page)
                        destroy_qobject(profile)

                self.assertEqual(read_neighbor(False), "FIXTURE_READABLE")
                self.assertIn(read_neighbor(True), {"NOT_ACCESSIBLE", "BLOCKED"})
            finally:
                app.quit()
                app.processEvents()
                destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

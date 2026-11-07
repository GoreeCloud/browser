"""Linux privacy-site exception behavior with only local request fixtures."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux Qt site exception check",
)
class SiteExceptionsSmoke(unittest.TestCase):
    def test_confirmed_exact_host_allow_and_restore(self):
        from PySide6.QtCore import QUrl
        from PySide6.QtWidgets import QApplication, QMessageBox
        from PySide6.QtWebEngineCore import QWebEngineUrlRequestInfo
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.site_exceptions_panel import SiteExceptionsDialog

        class RequestInfo:
            def __init__(self, first_party, kind=None):
                self.first_party = first_party
                self.kind = kind or QWebEngineUrlRequestInfo.ResourceType.ResourceTypeImage
                self.blocked = False
            def requestUrl(self):
                return QUrl("https://ads.example.net/pixel.gif")
            def firstPartyUrl(self):
                return QUrl(self.first_party)
            def resourceType(self):
                return self.kind
            def block(self, value):
                self.blocked = bool(value)

        with tempfile.TemporaryDirectory(prefix="gcb-site-exceptions-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                browser = BrowserWindow()
                manager = None
                try:
                    browser.show()
                    browser.interceptor.domain_rules = (
                        frozenset({"ads.example.net"}),)
                    browser.interceptor.block_trackers = False
                    baseline = RequestInfo("https://news.example.org/")
                    browser.interceptor.interceptRequest(baseline)
                    self.assertTrue(baseline.blocked)

                    manager = SiteExceptionsDialog(browser)
                    manager.hostname.setText("news.example.org")
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.No):
                        manager.add_host()
                    self.assertEqual(browser.site_exceptions, ())
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.Yes):
                        manager.add_host()
                    self.assertEqual(browser.site_exceptions, ("news.example.org",))
                    self.assertEqual(manager.list.count(), 1)
                    self.assertIn("news.example.org",
                                  browser.preferences.value("privacy_site_exceptions"))
                    for website in ("https://news.example.org/",
                                    "https://news.example.org/path",
                                    "http://news.example.org/"):
                        info = RequestInfo(website)
                        browser.interceptor.interceptRequest(info)
                        self.assertFalse(info.blocked, website)

                    for website in ("https://blog.news.example.org/",
                                    "https://news.example.org.evil.test/",
                                    "https://another.example.org/"):
                        info = RequestInfo(website)
                        browser.interceptor.interceptRequest(info)
                        self.assertTrue(info.blocked, website)

                    # Fail closed for missing first-party context.
                    missing = RequestInfo("about:blank")
                    browser.interceptor.interceptRequest(missing)
                    self.assertTrue(missing.blocked)

                    manager.list.setCurrentRow(0)
                    manager.remove_selected()
                    self.assertEqual(browser.site_exceptions, ())
                    restored = RequestInfo("https://news.example.org/")
                    browser.interceptor.interceptRequest(restored)
                    self.assertTrue(restored.blocked)

                    # Multiple-host clear requires a second confirmation.
                    browser.add_site_exception("news.example.org")
                    browser.add_site_exception("portal.example.org")
                    manager.refresh()
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.No):
                        manager.clear_all()
                    self.assertEqual(len(browser.site_exceptions), 2)
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.Yes):
                        manager.clear_all()
                    self.assertEqual(browser.site_exceptions, ())
                    self.assertEqual(manager.list.count(), 0)
                    self.assertEqual(browser.preferences.value("privacy_site_exceptions"),
                                     "[]")

                    with patch("goreecloud_browser.app.SiteExceptionsDialog.exec",
                               return_value=0) as opened:
                        browser.show_site_exceptions()
                    opened.assert_called_once_with()
                finally:
                    if manager is not None:
                        manager.close()
                        destroy_qobject(manager)
                    browser.close()
                    app.processEvents()
                    destroy_qobject(browser)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

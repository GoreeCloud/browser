"""Linux Qt profile content-security settings and Webspace parity acceptance.

No external sites, user profile or live system settings are accessed.
"""
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    sys.platform.startswith("linux") and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux WebEngine content security smoke",
)
class ContentSecuritySmoke(unittest.TestCase):
    def test_default_and_new_webspaces_inherit_strict_content_settings(self):
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import QWebEngineSettings
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.web_security import DISABLED_WEB_ATTRIBUTES

        with tempfile.TemporaryDirectory(prefix="gcb-content-security-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "config"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                browser = BrowserWindow()
                try:
                    browser.show()
                    default_tab = browser.current()
                    work_id = browser.add_webspace("Work")
                    work_tab = browser.new_tab(space_id=work_id)
                    self.assertIsNot(default_tab.profile, work_tab.profile)
                    self.assertEqual(
                        set(DISABLED_WEB_ATTRIBUTES),
                        {
                            "AllowRunningInsecureContent",
                            "AllowGeolocationOnInsecureOrigins",
                            "LocalContentCanAccessRemoteUrls",
                            "LocalContentCanAccessFileUrls",
                            "JavascriptCanOpenWindows",
                            "JavascriptCanAccessClipboard",
                            "JavascriptCanPaste",
                            "HyperlinkAuditingEnabled",
                        },
                    )
                    for tab in (default_tab, work_tab):
                        with self.subTest(webspace=tab.space_id):
                            settings = tab.profile.settings()
                            self.assertEqual(
                                settings.unknownUrlSchemePolicy(),
                                QWebEngineSettings.UnknownUrlSchemePolicy.DisallowUnknownUrlSchemes,
                            )
                            for attr_name in DISABLED_WEB_ATTRIBUTES:
                                attr = getattr(QWebEngineSettings.WebAttribute, attr_name)
                                self.assertFalse(settings.testAttribute(attr), attr_name)
                                self.assertFalse(tab.view.page().settings().testAttribute(attr),
                                                 f"{tab.space_id}: {attr_name}")
                    self.assertIs(
                        default_tab.profile.settings().testAttribute(
                            QWebEngineSettings.WebAttribute.JavascriptEnabled), True
                    )
                    self.assertIs(
                        work_tab.profile.settings().testAttribute(
                            QWebEngineSettings.WebAttribute.JavascriptEnabled), True
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

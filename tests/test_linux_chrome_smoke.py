"""Linux native toolbar, Settings and Webspace isolation smoke.

Opt-in, isolated temporary XDG paths. No external URLs or filter downloads.
"""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux Qt browser chrome smoke",
)
class ChromeAndWebspaceTests(unittest.TestCase):
    def test_toolbar_order_light_theme_and_native_settings(self):
        from PySide6.QtWidgets import QApplication, QGroupBox
        from PySide6.QtCore import Qt
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.native_panels import SettingsPage

        with tempfile.TemporaryDirectory(prefix="gcb-native-toolbar-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                window = BrowserWindow()
                try:
                    window.show()
                    app.processEvents()
                    self.assertEqual(window.mode, "light")
                    self.assertEqual(window.toolbar_order, (
                        "back", "forward", "refresh", "home", "omnibox",
                        "bookmark", "extensions", "settings", "profile",
                    ))
                    nav = window.centralWidget().layout().itemAt(0).widget()
                    controls = [nav.layout().itemAt(i).widget() for i in range(9)]
                    self.assertEqual(controls, [
                        window.back, window.forward, window.reload, window.home_button,
                        window.omnibox, window.bookmark_button, window.extensions_button,
                        window.settings_button, window.profile_button,
                    ])
                    for item in controls:
                        if item is window.omnibox:
                            continue
                        self.assertEqual(item.text(), "", "Chrome buttons must be glyph-only")
                        self.assertFalse(item.icon().isNull())
                        self.assertTrue(bool(item.accessibleName()))
                    omni = window.omnibox.layout()
                    self.assertEqual(
                        [omni.itemAt(i).widget() for i in range(4)],
                        [window.verify_button, window.permissions_button,
                         window.address, window.webspaces_button],
                    )
                    for item in (window.verify_button, window.permissions_button, window.webspaces_button):
                        self.assertEqual(item.text(), "")
                        self.assertFalse(item.icon().isNull())
                    window.show_settings()
                    self.assertEqual(window.current().current_address(), "goree://settings")
                    content = window.current().settings_page
                    self.assertIsInstance(content, SettingsPage)
                    titles = [x.title() for x in content.findChildren(QGroupBox)]
                    for expected in ("Theme Manager", "ChatGPT.com", "Cookies Manager", "DNS Manager",
                                     "Proxy Manager", "Advanced Tab Manager", "Advanced Download Manager"):
                        self.assertTrue(any(expected in title for title in titles), expected)
                    content.theme.setCurrentText("Dark")
                    self.assertEqual(window.mode, "dark")
                    self.assertEqual(window.preferences.value("appearance"), "dark")
                finally:
                    window.close()
                    app.processEvents()
                    destroy_qobject(window)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)



if __name__ == "__main__":
    unittest.main()

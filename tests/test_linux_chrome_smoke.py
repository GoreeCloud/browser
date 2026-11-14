"""Linux native toolbar, Settings and Webspace isolation smoke.

Opt-in, isolated temporary XDG paths. No external URLs or filter downloads.
"""
import json
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
        from PySide6.QtTest import QTest
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
                    self.assertFalse(window.isMaximized())
                    self.assertFalse(window.isFullScreen())
                    self.assertIn(
                        "F11", [shortcut.key().toString() for shortcut in window._shortcuts],
                    )
                    # Exercise Qt's real shortcut-event dispatcher from the
                    # focused address input, not only the callback method.
                    # Activate/focus before synthetic key events: headless
                    # Qt backends do not have an external window manager.
                    window.activateWindow()
                    window.address.setFocus()
                    QTest.qWait(100)
                    self.assertTrue(window.address.hasFocus())
                    QTest.keyClick(window.address, Qt.Key.Key_F11)
                    app.processEvents()
                    self.assertTrue(window.isFullScreen())
                    QTest.keyClick(window.address, Qt.Key.Key_F11)
                    app.processEvents()
                    self.assertFalse(window.isFullScreen())
                    self.assertFalse(window.isMaximized())
                    # Method checks still cover maximize -> fullscreen ->
                    # maximized, independently of keyboard event routing.
                    window.showMaximized()
                    app.processEvents()
                    self.assertTrue(window.isMaximized())
                    window.toggle_fullscreen()
                    app.processEvents()
                    self.assertTrue(window.isFullScreen())
                    window.toggle_fullscreen()
                    app.processEvents()
                    self.assertTrue(window.isMaximized())
                    self.assertFalse(window.isFullScreen())
                    window.showNormal()
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
                    # Explicit opt-in: no window layout data is stored by
                    # default, and disabling memory clears its saved size.
                    self.assertFalse(content.remember_window_size.isChecked())
                    content.remember_window_size.setChecked(True)
                    self.assertTrue(window.remember_window_size)
                    self.assertTrue(window.preferences.value(
                        "remember_window_size", False, type=bool))
                    window.preferences.setValue("window_normal_size", "990x650")
                    content.remember_window_size.setChecked(False)
                    self.assertFalse(window.remember_window_size)
                    self.assertFalse(window.preferences.contains("window_normal_size"))
                    titles = [x.title() for x in content.findChildren(QGroupBox)]
                    for expected in ("Theme Manager", "ChatGPT.com", "Cookies Manager", "DNS Manager",
                                     "Proxy Manager", "Advanced Tab Manager", "Advanced Download Manager"):
                        self.assertTrue(any(expected in title for title in titles), expected)
                    # Corrupted persisted bookmark titles are normalized
                    # without discarding otherwise-valid saved destinations.
                    window.preferences.setValue("bookmarks", json.dumps([
                        {"url": "https://valid.example", "title": 42,
                         "legacy_marker": "preserve"},
                        {"url": "https://other.example",
                         "title": "  Verified\u202e\n Website "},
                        {"url": 14, "title": "invalid destination"},
                    ]))
                    saved = window.read_bookmarks()
                    self.assertEqual(len(saved), 2)
                    self.assertEqual(saved[0]["title"], "https://valid.example")
                    self.assertEqual(saved[0]["legacy_marker"], "preserve")
                    self.assertEqual(saved[1]["title"], "Verified Website")
                    window.populate_bookmarks()
                    menu_titles = [action.text() for action in window.bookmark_menu.actions()]
                    self.assertIn("https://valid.example", menu_titles)
                    self.assertIn("Verified Website", menu_titles)
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

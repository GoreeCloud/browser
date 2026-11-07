"""Linux Qt regression: native start-page controls must not overlap at 700x480."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux Qt native layout check",
)
class NativeStartLayoutSmoke(unittest.TestCase):
    def test_narrow_window_scrolls_without_covering_controls(self):
        from PySide6.QtCore import QPoint, QRect
        from PySide6.QtWidgets import QApplication, QLabel, QPushButton, QFrame
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow

        with tempfile.TemporaryDirectory(prefix="gcb-layout-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "profile"),
            }):
                app = QApplication([])
                window = BrowserWindow()
                try:
                    window.resize(700, 480)
                    window.show()
                    app.processEvents()
                    tab = window.current()
                    self.assertEqual(tab.current_address(), "goree://new-tab")

                    for page, scroll, action in (
                        (tab.new, tab.new_scroll, "Go home"),
                        (tab.home, tab.home_scroll, "New tab"),
                    ):
                        if page is tab.home:
                            window.show_home()
                        app.processEvents()
                        self.assertGreater(
                            scroll.verticalScrollBar().maximum(), 0,
                            "Narrow start page must be vertically scrollable.",
                        )
                        hero = page.findChild(QFrame, "Hero")
                        search = page.search
                        explore = next(
                            b for b in page.findChildren(QPushButton)
                            if b.text() == "Explore"
                        )
                        secondary = next(
                            b for b in page.findChildren(QPushButton)
                            if b.text() == action
                        )
                        search_rect = QRect(search.mapTo(hero, QPoint(0, 0)), search.size())
                        explore_rect = QRect(explore.mapTo(hero, QPoint(0, 0)), explore.size())
                        secondary_rect = QRect(secondary.mapTo(hero, QPoint(0, 0)), secondary.size())
                        self.assertLess(search_rect.bottom(), explore_rect.top())
                        self.assertLess(search_rect.bottom(), secondary_rect.top())
                        self.assertGreaterEqual(explore_rect.left(), 0)

                        scroll.verticalScrollBar().setValue(scroll.verticalScrollBar().maximum())
                        app.processEvents()
                        note = page.findChild(QLabel, "FinePrint")
                        relative = note.mapTo(scroll.viewport(), QPoint(0, 0))
                        self.assertGreaterEqual(relative.y(), 0)
                        self.assertLessEqual(
                            relative.y() + note.height(), scroll.viewport().height()
                        )
                    self.assertEqual(tab.current_address(), "goree://home")
                finally:
                    window.close()
                    app.processEvents()
                    destroy_qobject(window)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

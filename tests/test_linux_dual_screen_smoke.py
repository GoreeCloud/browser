"""Opt-in native Qt two-screen startup smoke on disposable Xvfb displays.

The CI step supplies separate 1920x1080 and 800x600 X screens. This test
never touches a real browser profile, live desktop or Internet resource.
"""

from __future__ import annotations

import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    sys.platform.startswith("linux")
    and os.environ.get("GC_BROWSER_QT_SMOKE") == "1"
    and os.environ.get("GC_BROWSER_TWO_SCREENS") == "1",
    "Explicit private dual-screen native Qt smoke only",
)
class DualScreenStartupSmoke(unittest.TestCase):
    def test_normal_first_launch_and_remembered_size_fit_smallest_display(self):
        from PySide6.QtWidgets import QApplication
        from shiboken6 import delete as destroy_qobject

        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.windowing import restrictive_work_area

        with tempfile.TemporaryDirectory(prefix="gcb-dualscreen-native-") as temp:
            root = Path(temp)
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(root / "config"),
                "XDG_DATA_HOME": str(root / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                try:
                    dimensions = [
                        (screen.availableGeometry().width(),
                         screen.availableGeometry().height())
                        for screen in app.screens()
                    ]
                    self.assertEqual(len(dimensions), 2, dimensions)
                    self.assertEqual(
                        set(dimensions), {(1920, 1080), (800, 600)},
                    )
                    self.assertEqual(restrictive_work_area(dimensions), (800, 600))
                    window = BrowserWindow()
                    try:
                        self.assertEqual(
                            (window.width(), window.height()), (720, 540),
                        )
                        window.show()
                        app.processEvents()
                        self.assertFalse(window.isMaximized())
                        self.assertFalse(window.isFullScreen())
                        window.set_remember_window_size(True)
                        # The user may manually choose a larger normal size;
                        # reopening must clamp it to the smallest work area.
                        window.resize(1100, 700)
                        app.processEvents()
                    finally:
                        window.close()
                        app.processEvents()
                        destroy_qobject(window)

                    restored = BrowserWindow()
                    try:
                        self.assertTrue(restored.remember_window_size)
                        self.assertEqual(
                            (restored.width(), restored.height()), (720, 540),
                        )
                        self.assertFalse(restored.isMaximized())
                        self.assertFalse(restored.isFullScreen())
                    finally:
                        restored.close()
                        app.processEvents()
                        destroy_qobject(restored)
                finally:
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

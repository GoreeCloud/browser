"""Native manager smoke with all network access disabled."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Linux-only native manager test",
)
class NativePanelsSmoke(unittest.TestCase):
    def test_managers_are_usable_without_remote_pages(self):
        from PySide6.QtWidgets import QApplication, QLabel
        from PySide6.QtCore import Qt
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.native_panels import (
            WebspacesDialog, TabManagerDialog, DownloadManagerDialog,
            CookieManagerDialog,
        )

        with tempfile.TemporaryDirectory(prefix="gcb-panels-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                window = BrowserWindow()
                try:
                    window.show()
                    ident = window.add_webspace("Work")
                    window.new_tab(space_id=ident)
                    app.processEvents()

                    spaces = WebspacesDialog(window)
                    spaces.spaces.setCurrentRow(1)
                    spaces.open_selected()
                    self.assertEqual(window.current().space_id, ident)
                    self.assertEqual(window.tabs.count(), 3)
                    destroy_qobject(spaces)

                    tabs = TabManagerDialog(window)
                    tabs.search.setText("Work")
                    self.assertEqual(tabs.list.count(), 2)
                    tabs.list.setCurrentRow(0)
                    tabs.switch_selected()
                    self.assertEqual(window.current().space_id, ident)
                    destroy_qobject(tabs)

                    with patch("goreecloud_browser.native_panels.find_clamscan",
                               return_value=None):
                        downloads = DownloadManagerDialog(window)
                    downloads.refresh()
                    self.assertEqual(downloads.table.rowCount(), 0)
                    scanner_labels = [label.text() for label in downloads.findChildren(QLabel)]
                    self.assertTrue(any("scanner: unavailable" in text.lower()
                                        and "explicit approval" in text
                                        for text in scanner_labels))
                    downloads.close()
                    destroy_qobject(downloads)

                    cookies = CookieManagerDialog(window)
                    self.assertEqual(cookies.table.columnCount(), 3)
                    cookies.close()
                    destroy_qobject(cookies)
                finally:
                    window.close()
                    app.processEvents()
                    destroy_qobject(window)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

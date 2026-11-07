"""Qt request-interceptor integration with only local, validated fake lists."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in Linux interceptor check",
)
class FilterInterceptorSmoke(unittest.TestCase):
    def test_enabled_source_rules_block_third_party_subresources(self):
        from PySide6.QtCore import QUrl, QStandardPaths
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import QWebEngineUrlRequestInfo
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.filters import save_validated_cache

        class RequestInfo:
            def __init__(self, host, first_party="site.example"):
                self.host = host
                self.first_party = first_party
                self.blocked = False
            def requestUrl(self):
                return QUrl(f"https://{self.host}/pixel.gif")
            def firstPartyUrl(self):
                return QUrl(f"https://{self.first_party}/")
            def resourceType(self):
                return QWebEngineUrlRequestInfo.ResourceType.ResourceTypeImage
            def block(self, value):
                self.blocked = bool(value)

        with tempfile.TemporaryDirectory(prefix="gcb-interceptor-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "settings"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                filter_root = Path(QStandardPaths.writableLocation(
                    QStandardPaths.StandardLocation.AppDataLocation
                )) / "filter-lists"
                oisd = "".join(f"||oisd{n}.example.com^\n" for n in range(520)).encode()
                steven = "".join(f"0.0.0.0 steven{n}.example.org\n" for n in range(520)).encode()
                save_validated_cache(filter_root, "oisd-small", oisd)
                save_validated_cache(filter_root, "stevenblack", steven)
                w = BrowserWindow()
                try:
                    w.show()
                    self.assertTrue(w.list_enabled("oisd-small"))
                    self.assertTrue(w.list_enabled("stevenblack"))
                    self.assertEqual([len(x) for x in w.interceptor.domain_rules], [520, 520])
                    for host in ("sub.oisd0.example.com", "steven10.example.org"):
                        info = RequestInfo(host)
                        w.interceptor.interceptRequest(info)
                        self.assertTrue(info.blocked, host)
                    # The curated subscription switches are separate from the
                    # tiny built-in fallback tracker-domain toggle.
                    w.set_preference("block_trackers", False)
                    info = RequestInfo("oisd0.example.com")
                    w.interceptor.interceptRequest(info)
                    self.assertTrue(info.blocked)
                    w.set_list_enabled("oisd-small", False)
                    info = RequestInfo("oisd0.example.com")
                    w.interceptor.interceptRequest(info)
                    self.assertFalse(info.blocked)
                    info = RequestInfo("steven1.example.org", "steven1.example.org")
                    w.interceptor.interceptRequest(info)
                    self.assertFalse(info.blocked, "First-party host should be allowed")
                finally:
                    w.close()
                    app.processEvents()
                    destroy_qobject(w)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

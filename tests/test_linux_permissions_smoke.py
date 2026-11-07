"""Linux WebEngine permission decision review and revocation acceptance.

No external navigation, live device capture or private user profile is used.
The app runs with a temporary XDG profile and all background list fetches off.
"""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


@unittest.skipUnless(
    os.name == "posix" and os.environ.get("GC_BROWSER_QT_SMOKE") == "1",
    "Opt-in native Linux permission-center smoke",
)
class PermissionCenterSmoke(unittest.TestCase):
    def test_review_revoke_and_webspace_isolation(self):
        from PySide6.QtCore import QUrl
        from PySide6.QtWidgets import QApplication, QMessageBox
        from PySide6.QtWebEngineCore import QWebEnginePermission
        from shiboken6 import delete as destroy_qobject
        from goreecloud_browser.app import BrowserWindow
        from goreecloud_browser.permissions import PermissionCenterDialog, origin_key, display_origin

        with tempfile.TemporaryDirectory(prefix="gcb-permissions-") as tmp:
            with patch.dict(os.environ, {
                "XDG_CONFIG_HOME": str(Path(tmp) / "config"),
                "XDG_DATA_HOME": str(Path(tmp) / "data"),
                "GC_BROWSER_FILTER_NETWORK": "off",
            }):
                app = QApplication([])
                window = BrowserWindow()
                center = None
                try:
                    window.show()
                    default = window.current()
                    ident = window.add_webspace("Work")
                    window.new_tab(space_id=ident)
                    default_profile = default.profile
                    work_profile = window.current().profile
                    self.assertIsNot(default_profile, work_profile)
                    geolocation = QWebEnginePermission.PermissionType.Geolocation
                    notifications = QWebEnginePermission.PermissionType.Notifications
                    alpha = QUrl("https://alpha.example")
                    beta = QUrl("https://beta.example")
                    permission_a = default_profile.queryPermission(alpha, geolocation)
                    permission_b = default_profile.queryPermission(beta, notifications)
                    permission_work = work_profile.queryPermission(alpha, geolocation)
                    permission_a.grant()
                    permission_b.deny()
                    permission_work.grant()
                    self.assertEqual(permission_a.state().name, "Granted")
                    self.assertEqual(permission_b.state().name, "Denied")
                    self.assertEqual(permission_work.state().name, "Granted")

                    # The active Work tab must only enumerate Work decisions.
                    center = PermissionCenterDialog(window)
                    self.assertEqual(center.space_id, ident)
                    self.assertEqual(center.table.rowCount(), 1)
                    self.assertIn("alpha.example", center.table.item(0, 0).text())
                    center.table.selectRow(0)
                    center.reset_selected()
                    self.assertEqual(permission_work.state().name, "Ask")
                    self.assertEqual(permission_a.state().name, "Granted")
                    self.assertEqual(center.table.rowCount(), 0)
                    center.close()
                    destroy_qobject(center)
                    center = None

                    # Default Webspace: the requesting *origin*, not URL path,
                    # determines what counts as this website.
                    window.tabs.setCurrentWidget(default)
                    with patch.object(default, "current_address",
                                      return_value="https://alpha.example/secret-path?token=dont-show"):
                        center = PermissionCenterDialog(window)
                    self.assertTrue(center.site_only.isChecked())
                    self.assertEqual(center.table.rowCount(), 1)
                    self.assertNotIn("secret-path", center.table.item(0, 0).text())
                    self.assertNotIn("dont-show", center.table.item(0, 0).text())
                    self.assertEqual(center.table.item(0, 1).text(), "Geolocation")
                    center.site_only.setChecked(False)
                    self.assertEqual(center.table.rowCount(), 2)
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.No):
                        center.reset_all()
                    self.assertEqual(permission_a.state().name, "Granted")
                    self.assertEqual(permission_b.state().name, "Denied")
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.Yes):
                        center.reset_all()
                    self.assertEqual(permission_a.state().name, "Ask")
                    self.assertEqual(permission_b.state().name, "Ask")
                    self.assertEqual(center.table.rowCount(), 0)
                    # The actual toolbar affordance opens this native center.
                    with patch("goreecloud_browser.app.PermissionCenterDialog.exec",
                               return_value=0) as opened:
                        window.permissions_button.click()
                    opened.assert_called_once_with()
                    # Keep private paths and embedded credentials out of all
                    # origin strings presented in permission prompts.
                    displayed = display_origin(QUrl(
                        "https://username:secret@example.org:443/private?q=sensitive#fragment"))
                    self.assertEqual(displayed, "https://example.org")
                    self.assertEqual(display_origin(QUrl("https://example.org:8443/path")),
                                     "https://example.org:8443")
                    self.assertIsNone(origin_key(QUrl("goree://home")))
                    self.assertNotEqual(origin_key(QUrl("http://alpha.example")),
                                        origin_key(QUrl("https://alpha.example")))
                    self.assertEqual(origin_key(QUrl("https://alpha.example:443/home")),
                                     origin_key(QUrl("https://alpha.example/path")))
                    # Permission prompts disclose the full requesting origin,
                    # not a potentially misleading bare host or secret path.
                    prompt = default.view.page().ask_permission
                    permission_prompt = default_profile.queryPermission(
                        QUrl("https://thirdparty.example/"), notifications)
                    with patch.object(QMessageBox, "question",
                                      return_value=QMessageBox.StandardButton.No) as asked:
                        prompt(permission_prompt)
                    self.assertEqual(permission_prompt.state().name, "Denied")
                    shown = asked.call_args.args[2]
                    self.assertIn("Requesting origin: https://thirdparty.example/", shown)
                    self.assertIn("Webspace: Default", shown)
                    self.assertIn("Top-level website: No website loaded", shown)
                    self.assertNotIn("secret-path", shown)
                    self.assertNotIn("dont-show", shown)
                finally:
                    if center is not None:
                        center.close()
                        destroy_qobject(center)
                    window.close()
                    app.processEvents()
                    destroy_qobject(window)
                    app.quit()
                    app.processEvents()
                    destroy_qobject(app)


if __name__ == "__main__":
    unittest.main()

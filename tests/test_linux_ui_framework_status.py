"""Guard truthful Linux primary UI framework compliance reporting.

This intentionally checks the currently active application entrypoint.
When an approved replacement is implemented, replace these legacy
assertions with source, package and runtime-backed compliance tests.
"""
import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class LinuxPrimaryUiFrameworkStatusTests(unittest.TestCase):
    def test_legacy_primary_ui_is_explicitly_marked_migration_required(self):
        state = json.loads(
            (ROOT / "docs/linux-ui-framework-status.json").read_text(encoding="utf-8")
        )
        app = (ROOT / "goreecloud_browser/app.py").read_text(encoding="utf-8")
        project = (ROOT / "pyproject.toml").read_text(encoding="utf-8")

        self.assertEqual(state["platform"], "linux")
        self.assertEqual(state["currentPrimaryUi"], "qt-widgets")
        self.assertEqual(state["status"], "migration-required")
        self.assertIs(state["grandfathered"], False)
        self.assertFalse(state["runtimeTargetVerified"])
        self.assertFalse(state["releasedCompliant"])
        self.assertNotIn(state["currentPrimaryUi"], state["approvedPrimaryUi"])
        self.assertIn("from PySide6.QtWidgets import", app)
        self.assertIn("from PySide6.QtWebEngineWidgets import QWebEngineView", app)
        self.assertIn('PySide6==6.11.3', project)

    def test_target_requires_actual_qt_quick_version_not_a_widget_shell(self):
        state = json.loads(
            (ROOT / "docs/linux-ui-framework-status.json").read_text(encoding="utf-8")
        )
        self.assertIn(state["proposedTarget"], state["approvedPrimaryUi"])
        self.assertEqual(state["proposedTarget"], "qt-quick-6.12.0")
        self.assertEqual(state["targetStatus"], "design-and-feasibility-only")
        docs = (ROOT / state["plan"]).read_text(encoding="utf-8")
        self.assertIn("Qt Quick on Qt **6.12.0**", docs)
        self.assertIn("not", docs.lower())
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("Linux UI framework compliance: migration required", readme)


if __name__ == "__main__":
    unittest.main()

"""Offline contract tests for the explicit Linux Qt WebEngine patch baseline."""
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from scripts.linux.check_webengine_patch_baseline import (
    EngineBaselineError,
    EngineVersions,
    pinned_binding,
    validate_engine,
    version_tuple,
)


@unittest.skipUnless(sys.version_info >= (3, 11), "CI baseline tool uses Python 3.11+")
class EnginePatchBaselineTests(unittest.TestCase):
    def setUp(self):
        self.good = EngineVersions(
            "6.11.3", "6.11.3", "140.0.7339.264", "155.0.8059.85"
        )

    def test_reviewed_exact_runtime_passes(self):
        validate_engine("6.11.3", self.good)

    def test_newer_reported_security_patch_passes(self):
        changed = EngineVersions("6.11.3", "6.11.3", "140.0.7339.264", "155.0.8059.90")
        validate_engine("6.11.3", changed)

    def test_lower_security_patch_is_refused(self):
        changed = EngineVersions("6.11.3", "6.11.3", "140.0.7339.264", "155.0.8059.84")
        with self.assertRaisesRegex(EngineBaselineError, "regressed"):
            validate_engine("6.11.3", changed)

    def test_wrong_chromium_base_is_refused(self):
        changed = EngineVersions("6.11.3", "6.11.3", "140.0.7339.263", "155.0.8059.85")
        with self.assertRaisesRegex(EngineBaselineError, "source/base"):
            validate_engine("6.11.3", changed)

    def test_unreviewed_qt_upgrade_is_refused(self):
        changed = EngineVersions("6.12.0", "6.12.0", "140.0.7339.264", "155.0.8059.85")
        with self.assertRaisesRegex(EngineBaselineError, "no reviewed Chromium baseline"):
            validate_engine("6.12.0", changed)

    def test_mismatched_binding_refused(self):
        changed = EngineVersions("6.11.2", "6.11.3", "140.0.7339.264", "155.0.8059.85")
        with self.assertRaisesRegex(EngineBaselineError, "differs"):
            validate_engine("6.11.3", changed)

    def test_mismatched_qt_webengine_refused(self):
        changed = EngineVersions("6.11.3", "6.11.2", "140.0.7339.264", "155.0.8059.85")
        with self.assertRaisesRegex(EngineBaselineError, "differs"):
            validate_engine("6.11.3", changed)

    def test_malformed_version_is_not_accepted(self):
        for value in ("155.beta", "155.0.8059", "155.0.8059.85;rm", "-1.0.0.0", ""):
            with self.subTest(value=value):
                self.assertRaises(EngineBaselineError, version_tuple, value, 4)

    def test_real_pinned_repo_manifest(self):
        project = Path(__file__).resolve().parents[1]
        self.assertEqual(pinned_binding(project), "6.11.3")

    def test_manifest_drift_rejected(self):
        with tempfile.TemporaryDirectory(prefix="gcb-qt-manifest-drift-") as temp:
            repo = Path(temp)
            (repo / "pyproject.toml").write_text(
                '[project]\ndependencies = ["PySide6==6.11.3"]\n', encoding="utf-8"
            )
            (repo / "requirements.txt").write_text("PySide6==6.11.2\n", encoding="utf-8")
            with self.assertRaisesRegex(EngineBaselineError, "pins disagree"):
                pinned_binding(repo)
            (repo / "requirements.txt").write_text("PySide6==6.11.3\n", encoding="utf-8")
            self.assertEqual(pinned_binding(repo), "6.11.3")
            (repo / "pyproject.toml").write_text(
                '[project]\ndependencies = ["PySide6>=6.11"]\n', encoding="utf-8"
            )
            with self.assertRaisesRegex(EngineBaselineError, "version-pinned"):
                pinned_binding(repo)

    def test_additional_undeclared_direct_dependency_requires_review(self):
        with tempfile.TemporaryDirectory(prefix="gcb-extra-dep-") as temp:
            repo = Path(temp)
            (repo / "pyproject.toml").write_text(
                '[project]\ndependencies = ["PySide6==6.11.3", "another-lib==1"]\n',
                encoding="utf-8",
            )
            (repo / "requirements.txt").write_text("PySide6==6.11.3\n", encoding="utf-8")
            with self.assertRaisesRegex(EngineBaselineError, "Unexpected direct dependency"):
                pinned_binding(repo)


if __name__ == "__main__":
    unittest.main()

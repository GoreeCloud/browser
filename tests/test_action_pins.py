"""Regression checks for immutable, least-privilege GitHub Actions policy."""
from pathlib import Path
import tempfile
import unittest

from scripts.linux.check_action_pins import (
    APPROVED_ACTIONS,
    ActionPinError,
    validate_directory,
    validate_workflow,
)


class ActionPinPolicyTests(unittest.TestCase):
    def setUp(self):
        self.verified = (
            "name: Browser source checks\n"
            "on:\n  push:\n  pull_request:\n"
            "permissions:\n  contents: read\n"
            "jobs:\n  test:\n    steps:\n"
            "      - uses: actions/checkout@" + APPROVED_ACTIONS["actions/checkout"] + "\n"
            "        with:\n          persist-credentials: false\n"
            "      - uses: actions/setup-python@" +
            APPROVED_ACTIONS["actions/setup-python"] + "\n"
        )

    def test_real_repo_workflow_is_verified(self):
        root = Path(__file__).resolve().parents[1] / ".github" / "workflows"
        count, reviewed = validate_directory(root)
        self.assertGreaterEqual(count, 1)
        self.assertEqual(reviewed, set(APPROVED_ACTIONS))

    def test_reviewed_full_sha_refs_pass(self):
        self.assertEqual(
            validate_workflow(self.verified, "check.yml"),
            set(APPROVED_ACTIONS)
        )

    def test_movable_version_tags_refused(self):
        for movable in ("v4", "main", "master", "releases/v4", "v4.2.2"):
            with self.subTest(ref=movable), self.assertRaisesRegex(
                ActionPinError, "full immutable commit SHA"
            ):
                validate_workflow(
                    self.verified.replace(
                        "actions/checkout@" + APPROVED_ACTIONS["actions/checkout"],
                        "actions/checkout@" + movable
                    ),
                    "check.yml"
                )

    def test_unreviewed_sha_refused(self):
        with self.assertRaisesRegex(ActionPinError, "unreviewed action or commit"):
            validate_workflow(
                self.verified.replace(
                    APPROVED_ACTIONS["actions/checkout"], "a" * 40
                ),
                "check.yml"
            )

    def test_unknown_action_even_with_sha_refused(self):
        with self.assertRaisesRegex(ActionPinError, "unreviewed action or commit"):
            validate_workflow(
                self.verified + "      - uses: unknown/uploader@" + "b" * 40 + "\n",
                "check.yml"
            )

    def test_unpinned_reusable_workflow_refused(self):
        with self.assertRaisesRegex(ActionPinError, "full immutable commit SHA"):
            validate_workflow(
                self.verified + "    uses: unknown/repo/.github/workflows/release.yml@main\n",
                "check.yml"
            )

    def test_write_permission_refused(self):
        for replacement in ("contents: write", "packages: write", "contents: write # extra"):
            with self.subTest(replacement=replacement), self.assertRaises(ActionPinError):
                validate_workflow(
                    self.verified.replace("contents: read", replacement),
                    "check.yml"
                )

    def test_permission_shortcuts_and_empty_map_refused(self):
        for replacement in ("permissions: write-all", "permissions: {}", "permissions:\n"):
            with self.subTest(replacement=replacement), self.assertRaises(ActionPinError):
                validate_workflow(self.verified.replace(
                    "permissions:\n  contents: read\n", replacement
                ), "check.yml")

    def test_absent_permissions_refused(self):
        with self.assertRaisesRegex(ActionPinError, "top-level"):
            validate_workflow(
                self.verified.replace("permissions:\n  contents: read\n", ""),
                "check.yml"
            )

    def test_job_level_override_write_refused(self):
        with self.assertRaisesRegex(ActionPinError, "read or none"):
            validate_workflow(
                self.verified +
                "    permissions:\n      contents: write\n",
                "check.yml"
            )

    def test_block_alias_refused(self):
        with self.assertRaisesRegex(ActionPinError, "full immutable commit SHA"):
            validate_workflow(self.verified + "      - uses: *update-action\n", "check.yml")

    def test_unsupported_inline_uses_syntax_refused(self):
        with self.assertRaisesRegex(ActionPinError, "unsupported uses syntax"):
            validate_workflow(
                self.verified + "      - {uses: unknown/tool@main}\n",
                "check.yml"
            )

    def test_missing_top_level_contents_read_refused(self):
        with self.assertRaisesRegex(ActionPinError, "contents: read"):
            validate_workflow(
                self.verified.replace("contents: read", "actions: read"),
                "check.yml"
            )

    def test_checkout_default_credentials_are_refused(self):
        cases = (
            "        with:\n          persist-credentials: false\n",
            "          persist-credentials: false\n",
        )
        for removed in cases:
            with self.subTest(removed=removed), self.assertRaisesRegex(
                ActionPinError, "checkout must disable persisted credentials"
            ):
                validate_workflow(self.verified.replace(removed, ""), "check.yml")

    def test_checkout_credential_true_or_ambiguous_refused(self):
        for substituted in (
            "persist-credentials: true", "persist-credentials: 'false'",
            "persist-credentials: ${OFF}", "persist-credentials: False",
        ):
            with self.subTest(value=substituted), self.assertRaisesRegex(
                ActionPinError, "checkout must disable persisted credentials"
            ):
                validate_workflow(self.verified.replace(
                    "persist-credentials: false", substituted
                ), "check.yml")

    def test_duplicate_checkout_credential_setting_refused(self):
        bad = self.verified.replace(
            "persist-credentials: false\n",
            "persist-credentials: false\n          persist-credentials: true\n"
        )
        with self.assertRaisesRegex(ActionPinError, "duplicate checkout credential"):
            validate_workflow(bad, "check.yml")

    def test_comment_not_action(self):
        text = self.verified + "      # - uses: unverified/tool@main\n"
        self.assertEqual(validate_workflow(text, "check.yml"), set(APPROVED_ACTIONS))

    def test_empty_directory_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ActionPinError, "No GitHub workflow"):
                validate_directory(Path(temp))

    def test_symlinked_workflow_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            first = root / "check.yml"
            first.write_text(self.verified, encoding="utf-8")
            (root / "linked.yml").symlink_to(first)
            with self.assertRaisesRegex(ActionPinError, "Symlink workflow refused"):
                validate_directory(root)

    def test_reviewed_shas_are_not_movable_tags(self):
        self.assertEqual(
            APPROVED_ACTIONS["actions/checkout"],
            "11d5960a326750d5838078e36cf38b85af677262"
        )
        self.assertEqual(
            APPROVED_ACTIONS["actions/setup-python"],
            "a26af69be951a213d495a4c3e4e4022e16d87065"
        )


if __name__ == "__main__":
    unittest.main()

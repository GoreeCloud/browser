"""Offline regression checks for /proc Qt WebEngine process role redaction.

No Qt process, browser profile, system setting or network access is needed.
"""
import unittest

from scripts.linux.audit_webengine_sandbox import safe_webengine_role


class SafeRoleLabelTests(unittest.TestCase):
    def test_known_process_roles(self):
        for role in ("renderer", "gpu-process", "utility", "zygote", "broker"):
            with self.subTest(role=role):
                raw = b"/usr/bin/QtWebEngineProcess\x00--type=" + role.encode() + b"\x00"
                self.assertEqual(safe_webengine_role(raw), role)

    def test_no_type_is_unclassified(self):
        self.assertEqual(
            safe_webengine_role(b"/usr/bin/QtWebEngineProcess\x00--no-sandbox\x00"),
            "unspecified",
        )
        self.assertEqual(safe_webengine_role(b""), "unspecified")

    def test_unrecognized_or_sensitive_value_never_emitted(self):
        raw = b"QtWebEngineProcess\x00--type=private-user-token\x00"
        self.assertEqual(safe_webengine_role(raw), "unrecognized")
        self.assertEqual(safe_webengine_role(b"--type=\xffsecret\x00"), "unrecognized")

    def test_duplicate_or_conflicting_roles_fail_closed(self):
        self.assertEqual(
            safe_webengine_role(b"--type=renderer\x00--type=gpu-process\x00"),
            "ambiguous",
        )
        self.assertEqual(
            safe_webengine_role(b"--type=renderer\x00--type=renderer\x00"),
            "ambiguous",
        )

    def test_command_line_is_nul_delimited_not_string_matched(self):
        self.assertEqual(
            safe_webengine_role(b"--extension=--type=renderer\x00--type=utility\x00"),
            "utility",
        )
        self.assertEqual(
            safe_webengine_role(b"--type=renderer extra-private-text\x00"),
            "unrecognized",
        )

    def test_invalid_data_stays_unclassified(self):
        self.assertEqual(safe_webengine_role(None), "unreadable")
        self.assertEqual(safe_webengine_role("--type=renderer"), "unreadable")


if __name__ == "__main__":
    unittest.main()

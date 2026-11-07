"""Webspace identity/path validation; no Qt or network."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from goreecloud_browser.webspaces import (
    DEFAULT_ID, create_space, decode_spaces, encode_spaces, storage_paths, clean_label,
)


class WebspacesTests(unittest.TestCase):
    def test_empty_registry_and_default_path_compatibility(self):
        self.assertEqual(decode_spaces("[]"), [])
        self.assertEqual(storage_paths(Path("/tmp/space"), DEFAULT_ID),
                         (Path("/tmp/space/web-profile"), Path("/tmp/space/cache")))

    def test_create_and_reopen_registry(self):
        entries = create_space([], "  Work   Projects  ")
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0]["name"], "Work Projects")
        self.assertEqual(decode_spaces(encode_spaces(entries)), entries)
        web, cache = storage_paths(Path("/tmp/space"), entries[0]["id"])
        self.assertTrue(str(web).endswith("/spaces/" + entries[0]["id"] + "/web-profile"))
        self.assertNotEqual(web, cache)

    def test_duplicate_and_invalid_names(self):
        entries = create_space([], "Work")
        for value in ("Work", "work", " Default ", "", "x" * 40, "\x00bad"):
            with self.subTest(name=value):
                with self.assertRaises(ValueError):
                    create_space(entries, value)

    def test_decode_rejects_injected_ids(self):
        content = json.dumps([
            {"id": "../outside", "name": "Bad"},
            {"id": "1234567890abcdef", "name": "Valid"},
            {"id": "1234567890abcdef", "name": "Duplicate"},
        ])
        self.assertEqual(decode_spaces(content), [{"id": "1234567890abcdef", "name": "Valid"}])
        with self.assertRaises(ValueError):
            storage_paths(Path("/tmp"), "../bad")

    def test_registry_limit(self):
        entries = [{"id": f"{n:016x}", "name": f"Space {n}"} for n in range(16)]
        self.assertEqual(len(decode_spaces(encode_spaces(entries))), 16)
        with self.assertRaises(ValueError):
            create_space(entries, "Overflow")


if __name__ == "__main__":
    unittest.main()

"""Offline tests for upstream release watch."""
import importlib.util
from pathlib import Path
import unittest

target = Path(__file__).resolve().parents[1] / "tools" / "upstream_watch.py"
spec = importlib.util.spec_from_file_location("upstream_watch", target)
watch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(watch)


class WatchTests(unittest.TestCase):
    def test_release_tuple(self):
        self.assertEqual(watch.release_tuple("158.0"), (158, 0, 0))

    def test_dot_release(self):
        self.assertGreater(watch.release_tuple("158.0.1"), watch.release_tuple("158.0"))

    def test_major_release(self):
        self.assertGreater(watch.release_tuple("159.0"), watch.release_tuple("158.0.9"))

    def test_beta_rejected(self):
        with self.assertRaises(ValueError):
            watch.release_tuple("159.0b1")

    def test_unknown_suffix_rejected(self):
        with self.assertRaises(ValueError):
            watch.release_tuple("158.0-not-a-release")

    def test_official_field(self):
        self.assertEqual(watch.newest_from_payload({"LATEST_FIREFOX_VERSION": "158.0"}), "158.0")

    def test_missing_field(self):
        with self.assertRaises(ValueError):
            watch.newest_from_payload({})


if __name__ == "__main__":
    unittest.main()

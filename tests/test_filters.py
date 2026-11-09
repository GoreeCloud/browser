"""Domain-subscription verification; no network is used in tests."""
from pathlib import Path
import os
import tempfile
import time
import unittest
from unittest.mock import patch

from goreecloud_browser.filters import (
    SOURCES, parse_subscription, is_blocked, read_cached, save_validated_cache,
    needs_update, SameOriginRedirects,
)


class FilterTests(unittest.TestCase):
    def test_oisd_domain_only_and_no_cosmetic_code(self):
        data = b"||ad.example.com^\n||tracker.example.net^\n@@||safe.example.net^\n##.ad-widget\n||bad.domain^$script\n"
        parsed = parse_subscription(data, "oisd-small")
        self.assertEqual(parsed, frozenset({"ad.example.com", "tracker.example.net"}))
        self.assertTrue(is_blocked("sub.ad.example.com", (parsed,)))
        self.assertFalse(is_blocked("ad.example.org", (parsed,)))

    def test_stevenblack_host_mapping_and_comments(self):
        data = b"0.0.0.0 ad.example.com tracker.example.org\n127.0.0.1 media.example.org\n# note\n0.0.0.0 localhost\n"
        parsed = parse_subscription(data, "stevenblack")
        self.assertIn("ad.example.com", parsed)
        self.assertIn("media.example.org", parsed)
        self.assertNotIn("localhost", parsed)
        self.assertFalse(is_blocked("example.com", (parsed,)))

    def test_empty_and_malformed_never_replace_last_good_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            valid = "".join(f"||domain{n}.example.com^\n" for n in range(560)).encode()
            original = save_validated_cache(root, "oisd-small", valid)
            self.assertEqual(len(original), 560)
            self.assertEqual(read_cached(root, "oisd-small"), original)
            with self.assertRaises(ValueError):
                save_validated_cache(root, "oisd-small", b"<html>Oops</html>")
            self.assertEqual(read_cached(root, "oisd-small"), original)
            self.assertEqual((root / "oisd-small.txt").stat().st_mode & 0o777, 0o600)

    def test_symlink_cache_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            outside = root / "outside"
            outside.write_bytes(b"secret")
            (root / "stevenblack.txt").symlink_to(outside)
            self.assertEqual(read_cached(root, "stevenblack"), frozenset())
            valid = "".join(f"0.0.0.0 domain{n}.example.com\n" for n in range(510)).encode()
            with self.assertRaises(ValueError):
                save_validated_cache(root, "stevenblack", valid)
            self.assertEqual(outside.read_bytes(), b"secret")

    def test_stale_cache_trigger(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertTrue(needs_update(root, "oisd-small", now=100000))
            data = "".join(f"||host{n}.example.net^\n" for n in range(520)).encode()
            save_validated_cache(root, "oisd-small", data)
            self.assertFalse(needs_update(root, "oisd-small", now=time.time()))

    def test_fresh_invalid_cache_is_never_treated_as_current(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "filters"
            good = "".join(f"||host{n}.example.net^\n" for n in range(520)).encode()
            rules = save_validated_cache(root, "oisd-small", good)
            now = time.time()
            self.assertFalse(needs_update(root, "oisd-small", now=now))
            self.assertFalse(needs_update(root, "oisd-small", now=now,
                                          loaded_domains=rules))
            cache = root / "oisd-small.txt"
            for invalid in (b"<html>captive portal</html>", b"||one.example.net^\n", b""):
                with self.subTest(invalid=invalid):
                    cache.write_bytes(invalid)
                    self.assertTrue(needs_update(root, "oisd-small", now=time.time()))
                    self.assertTrue(needs_update(root, "oisd-small", now=time.time(),
                                                 loaded_domains=read_cached(root, "oisd-small")))

    def test_future_mtime_cannot_delay_refresh(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "filters"
            rules = save_validated_cache(root, "oisd-small",
                "".join(f"||host{n}.example.net^\n" for n in range(520)).encode())
            now = time.time()
            os.utime(root / "oisd-small.txt", (now + 3600, now + 3600))
            self.assertTrue(needs_update(root, "oisd-small", now=now,
                                         loaded_domains=rules))

    def test_cache_root_symlink_is_never_read_as_trusted_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "real-filters"
            good = "".join(f"||host{n}.example.net^\n" for n in range(520)).encode()
            rules = save_validated_cache(root, "oisd-small", good)
            alias = Path(tmp) / "alias"
            alias.symlink_to(root, target_is_directory=True)
            self.assertEqual(read_cached(root, "oisd-small"), rules)
            self.assertEqual(read_cached(alias, "oisd-small"), frozenset())
            self.assertTrue(needs_update(alias, "oisd-small", now=time.time()))
            self.assertTrue(needs_update(alias, "oisd-small", now=time.time(),
                                         loaded_domains=rules))

    def test_preparsed_domains_avoid_another_gui_thread_parse(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "filters"
            rules = save_validated_cache(root, "oisd-small",
                "".join(f"||host{n}.example.net^\n" for n in range(520)).encode())
            with patch("goreecloud_browser.filters.read_cached",
                       side_effect=AssertionError("redundant parse")):
                self.assertFalse(needs_update(root, "oisd-small", now=time.time(),
                                              loaded_domains=rules))

    def test_https_origins_are_fixed(self):
        self.assertEqual(set(SOURCES), {"oisd-small", "stevenblack"})
        self.assertTrue(all(link.startswith("https://") for link in SOURCES.values()))
        handler = SameOriginRedirects()
        class Request:
            full_url = "https://small.oisd.nl/"
        with self.assertRaises(ValueError):
            handler.redirect_request(Request(), None, 302, "redirect", {}, "http://evil.test/")
        with self.assertRaises(ValueError):
            handler.redirect_request(Request(), None, 302, "redirect", {}, "https://elsewhere.test/")

    def test_oversize_and_unrecognized_rules(self):
        with self.assertRaises(ValueError):
            parse_subscription(b"a", "unknown")
        self.assertEqual(parse_subscription(b"/ad/ \n<script>\n||*.example.com^\n", "oisd-small"), frozenset())


if __name__ == "__main__":
    unittest.main()

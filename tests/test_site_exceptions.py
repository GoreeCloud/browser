"""Exact-host site-exception validation, no Qt or network."""
import json
import unittest

from goreecloud_browser.site_exceptions import (
    MAX_EXCEPTIONS, add_exception, decode_exceptions, encode_exceptions,
    exact_host, is_exempt_first_party, remove_exception,
)


class SiteExceptionTests(unittest.TestCase):
    def test_exact_hostname_only(self):
        self.assertEqual(exact_host("  News.Example.org.  "), "news.example.org")
        self.assertEqual(exact_host("bücher.example"), "xn--bcher-kva.example")
        for value in (
            "https://news.example.org", "news.example.org/path",
            "*.example.org", "news..example.org", "-host.example",
            "user@news.example.org", "news.example.org:443",
            "127.0.0.1", "", "localhost", "bad name.example",
            "news.example.org\u202E", "\x00news.example.org",
        ):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    exact_host(value)

    def test_numeric_ip_aliases_cannot_disable_privacy_shield(self):
        for value in (
            "127.1", "0177.0.0.1", "0x7f.0.0.1", "123.456",
            "１２７.1", "０１７７.0.0.1", "127。1",
        ):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    exact_host(value)
                with self.assertRaises(ValueError):
                    add_exception((), value)
                self.assertFalse(is_exempt_first_party(value, frozenset({value})))
        self.assertEqual(
            decode_exceptions(json.dumps(["127.1", "news.example.org",
                                          "0177.0.0.1", "１２７.1"])),
            ("news.example.org",),
        )
        # Legitimate internationalized DNS remains compatible.
        self.assertEqual(exact_host("４２.example.org"), "42.example.org")

    def test_roundtrip_persistence_only_exact_host(self):
        entries = add_exception((), " News.Example.org ")
        entries = add_exception(entries, "Another.example.org")
        self.assertEqual(entries, ("news.example.org", "another.example.org"))
        self.assertEqual(decode_exceptions(encode_exceptions(entries)), entries)
        with self.assertRaises(ValueError):
            add_exception(entries, "NEWS.example.org")
        self.assertEqual(remove_exception(entries, "news.example.org"),
                         ("another.example.org",))

    def test_corrupt_and_oversize_registries_fail_closed(self):
        for value in ("", "not-json", '{"host":"news.example.org"}',
                      json.dumps(["x.example"] * (MAX_EXCEPTIONS + 1)),
                      " " * 8300):
            self.assertEqual(decode_exceptions(value), ())
        encoded = json.dumps(["news.example.org", "*.other.test", 35,
                              "NEWS.EXAMPLE.ORG", "valid.other.test"])
        self.assertEqual(decode_exceptions(encoded),
                         ("news.example.org", "valid.other.test"))

    def test_exact_match_no_subdomain_or_suffix_bypass(self):
        rules = frozenset({"news.example.org"})
        self.assertTrue(is_exempt_first_party("NEWS.example.org.", rules))
        for value in ("sub.news.example.org", "news.example.org.evil.test",
                      "example.org", "other.example.org", ""):
            self.assertFalse(is_exempt_first_party(value, rules), value)

    def test_limit_and_deduplication(self):
        hosts = tuple(f"site{n}.example.org" for n in range(MAX_EXCEPTIONS))
        self.assertEqual(len(decode_exceptions(encode_exceptions(hosts))), MAX_EXCEPTIONS)
        with self.assertRaises(ValueError):
            add_exception(hosts, "another.example.org")
        with self.assertRaises(ValueError):
            encode_exceptions(("news.example.org", "NEWS.example.org"))


if __name__ == "__main__":
    unittest.main()

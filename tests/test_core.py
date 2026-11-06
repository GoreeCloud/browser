import unittest
from goreecloud_browser.core import (
    STARTPAGE_SEARCH, clean_tracking_parameters, destination_for,
    is_known_tracker, valid_web_url,
)


class NavigationTests(unittest.TestCase):
    def test_empty_is_native_new_tab(self):
        self.assertEqual(destination_for(" ").kind, "new-tab")

    def test_home_is_native(self):
        self.assertEqual(destination_for("goree://home").kind, "home")

    def test_bare_host_https_first(self):
        self.assertEqual(destination_for("example.com").value, "https://example.com")

    def test_localhost_https_first(self):
        self.assertEqual(destination_for("localhost:8000/a").value,
                         "https://localhost:8000/a")

    def test_explicit_http_allowed(self):
        self.assertEqual(destination_for("http://example.com/a").value,
                         "http://example.com/a")

    def test_search_uses_startpage(self):
        self.assertEqual(destination_for("private search query").value,
                         STARTPAGE_SEARCH + "private+search+query")

    def test_reject_javascript(self):
        with self.assertRaises(ValueError):
            destination_for("javascript:alert(1)")

    def test_reject_file_url(self):
        with self.assertRaises(ValueError):
            destination_for("file:///etc/passwd")

    def test_reject_userinfo(self):
        with self.assertRaises(ValueError):
            destination_for("https://user:pass@example.com/")

    def test_reject_bad_port(self):
        with self.assertRaises(ValueError):
            destination_for("https://example.com:bad/")

    def test_reject_backslash_url(self):
        with self.assertRaises(ValueError):
            destination_for(r"https://trusted.example\@evil.example/")

    def test_reject_backslash_bare_address(self):
        with self.assertRaises(ValueError):
            destination_for(r"example.com\@evil.example/")

    def test_reject_control_char(self):
        with self.assertRaises(ValueError):
            destination_for("https://example.com/\nfoo")


class PrivacyTests(unittest.TestCase):
    def test_remove_tracking_parameters(self):
        self.assertEqual(
            clean_tracking_parameters("https://example.com/p?utm_source=mail&item=2&fbclid=123#section"),
            "https://example.com/p?item=2#section",
        )

    def test_case_insensitive_parameters(self):
        self.assertEqual(clean_tracking_parameters("https://e.com/?UTM_Campaign=x&A=1"),
                         "https://e.com/?A=1")

    def test_preserve_noop_encoding(self):
        url = "https://e.com/?q=a%20b&c=1"
        self.assertEqual(clean_tracking_parameters(url), url)

    def test_preserve_signed_url(self):
        url = "https://storage.example/file?X-Amz-Signature=abcdef&utm_source=mail"
        self.assertEqual(clean_tracking_parameters(url), url)

    def test_preserve_non_http(self):
        self.assertEqual(clean_tracking_parameters("data:text/plain,utm_test=1"),
                         "data:text/plain,utm_test=1")

    def test_known_third_party_tracker(self):
        self.assertTrue(is_known_tracker("stats.google-analytics.com", "example.org"))

    def test_same_host_not_blocked(self):
        self.assertFalse(is_known_tracker("analytics.google.com", "analytics.google.com"))

    def test_unlisted_domain_not_blocked(self):
        self.assertFalse(is_known_tracker("good.example", "example.com"))

    def test_domain_suffix_boundary(self):
        self.assertFalse(is_known_tracker("notdoubleclick.net", "example.com"))


if __name__ == "__main__":
    unittest.main()

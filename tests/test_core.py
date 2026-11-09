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

    def test_cleaning_enabled_for_typed_address(self):
        self.assertEqual(
            destination_for("https://example.com/?utm_source=mail&q=a%20b").value,
            "https://example.com/?q=a%20b",
        )

    def test_cleaning_can_be_disabled_for_typed_address(self):
        original = "https://example.com/?utm_source=mail&q=a%20b"
        self.assertEqual(destination_for(original, clean_links=False).value, original)

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

    def test_reject_percent_encoded_host_dot(self):
        self.assertFalse(valid_web_url("https://example.com%2Eattacker.test/path"))
        with self.assertRaises(ValueError):
            destination_for("https://example.com%2Eattacker.test/path")

    def test_reject_percent_encoded_host_slash(self):
        self.assertFalse(valid_web_url("https://example.com%2fattacker.test/path"))

    def test_reject_empty_domain_label(self):
        self.assertFalse(valid_web_url("https://example..test/path"))

    def test_reject_bidi_override_in_url(self):
        self.assertFalse(valid_web_url("https://good.test\u202eevil.test/path"))

    def test_accept_internationalized_host_and_ipv6(self):
        self.assertTrue(valid_web_url("https://bücher.example/page"))
        self.assertTrue(valid_web_url("http://[::1]:8080/page"))

    def test_reject_legacy_numeric_ip_spellings(self):
        # Qt/Chromium canonicalizes e.g. 0177.0.0.1 and 127.1 to 127.0.0.1.
        for host in ("0177.0.0.1", "0x7f.0.0.1", "127.1",
                     "2130706433", "0x7f000001", "123.456",
                     "000127.0.0.1", "10.0.0.01", "127.0.0.999"):
            with self.subTest(host=host):
                self.assertFalse(valid_web_url("https://" + host + "/"))
                with self.assertRaises(ValueError):
                    destination_for("https://" + host + "/")
                if "." in host:
                    with self.assertRaises(ValueError):
                        destination_for(host)

    def test_reject_idna_mapped_numeric_ip_aliases(self):
        # Qt normalizes these to 127.0.0.1, but Python's ipaddress checks
        # alone do not recognize Unicode digits and Unicode dot separators.
        for host in (
            "０１７７.0.0.1", "１２７.1", "０x7f.0.0.1",
            "１２７.０.０.１", "127。0。0。1", "１２７．0．0．1",
        ):
            with self.subTest(host=host):
                self.assertFalse(valid_web_url("https://" + host + "/"))
                with self.assertRaises(ValueError):
                    destination_for("https://" + host + "/")
                with self.assertRaises(ValueError):
                    destination_for(host)

    def test_canonical_ips_and_numeric_dns_labels_still_work(self):
        for host in ("127.0.0.1", "1.2.3.4", "42.example.com",
                     "４２.example.com", "example.123"):
            with self.subTest(host=host):
                self.assertTrue(valid_web_url("https://" + host + "/"))
                self.assertEqual(destination_for(host).value, "https://" + host)

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

    def test_preserve_encoding_when_tracking_removed(self):
        original = ("https://e.com/path?utm_source=mail&q=a%20b&literal=a%2Bb"
                    "&raw=%26%3D&&utm_campaign=foo&empty=#fragment")
        expected = ("https://e.com/path?q=a%20b&literal=a%2Bb"
                    "&raw=%26%3D&&empty=#fragment")
        self.assertEqual(clean_tracking_parameters(original), expected)

    def test_encoded_tracking_name_removed_without_reencoding_values(self):
        self.assertEqual(
            clean_tracking_parameters("https://e.com/?%75tm_source=1&next=%2f%2F&item=0"),
            "https://e.com/?next=%2f%2F&item=0",
        )

    def test_preserve_signed_url_with_encoded_signature_name(self):
        original = "https://e.com/file?X-Amz-%53ignature=abc&utm_source=mail&q=a%20b"
        self.assertEqual(clean_tracking_parameters(original), original)

    def test_tracking_text_in_value_not_a_parameter_name(self):
        original = "https://e.com/?redirect=utm_source%3Dfoo&x=2"
        self.assertEqual(clean_tracking_parameters(original), original)

    def test_preserve_duplicate_keys_and_empty_value(self):
        self.assertEqual(
            clean_tracking_parameters("https://e.com/?a=1&fbclid=hi&a=2&b="),
            "https://e.com/?a=1&a=2&b=",
        )

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

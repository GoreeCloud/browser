"""Proxy settings: explicit modes only, no credentials stored."""
import unittest
from goreecloud_browser.proxy import ProxyConfig, validate_proxy


class ProxyTests(unittest.TestCase):
    def test_defaults(self):
        self.assertEqual(validate_proxy("system"), ProxyConfig("system"))
        self.assertEqual(validate_proxy("direct", "ignored", 99999), ProxyConfig("direct"))

    def test_valid_http_and_socks(self):
        self.assertEqual(validate_proxy("http", "proxy.example.com", 8080),
                         ProxyConfig("http", "proxy.example.com", 8080))
        self.assertEqual(validate_proxy("socks5", "::1", "9050"),
                         ProxyConfig("socks5", "::1", 9050))

    def test_reject_invalid_host_and_mode(self):
        for mode, host, port in (
            ("pac", "", 0), ("http", "https://evil.test", 8080),
            ("http", "host:9000", 9000),
            ("socks5", "space host", 1000),
            ("http", "-host.test", 3000),
            ("http", "host.test", 0),
            ("http", "host.test", 65536),
            ("http", "host.test", True),
        ):
            with self.subTest(mode=mode, host=host, port=port):
                with self.assertRaises(ValueError):
                    validate_proxy(mode, host, port)


if __name__ == "__main__":
    unittest.main()

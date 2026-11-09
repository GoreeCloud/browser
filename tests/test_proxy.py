"""Proxy settings: explicit modes only, no credentials stored."""
import unittest
from goreecloud_browser.proxy import ProxyConfig, validate_proxy, valid_proxy_host


class ProxyTests(unittest.TestCase):
    def test_defaults(self):
        self.assertEqual(validate_proxy("system"), ProxyConfig("system"))
        self.assertEqual(validate_proxy("direct", "ignored", 99999), ProxyConfig("direct"))

    def test_valid_http_and_socks(self):
        self.assertEqual(validate_proxy("http", "proxy.example.com", 8080),
                         ProxyConfig("http", "proxy.example.com", 8080))
        self.assertEqual(validate_proxy("socks5", "::1", "9050"),
                         ProxyConfig("socks5", "::1", 9050))

    def test_reject_ambiguous_numeric_proxy_endpoints(self):
        for host in (
            "0177.0.0.1", "0x7f.0.0.1", "127.1",
            "2130706433", "0x7f000001", "123.456",
            "10.0.0.01", "127.0.0.999", "００１２７.0.0.1",
        ):
            with self.subTest(host=host):
                with self.assertRaises(ValueError):
                    valid_proxy_host(host)
                with self.assertRaises(ValueError):
                    validate_proxy("http", host, 8080)

    def test_bracketed_proxy_host_requires_complete_numeric_address(self):
        self.assertEqual(valid_proxy_host("[::1]"), "::1")
        self.assertEqual(valid_proxy_host("[127.0.0.1]"), "127.0.0.1")
        for host in ("[::1", "::1]", "[[::1]]", "[proxy.example]", "[0177.0.0.1]"):
            with self.subTest(host=host):
                with self.assertRaises(ValueError):
                    valid_proxy_host(host)

    def test_canonical_hosts_and_short_numeric_port_limits(self):
        self.assertEqual(valid_proxy_host("127.0.0.1"), "127.0.0.1")
        self.assertEqual(valid_proxy_host("42.example.com"), "42.example.com")
        self.assertEqual(
            validate_proxy("socks5", "42.example.com", "00080"),
            ProxyConfig("socks5", "42.example.com", 80),
        )
        with self.assertRaises(ValueError):
            validate_proxy("http", "proxy.example.com", "8" * 10000)

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

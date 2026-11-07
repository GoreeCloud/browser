"""Pure Linux QtWebEngine explicit sandbox-bypass preflight tests."""
import unittest

from goreecloud_browser.sandbox import (
    ENV_DISABLE, ENV_FLAGS, WebEngineSecurityError,
    unsafe_webengine_settings, require_safe_webengine_settings,
)


class SandboxSettingsTests(unittest.TestCase):
    def test_safe_normal_qt_and_loopback_flags(self):
        for flags in (
            "",
            "--disable-gpu",
            '--disable-gpu --no-proxy-server --host-resolver-rules="MAP news.browser-test.invalid 127.0.0.1, MAP ads.browser-test.invalid 127.0.0.1"',
            "--enable-logging --log-level=2",
            "--enable-features=InterestGroupStorage",
            "--host-resolver-rules='MAP --no-sandbox.test 127.0.0.1'",
        ):
            with self.subTest(flags=flags):
                self.assertEqual(
                    unsafe_webengine_settings({ENV_FLAGS: flags}, ["--webEngineArgs"]),
                    (),
                )
                require_safe_webengine_settings({ENV_FLAGS: flags}, ["--webEngineArgs"])

    def test_documented_unsafe_sandbox_disable(self):
        for variable_value in ("1", "true", "0", "yes", ""):
            with self.subTest(value=variable_value):
                failures = unsafe_webengine_settings(
                    {ENV_DISABLE: variable_value}, []
                )
                self.assertEqual(failures, (ENV_DISABLE,))
                with self.assertRaises(WebEngineSecurityError):
                    require_safe_webengine_settings(
                        {ENV_DISABLE: variable_value}, []
                    )

    def test_refuse_known_unsafe_chromium_flags_in_env_or_cli(self):
        unsafe = (
            "--no-sandbox",
            "--no-sandbox=0",
            "--single-process",
            "--disable-seccomp-filter-sandbox",
            "--disable-namespace-sandbox",
            "--disable-gpu-sandbox",
            "--disable-setuid-sandbox",
            "--in-process-gpu",
            "--disable-site-isolation-trials",
            "--enable-features=NetworkServiceInProcess",
            "--enable-features=Other,NetworkServiceInProcess,Unrelated",
        )
        for flag in unsafe:
            with self.subTest(flag=flag):
                for environment, argv in (
                    ({ENV_FLAGS: f"--disable-gpu {flag}"}, []),
                    ({ENV_FLAGS: "--disable-gpu"}, ["--webEngineArgs", flag]),
                ):
                    with self.assertRaises(WebEngineSecurityError) as ctx:
                        require_safe_webengine_settings(environment, argv)
                    self.assertIn("refuses to start", str(ctx.exception))
                    self.assertIn("sandbox", str(ctx.exception).lower())

    def test_tls_and_web_origin_security_bypass_flags_refused(self):
        forbidden = (
            "--ignore-certificate-errors",
            "--ignore-certificate-errors-spki-list=abcdef",
            "--allow-insecure-localhost",
            "--allow-running-insecure-content",
            "--disable-web-security",
            "--unsafely-treat-insecure-origin-as-secure=http://example.org",
        )
        for flag in forbidden:
            with self.subTest(flag=flag):
                for environment, argv in (
                    ({ENV_FLAGS: f"--disable-gpu {flag}"}, []),
                    ({}, ["--webEngineArgs", flag]),
                ):
                    with self.assertRaises(WebEngineSecurityError) as ctx:
                        require_safe_webengine_settings(environment, argv)
                    self.assertIn("TLS", str(ctx.exception))

    def test_name_not_prefix_and_no_substring_false_positive(self):
        safe = (
            "--no-sandbox-extra",
            "--disallow-single-process",
            "--host-resolver-rules=MAP--no-sandbox",
            "--enable-features=NetworkService",
            "--disable-features=NetworkServiceInProcess",
        )
        for flag in safe:
            with self.subTest(flag=flag):
                self.assertEqual(
                    unsafe_webengine_settings({ENV_FLAGS: flag}, []), ()
                )

    def test_badly_quoted_flags_fail_closed_without_exposing_value(self):
        bad = '--host-resolver-rules="secret unclosed'
        found = unsafe_webengine_settings({ENV_FLAGS: bad}, [])
        self.assertEqual(found, ("malformed " + ENV_FLAGS,))
        with self.assertRaises(WebEngineSecurityError) as ctx:
            require_safe_webengine_settings({ENV_FLAGS: bad}, [])
        self.assertNotIn("secret", str(ctx.exception))

    def test_multiple_reasons_are_reported_without_raw_extra_flags(self):
        found = unsafe_webengine_settings(
            {
                ENV_DISABLE: "1",
                ENV_FLAGS: "--no-sandbox --enable-features=NetworkServiceInProcess",
            },
            ["--webEngineArgs", "--single-process"],
        )
        self.assertIn(ENV_DISABLE, found)
        self.assertIn("--no-sandbox", found)
        self.assertIn("--single-process", found)
        self.assertIn("--enable-features=NetworkServiceInProcess", found)


if __name__ == "__main__":
    unittest.main()

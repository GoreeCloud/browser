"""Build-source checks for development-only, generated Gecko GTK branding."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
GEN = ROOT / "goreecloud/tools/generate_linux_branding.py"
BRAND = ROOT / "browser/branding/goreecloud"
CONF = ROOT / "goreecloud/configs/mozconfig-linux-dev"
spec = importlib.util.spec_from_file_location("gc_browser_brand", GEN)
brand = importlib.util.module_from_spec(spec)
spec.loader.exec_module(brand)


class BrandingTests(unittest.TestCase):
    def test_reproducible_assets(self):
        expected = brand.generated()
        self.assertGreaterEqual(len(expected), 30)
        for relative, data in expected.items():
            self.assertEqual((BRAND / relative).read_bytes(), data, relative)

    def test_isolated_build_selection(self):
        config = CONF.read_text(encoding="utf-8")
        self.assertIn("ac_add_options --with-branding=browser/branding/goreecloud", config)
        self.assertIn("ac_add_options --disable-updater", config)
        self.assertNotIn("--enable-official-branding", config)
        self.assertEqual((ROOT / "browser/confvars.sh").read_text().count("MOZ_BRANDING_DIRECTORY=browser/branding/unofficial"), 1)

    def test_no_upstream_product_labels_in_visible_brand_resources(self):
        resources = ["locales/en-US/brand.ftl", "locales/en-US/brand.properties",
                     "content/about-logo.svg", "content/about-wordmark.svg"]
        for rel in resources:
            text = (BRAND / rel).read_text(encoding="utf-8").lower()
            self.assertIn("goreecloud", text)
            for upstream in ("nightly", "mozilla", "firefox"):
                self.assertNotIn(upstream, text, rel)

    def test_png_signatures(self):
        for size in (16, 32, 48, 64, 128):
            data = (BRAND / ("default" + str(size) + ".png")).read_bytes()
            self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")

    def test_missing_generated_resource_is_detected(self):
        with tempfile.TemporaryDirectory() as temp:
            subprocess.run([sys.executable, str(GEN), "--output", temp], check=True, capture_output=True)
            good = subprocess.run([sys.executable, str(GEN), "--output", temp, "--check"], capture_output=True)
            self.assertEqual(good.returncode, 0, good.stderr.decode())
            (Path(temp) / "default16.png").write_bytes(b"incorrect")
            bad = subprocess.run([sys.executable, str(GEN), "--output", temp, "--check"], capture_output=True)
            self.assertNotEqual(bad.returncode, 0)

    def test_dev_privacy_defaults_are_explicit(self):
        content = (BRAND / "pref/firefox-branding.js").read_text(encoding="utf-8")
        names = (
            "app.normandy.enabled",
            "app.shield.optoutstudies.enabled",
            "browser.discovery.enabled",
            "browser.newtabpage.activity-stream.showSponsored",
            "browser.newtabpage.activity-stream.showSponsoredTopSites",
            "browser.newtabpage.activity-stream.telemetry.privatePing.enabled",
        )
        for pref_name in names:
            statement = 'pref("' + pref_name + '", false);'
            self.assertEqual(content.count(statement), 1, pref_name)
        for critical in ("browser.safebrowsing.", "xpinstall.signatures.required",
                         "security.enterprise_roots.enabled"):
            self.assertNotIn(critical, content)

    def test_localization_manifest_substitution(self):
        jar = (BRAND / "locales/jar.mn").read_text(encoding="utf-8")
        self.assertIn("#filter substitution", jar)
        self.assertIn("[localization] @AB_CD@.jar:", jar)
        self.assertLess(jar.index("#filter substitution"), jar.index("[localization]"))
        self.assertIn("locale/branding/brand.properties", jar)

    def test_artifact_mode_remains_development_only(self):
        path = ROOT / "goreecloud/configs/mozconfig-linux-artifact-dev"
        config = path.read_text(encoding="utf-8")
        self.assertIn("--enable-artifact-builds", config)
        self.assertIn("--with-branding=browser/branding/goreecloud", config)
        self.assertIn("--disable-updater", config)
        self.assertNotIn("--enable-official-branding", config)

    def test_gtk_resources_declared(self):
        jar = (BRAND / "content/jar.mn").read_text(encoding="utf-8")
        self.assertIn("content/branding/icon128.png", jar)
        self.assertIn("content/branding/about-wordmark.svg", jar)
        self.assertTrue((BRAND / "moz.build").exists())
        self.assertTrue((BRAND / "pref/firefox-branding.js").exists())


if __name__ == "__main__":
    unittest.main()

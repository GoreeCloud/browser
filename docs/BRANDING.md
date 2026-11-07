# GoreeCloud Browser — founder-approved Prism Tabs v5 identity (Linux)

The founder approved **Prism Tabs v5** as the shared visual identity for both the native Linux-first `GoreeCloud/browser` and Android `GoreeCloud/browser-mobile`. The canonical color source is [brand-assets/products/browser-mobile/app-icon.svg](https://github.com/GoreeCloud/brand-assets/blob/1b12ecbb043c89b4f461e68ce2cd83b5fd70eb3f/products/browser-mobile/app-icon.svg), promoted by [PR #10](https://github.com/GoreeCloud/brand-assets/pull/10). The central [desktop light and dark Browser wordmarks](https://github.com/GoreeCloud/brand-assets/tree/main/products/browser) use the same icon geometry without the “MOBILE” subtitle.

## Native Linux resources

The desktop package contains pinned offline derivatives of that source: `goreecloud_browser/assets/prism-tabs-v5.svg`, 48px and 256px PNGs (from the central `sharp@0.34.5` raster manifest). `branding/asset-index.json` preserves the canonical SHA-256 hashes and Git blob identity `c82371aa6fadd45eb26610bf4907e64122d15b12`. `tests/test_branding.py` verifies exact source provenance, PNG signature, dimensions and hashes.

- `QApplication.setWindowIcon()` and `BrowserWindow.setWindowIcon()` now use a composed `QIcon` from bundled 48/256px exports, without network access or runtime file generation.
- The native Home and New Tab pages show the same Prism Tabs icon next to their page heading. The browser engine/webpage content and privacy defaults are unchanged.
- The Linux per-user installer copies the pinned 256px PNG into its freshly created private runtime folder and sets an **absolute, validated Icon path** in the freedesktop `.desktop` entry. It no longer relies on the generic `web-browser` icon. The existing uninstaller removes the managed runtime (including that icon) but preserves browsing data, preferences and bookmarks.
- `pyproject.toml` includes PNG and SVG package resources inside installed wheels. No default-browser registration, system-wide install, unsigned release distribution, or Windows/macOS packaging is performed by this branding integration.

**Testing:** Python core/unit tests (with Qt smoke intentionally opt-in), source compilation, wheel inclusion, and native Xvfb Qt WebEngine smoke should all be verified independently. The browser and brand-assets repositories each have their own CI; a central icon source match is not evidence of full Linux launcher-manager or accessibility acceptance.

**Limits:** The founder approved the full-color v5 identity. User-visible desktop launcher behavior across GNOME/KDE/Zorin, icon cache refresh, HiDPI, window manager variants, automated screen-reader validation, Windows and macOS all remain separate acceptance gates. The product is still a Linux-first development preview.

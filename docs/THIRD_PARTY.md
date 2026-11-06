# Third-party provenance and distribution notes

Original GoreeCloud Browser Python sources are Apache-2.0 (LICENSE and NOTICE).
This license does not relicense Qt or Chromium content.

- **PySide6 / Qt for Python**: Qt Project, pinned at 6.11.3 in requirements.txt.
  The wheels declare LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only.
  Dynamic LGPL-compatible use is intended; distribution compliance still
  requires review for the actual platform and bundle.
- **Qt WebEngine**: includes Chromium-derived components with multiple
  third-party licenses. Preserve actual shipped Qt and Chromium license
  notices and relevant source/linking/replacement obligations.
- **Python**: standard library and any bundling require their applicable
  upstream copyright and license notices.

No third-party icon pack, font files, browser extensions, filter subscriptions
or upstream browser source have been copied into this repo.

Primary references:
- https://doc.qt.io/qtforpython-6/licenses.html
- https://doc.qt.io/qt-6/licensing.html
- https://www.chromium.org/chromium-os/licenses/

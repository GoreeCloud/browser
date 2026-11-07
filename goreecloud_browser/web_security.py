"""Conservative Qt WebEngine content-security defaults for every Webspace.

Apply to *each* QWebEngineProfile before constructing pages. Qt defaults
vary by engine version; GoreeCloud pins required browser-security behavior
rather than relying on ambient defaults. This is a Wardveil security policy.
It does not certify all mixed-content/browser security behavior.
"""
from __future__ import annotations

from PySide6.QtWebEngineCore import QWebEngineProfile, QWebEngineSettings


# Explicitly disable capabilities which could reduce origin separation,
# open local files, access the system clipboard, or emit hyperlink pings.
# These settings are not a complete CSP, permission policy or sandbox.
DISABLED_WEB_ATTRIBUTES = (
    "AllowRunningInsecureContent",
    "AllowGeolocationOnInsecureOrigins",
    "LocalContentCanAccessRemoteUrls",
    "LocalContentCanAccessFileUrls",
    "JavascriptCanOpenWindows",
    "JavascriptCanAccessClipboard",
    "JavascriptCanPaste",
    "HyperlinkAuditingEnabled",
)


def configure_content_security(profile: QWebEngineProfile) -> None:
    """Apply first-party security settings to a WebEngine profile."""
    settings = profile.settings()
    settings.setUnknownUrlSchemePolicy(
        QWebEngineSettings.UnknownUrlSchemePolicy.DisallowUnknownUrlSchemes
    )
    for name in DISABLED_WEB_ATTRIBUTES:
        settings.setAttribute(
            getattr(QWebEngineSettings.WebAttribute, name),
            False,
        )

"""GoreeCloud Browser: native Qt Widgets chrome with Qt WebEngine pages.

No remote service is contacted by Home or New Tab. Network navigation requires
a user action. This is a development preview, not a hardened full browser.
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path

from PySide6.QtCore import Qt, QSettings, QStandardPaths, QUrl, QTimer, QSize, Signal, QProcess
from PySide6.QtGui import QAction, QKeySequence, QShortcut
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QDialog, QFileDialog, QFrame, QHBoxLayout,
    QLabel, QLineEdit, QMainWindow, QMenu, QMessageBox, QProgressBar,
    QPushButton, QScrollArea, QStackedWidget, QTabWidget, QToolButton,
    QVBoxLayout, QWidget,
)
from PySide6.QtWebEngineCore import (
    QWebEngineDownloadRequest, QWebEnginePage, QWebEngineProfile,
    QWebEngineUrlRequestInfo, QWebEngineUrlRequestInterceptor,
)
from PySide6.QtWebEngineWidgets import QWebEngineView
from shiboken6 import delete as destroy_qobject

from goreecloud_browser.core import (
    destination_for, is_known_tracker, valid_web_url,
)
from goreecloud_browser.branding import browser_icon
from goreecloud_browser.site_data import (
    ProfileLock, apply_pending_site_reset, schedule_site_reset,
)
from goreecloud_browser.downloads import (
    new_staging, publish_complete_file, suggested_filename, validate_target,
)
from goreecloud_browser.malware_scan import (
    SCAN_TIMEOUT_MS, MAX_PARALLEL_SCANS, ScanVerdict, find_clamscan, staged_fingerprint,
    scanner_arguments, classify_clamscan_result,
)
from goreecloud_browser.chrome_icons import icon_for
from goreecloud_browser.filters import (
    SOURCES, is_blocked, needs_update, read_cached,
)
from goreecloud_browser.filter_updater import FilterUpdater
from goreecloud_browser.native_panels import (
    SettingsPage, TabManagerDialog, DownloadManagerDialog,
    WebspacesDialog, CookieManagerDialog,
)
from goreecloud_browser.site_exceptions_panel import SiteExceptionsDialog
from goreecloud_browser.webspaces import (
    DEFAULT_ID, decode_spaces, encode_spaces, create_space, storage_paths,
)
from goreecloud_browser.proxy import ProxyConfig, validate_proxy
from goreecloud_browser.proxy_runtime import apply_global_proxy
from goreecloud_browser.permissions import PermissionCenterDialog, display_origin
from goreecloud_browser.site_exceptions import (
    decode_exceptions, encode_exceptions, add_exception, remove_exception,
    is_exempt_first_party,
)
from goreecloud_browser.sandbox import (
    WebEngineSecurityError, require_safe_webengine_settings,
)
from goreecloud_browser.web_security import configure_content_security

# Values copied from the canonical GC UI token implementation
# GoreeCloud/gc-ui: styles/tokens.css (verified 2026-11-06).
# These are a native Qt adapter, NOT a second authoritative design system.
THEMES = {
    "light": {
        "canvas": "#ecf0f8", "surface": "#f9fafd", "raised": "#ffffff",
        "tonal": "#e5ebf6", "text": "#20243a", "soft": "#666c80",
        "outline": "#e5e7f0", "brand": "#087c8d", "onbrand": "#ffffff",
        "container": "#d1f4f5", "oncontainer": "#07565d",
    },
    "dark": {
        "canvas": "#10131e", "surface": "#1d2231", "raised": "#252b3d",
        "tonal": "#292f40", "text": "#f2f2fa", "soft": "#adb2c5",
        "outline": "#34394c", "brand": "#89e1e9", "onbrand": "#003b43",
        "container": "#1c4d57", "oncontainer": "#c8fcff",
    },
}


def stylesheet(mode: str) -> str:
    c = THEMES[mode]
    return f"""
    QWidget {{ background: {c['canvas']}; color: {c['text']}; font-family: sans-serif; font-size: 14px; }}
    QLabel {{ background: transparent; }}
    QWidget#Shell, QWidget#Navigation {{ background: {c['surface']}; }}
    QLabel#Eyebrow {{ color: {c['brand']}; font-size: 12px; font-weight: 700; }}
    QLabel#HeroTitle {{ font-size: 31px; font-weight: 750; }}
    QLabel#HeroSubtitle {{ color: {c['soft']}; font-size: 15px; }}
    QLabel#SectionTitle {{ font-size: 18px; font-weight: 700; }}
    QLabel#FinePrint {{ color: {c['soft']}; font-size: 12px; }}
    QFrame#Hero {{ background: {c['raised']}; border: 1px solid {c['outline']}; border-radius: 26px; }}
    QFrame#Tile {{ background: {c['surface']}; border: 1px solid {c['outline']}; border-radius: 18px; }}
    QLineEdit {{ background: {c['raised']}; border: 1px solid {c['outline']}; border-radius: 14px;
                 padding: 11px 15px; min-height: 24px; selection-background-color: {c['container']};
                 selection-color: {c['oncontainer']}; }}
    QLineEdit:focus {{ border: 2px solid {c['brand']}; padding: 10px 14px; }}
    QPushButton, QToolButton {{ background: {c['surface']}; border: 1px solid {c['outline']};
                                border-radius: 12px; padding: 9px 12px; min-height: 25px; }}
    QPushButton:hover, QToolButton:hover {{ background: {c['tonal']}; }}
    QPushButton:focus, QToolButton:focus {{ border: 2px solid {c['brand']}; }}
    QPushButton#Primary {{ background: {c['brand']}; color: {c['onbrand']};
                           border: 1px solid {c['brand']}; font-weight: 650; }}
    QPushButton#Primary:hover {{ background: {c['container']}; color: {c['oncontainer']}; }}
    QPushButton#TileAction {{ text-align: left; padding: 15px; background: {c['raised']};
                              border: 1px solid {c['outline']}; border-radius: 18px; }}
    QPushButton#TileAction:hover {{ border: 2px solid {c['brand']}; padding: 14px; }}
    QTabWidget::pane {{ background: {c['surface']}; border: none; }}
    QTabBar {{ background: {c['surface']}; }}
    QTabBar::tab {{ background: {c['tonal']}; border: none; border-top-left-radius: 12px;
                    border-top-right-radius: 12px; padding: 11px 18px; margin: 3px 3px 0 0;
                    min-width: 95px; max-width: 225px; }}
    QTabBar::tab:selected {{ background: {c['raised']}; color: {c['brand']}; font-weight: 700; }}
    QTabBar::tab:hover {{ background: {c['container']}; color: {c['oncontainer']}; }}
    QMenu {{ background: {c['raised']}; border: 1px solid {c['outline']}; padding: 6px; }}
    QMenu::item {{ padding: 9px 20px; }}
    QMenu::item:selected {{ background: {c['container']}; color: {c['oncontainer']}; }}
    QDialog {{ background: {c['canvas']}; }}
    QGroupBox#SettingsSection {{ background: {c['raised']}; border: 1px solid {c['outline']};
                                 border-radius: 18px; margin-top: 14px; font-size: 16px;
                                 font-weight: 700; padding-top: 8px; }}
    QGroupBox#SettingsSection::title {{ subcontrol-origin: margin; left: 17px;
                                       padding: 2px 8px; }}
    QComboBox, QPlainTextEdit, QTableWidget, QListWidget {{ background: {c['raised']};
                   color: {c['text']}; border: 1px solid {c['outline']}; border-radius: 10px;
                   padding: 7px; }}
    QFrame#Omnibox {{ background: {c['raised']}; border: 1px solid {c['outline']};
                      border-radius: 14px; }}
    QLineEdit#OmniboxEdit {{ border: none; background: transparent; padding: 6px;
                             min-height: 28px; }}
    QLineEdit#OmniboxEdit:focus {{ border: none; padding: 6px; }}
    QToolButton#OmniboxGlyph {{ border: none; background: transparent; padding: 4px; }}
    QToolButton#OmniboxGlyph:hover {{ background: {c['tonal']}; }}
    QProgressBar {{ border: none; background: {c['tonal']}; max-height: 3px; }}
    QProgressBar::chunk {{ background: {c['brand']}; }}
    QStatusBar {{ background: {c['surface']}; color: {c['soft']}; }}
    QScrollArea {{ background: {c['canvas']}; border: none; }}
    QScrollBar:vertical {{ background: {c['surface']}; width: 10px; margin: 0; }}
    QScrollBar::handle:vertical {{ background: {c['soft']}; border-radius: 5px; min-height: 36px; }}
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{ height: 0; }}
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{ background: transparent; }}
    """


def button(text: str, tooltip: str, callback, primary: bool = False) -> QPushButton:
    item = QPushButton(text)
    item.setToolTip(tooltip)
    item.setAccessibleName(tooltip)
    item.setMinimumHeight(44)
    if primary:
        item.setObjectName("Primary")
    item.clicked.connect(callback)
    return item


class PrivacyInterceptor(QWebEngineUrlRequestInterceptor):
    """Fast thread-safe* pure-Python rules; no GUI or profile calls here.

    *Boolean policy assignment uses the GIL; these assignments are independent.
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.block_trackers = True
        self.domain_rules = ()  # immutable snapshot swapped from the GUI thread
        self.exempted_first_party_hosts = frozenset()

    def interceptRequest(self, info: QWebEngineUrlRequestInfo) -> None:
        url = info.requestUrl()
        if url.scheme() not in ("http", "https"):
            return
        main_frame = (info.resourceType() ==
                      QWebEngineUrlRequestInfo.ResourceType.ResourceTypeMainFrame)
        if not main_frame:
            host = url.host().lower().rstrip(".")
            first_party_url = info.firstPartyUrl()
            first_party = first_party_url.host().lower().rstrip(".")
            # Explicit, saved host-only user exceptions are applied to the
            # first-party site, not to every resource from a blocked domain.
            if (first_party_url.scheme() in ("https", "http")
                    and is_exempt_first_party(
                        first_party, self.exempted_first_party_hosts)):
                return
            if host != first_party and (
                (self.block_trackers and is_known_tracker(host, first_party))
                or is_blocked(host, self.domain_rules)
            ):
                info.block(True)
                return


class BrowserPage(QWebEnginePage):
    def __init__(self, profile: QWebEngineProfile, owner, parent=None, space_id=DEFAULT_ID):
        super().__init__(profile, parent)
        self.owner = owner
        self.space_id = space_id
        # Site capabilities always require consent; no silent grants.
        self.permissionRequested.connect(self.ask_permission)
        # Never offer a certificate bypass, even for locally generated or
        # otherwise Qt-overridable errors. Chromium's trust checks are the
        # authority; this callback simply enforces their rejection.
        self.certificateError.connect(self.reject_bad_certificate)

    def reject_bad_certificate(self, certificate_error) -> None:
        certificate_error.rejectCertificate()
        if not getattr(self.owner, "_closing", False):
            self.owner.statusBar().showMessage(
                "Blocked an HTTPS connection with an invalid certificate.", 8000)

    def acceptNavigationRequest(self, url: QUrl, nav_type, is_main_frame: bool) -> bool:
        if is_main_frame and url.scheme() not in ("http", "https", "about"):
            self.owner.statusBar().showMessage("Blocked an unsupported navigation scheme.", 5000)
            return False
        # Clicked links, scripts and redirects do not pass through the address
        # bar's typed-URL validation. Reject unsafe HTTP(S) main-frame URLs
        # *before* handing them to Qt WebEngine, without rewriting query strings
        # or replacing navigation methods (POST and signed URLs must survive).
        if (is_main_frame and url.scheme() in ("http", "https")
                and not valid_web_url(
                    url.toString(QUrl.ComponentFormattingOption.FullyEncoded)
                )):
            self.owner.statusBar().showMessage("Blocked an unsafe website address.", 5000)
            return False
        return super().acceptNavigationRequest(url, nav_type, is_main_frame)

    def createWindow(self, _type):
        # Open target=_blank in a contained browser tab, not an external window.
        return self.owner.new_tab(space_id=self.space_id).view.page()

    def ask_permission(self, permission) -> None:
        if not permission.isValid():
            return
        # Show the *requesting origin*, not just a host or current page title.
        # Embedded third-party frames may request their own capabilities.
        origin = permission.origin().toString(QUrl.ComponentFormattingOption.FullyEncoded)
        top_origin = display_origin(self.url())
        answer = QMessageBox.question(
            self.owner, "Website permission",
            f"Requesting origin: {origin}\n"
            f"Top-level website: {top_origin}\n"
            f"Webspace: {self.owner.webspace_name(self.space_id)}\n\n"
            f"Requested capability: {permission.permissionType().name}\n\n"
            "Allow this website capability for the current session? "
            "Review or reset stored decisions using Permissions in the omnibox.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer == QMessageBox.StandardButton.Yes:
            permission.grant()
        else:
            permission.deny()


class NativeStart(QWidget):
    """Qt-native Home and New Tab experiences; no remote HTML or JavaScript."""
    def __init__(self, browser, tab, kind: str):
        super().__init__()
        # Preserve the form's minimum usable content height; the parent
        # QScrollArea provides access at the supported 700x480 window size.
        self.setMinimumHeight(565)
        outer = QVBoxLayout(self)
        outer.setContentsMargins(28, 26, 28, 30)
        outer.setSpacing(20)
        hero = QFrame()
        hero.setObjectName("Hero")
        hero.setMaximumWidth(880)
        hero.setMinimumHeight(310)
        box = QVBoxLayout(hero)
        box.setContentsMargins(38, 36, 38, 38)
        box.setSpacing(15)

        if kind == "home":
            eyebrow, heading = "GOREECLOUD  /  HOME", "Your space. Your way."
            subtitle = "A quiet starting point for everything you browse."
        else:
            eyebrow, heading = "NEW TAB  /  EXPLORE", "Find your next idea."
            subtitle = "Search privately with Startpage while GoreeCloud Search matures."

        # Product identity is a bundled, first-party Qt image, not remote page content.
        brand_row = QHBoxLayout()
        brand = QLabel()
        brand.setObjectName("BrandMark")
        brand.setFixedSize(54, 54)
        brand.setPixmap(browser_icon().pixmap(52, 52))
        brand.setAccessibleName("GoreeCloud Browser Prism Tabs icon")
        brand_row.addWidget(brand)
        label = QLabel(eyebrow)
        label.setObjectName("Eyebrow")
        brand_row.addWidget(label)
        brand_row.addStretch(1)
        box.addLayout(brand_row)
        title = QLabel(heading)
        title.setObjectName("HeroTitle")
        title.setWordWrap(True)
        box.addWidget(title)
        detail = QLabel(subtitle)
        detail.setObjectName("HeroSubtitle")
        detail.setWordWrap(True)
        box.addWidget(detail)

        self.search = QLineEdit()
        self.search.setPlaceholderText("Search with Startpage or enter a website")
        self.search.setAccessibleName("Search or enter address")
        self.search.returnPressed.connect(
            lambda: browser.navigate(self.search.text(), tab))
        box.addWidget(self.search)
        row = QHBoxLayout()
        row.setSpacing(10)
        go = button("Explore", "Search or visit the entered address",
                    lambda: browser.navigate(self.search.text(), tab), primary=True)
        row.addWidget(go)
        if kind == "home":
            row.addWidget(button("New tab", "Open a new tab", browser.new_tab))
        else:
            row.addWidget(button("Go home", "Open the native Home page",
                                  lambda: browser.show_home(tab)))
        row.addStretch()
        box.addLayout(row)

        outer.addStretch(1)
        hero_row = QHBoxLayout()
        hero_row.addStretch(1)
        hero_row.addWidget(hero, 4)
        hero_row.addStretch(1)
        outer.addLayout(hero_row)

        section = QLabel("YOUR BROWSER CONTROLS")
        section.setObjectName("SectionTitle")
        outer.addWidget(section)
        quick = QHBoxLayout()
        quick.setSpacing(12)
        actions = (
            [("★  Bookmarks", "Access pages you've saved", browser.open_bookmark_menu),
             ("◈  Privacy", "Manage cookies and tracking controls", browser.show_privacy)]
            if kind == "home" else
            [("⌂  Home", "Open the Home page", lambda: browser.show_home(tab)),
             ("★  Bookmarks", "Open saved bookmarks", browser.open_bookmark_menu)]
        )
        for name, tip, action in actions:
            tile = button(name, tip, action)
            tile.setObjectName("TileAction")
            tile.setMinimumHeight(68)
            quick.addWidget(tile)
        outer.addLayout(quick)
        note = QLabel("Native pages load no external website  •  Enabled filter lists may refresh from their HTTPS sources  •  Session-only cookies by default")
        note.setObjectName("FinePrint")
        note.setWordWrap(True)
        outer.addWidget(note)
        outer.addStretch(2)


class BrowserTab(QWidget):
    def __init__(self, browser, profile: QWebEngineProfile, space_id=DEFAULT_ID):
        super().__init__()
        self.browser = browser
        self.profile = profile
        self.space_id = space_id
        self.settings_page = None
        self.stack = QStackedWidget()
        self.view = QWebEngineView()
        self.view.setPage(BrowserPage(profile, browser, self.view, space_id=space_id))
        self.home = NativeStart(browser, self, "home")
        self.new = NativeStart(browser, self, "new-tab")
        self.new_scroll = self._native_scroll(self.new)
        self.home_scroll = self._native_scroll(self.home)
        self.stack.addWidget(self.new_scroll)
        self.stack.addWidget(self.home_scroll)
        self.stack.addWidget(self.view)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.stack)

        self.view.titleChanged.connect(lambda _title: browser.update_tab(self))
        self.view.urlChanged.connect(lambda _url: browser.update_tab(self))
        self.view.loadStarted.connect(lambda: self.stack.setCurrentWidget(self.view))
        self.view.loadProgress.connect(browser.load_progress)
        self.view.loadFinished.connect(
            lambda ok: browser.statusBar().showMessage(
                "Page loaded" if ok else "Page could not be loaded", 4000))
        self.show_new()

    @staticmethod
    def _native_scroll(content: QWidget) -> QScrollArea:
        area = QScrollArea()
        area.setWidgetResizable(True)
        area.setFrameShape(QFrame.Shape.NoFrame)
        area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        area.setWidget(content)
        area.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        return area

    def show_new(self):
        self.new_scroll.verticalScrollBar().setValue(0)
        self.stack.setCurrentWidget(self.new_scroll)
        self.browser.update_tab(self)

    def show_home(self):
        self.home_scroll.verticalScrollBar().setValue(0)
        self.stack.setCurrentWidget(self.home_scroll)
        self.browser.update_tab(self)

    def show_settings(self):
        if self.settings_page is None:
            self.settings_page = SettingsPage(self.browser)
            self.stack.addWidget(self.settings_page)
        self.stack.setCurrentWidget(self.settings_page)
        self.browser.update_tab(self)

    def browse(self, target: str):
        self.stack.setCurrentWidget(self.view)
        self.view.setUrl(QUrl(target))

    def current_address(self) -> str:
        current = self.stack.currentWidget()
        if current is self.new_scroll:
            return "goree://new-tab"
        if current is self.home_scroll:
            return "goree://home"
        if self.settings_page is not None and current is self.settings_page:
            return "goree://settings"
        return self.view.url().toString(QUrl.ComponentFormattingOption.FullyEncoded)


class PrivacyDialog(QDialog):
    def __init__(self, browser):
        super().__init__(browser)
        self.setWindowTitle("Privacy • GoreeCloud Browser")
        self.setMinimumWidth(480)
        box = QVBoxLayout(self)
        box.setSpacing(14)
        title = QLabel("Privacy controls")
        title.setObjectName("HeroTitle")
        box.addWidget(title)
        expl = QLabel("GoreeShield governs privacy choices. Wardveil owns cybersecurity protections.")
        expl.setWordWrap(True)
        expl.setObjectName("HeroSubtitle")
        box.addWidget(expl)

        tracking = QCheckBox("Block known third-party tracker domains")
        tracking.setChecked(browser.interceptor.block_trackers)
        tracking.toggled.connect(lambda value: browser.set_preference("block_trackers", value))
        box.addWidget(tracking)
        cleaning = QCheckBox("Clean known tracking parameters from entered URLs")
        cleaning.setChecked(browser.clean_links)
        cleaning.toggled.connect(lambda value: browser.set_preference("clean_links", value))
        box.addWidget(cleaning)
        cookies = QCheckBox("Allow cookies to persist after closing the browser")
        cookies.setChecked(browser.persistent_cookies)
        cookies.toggled.connect(lambda value: browser.set_preference("persistent_cookies", value))
        box.addWidget(cookies)

        note = QLabel(
            "URL cleaning applies to addresses entered or pasted into this browser, "
            "not automatically to clicked links or site redirects. "
            "This is not a complete ad blocker or anti-fingerprinting system. "
            "Cookie/cache clearing leaves some site storage. DNS follows your OS."
        )
        note.setWordWrap(True)
        note.setObjectName("FinePrint")
        box.addWidget(note)
        box.addWidget(button("Clear cookies and HTTP cache", "Delete browser cookies and HTTP cache",
                             browser.clear_cookies))
        box.addWidget(button("Erase all website data after restart…",
                             "Request an explicit restart-time site data reset",
                             browser.request_site_data_reset))
        done = button("Done", "Close privacy controls", self.accept, primary=True)
        box.addWidget(done)


class BrowserWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        # Run before Qt WebEngine profile creation, cache access and any
        # browsing state changes. It also covers direct Python constructors.
        require_safe_webengine_settings(os.environ, sys.argv[1:])
        self.setWindowTitle("GoreeCloud Browser")
        self.setWindowIcon(browser_icon())
        self.resize(1250, 790)
        self.setMinimumSize(700, 480)
        self.preferences = QSettings("GoreeCloud", "GoreeCloudBrowser")
        self.mode = self.preferences.value("appearance", "light")
        if self.mode not in THEMES:
            self.mode = "light"
        self.interceptor = PrivacyInterceptor(self)
        self.site_exceptions = decode_exceptions(
            self.preferences.value("privacy_site_exceptions", "[]"))
        self.interceptor.exempted_first_party_hosts = frozenset(self.site_exceptions)
        self.interceptor.block_trackers = self.preferences.value("block_trackers", True, type=bool)
        self.clean_links = self.preferences.value("clean_links", True, type=bool)
        self.persistent_cookies = self.preferences.value("persistent_cookies", False, type=bool)
        try:
            self.proxy_config = validate_proxy(
                self.preferences.value("proxy_mode", "system"),
                self.preferences.value("proxy_host", ""),
                self.preferences.value("proxy_port", 0),
            )
        except ValueError:
            self.proxy_config = ProxyConfig()  # invalid data cannot route traffic
        apply_global_proxy(self.proxy_config)
        self._spaces = decode_spaces(self.preferences.value("webspaces", "[]"))
        self.space_profiles = {}
        self._list_enabled = {
            name: self.preferences.value("filter_" + name, True, type=bool)
            for name in SOURCES
        }
        self._filter_domains = {}
        self._filter_errors = {}
        self.filter_updater = FilterUpdater()
        self.filter_updater.signals.completed.connect(self.filters_updated)

        storage = Path(QStandardPaths.writableLocation(QStandardPaths.StandardLocation.AppDataLocation))
        self.site_storage = storage
        self.filter_storage = storage / "filter-lists"
        self._filter_domains = {
            name: read_cached(self.filter_storage, name) if enabled else frozenset()
            for name, enabled in self._list_enabled.items()
        }
        self.refresh_filter_rules()
        try:
            self.profile_lock = ProfileLock(storage)
            apply_pending_site_reset(storage)
        except (OSError, ValueError, BlockingIOError) as exc:
            if hasattr(self, "profile_lock"):
                self.profile_lock.close()
            raise RuntimeError("Browser profile is already in use or its data reset failed. "
                               "Close other GoreeCloud Browser processes and check site storage.") from exc
        self.profile = QWebEngineProfile("GoreeCloudBrowser", self)
        self.profile.setPersistentStoragePath(str(storage / "web-profile"))
        self.profile.setCachePath(str(storage / "cache"))
        self.profile.setHttpCacheType(QWebEngineProfile.HttpCacheType.MemoryHttpCache)
        self.profile.setPersistentCookiesPolicy(
            QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies
            if self.persistent_cookies
            else QWebEngineProfile.PersistentCookiesPolicy.NoPersistentCookies)
        # Prevent granted website permissions from surviving an app restart.
        self.profile.setPersistentPermissionsPolicy(
            QWebEngineProfile.PersistentPermissionsPolicy.StoreInMemory)
        configure_content_security(self.profile)
        self.profile.setUrlRequestInterceptor(self.interceptor)
        self.space_profiles[DEFAULT_ID] = self.profile
        self._pending_downloads = {}
        # Qt QProcess scanners run asynchronously; never block the UI with
        # a large archive and never publish before a clean scanner verdict.
        self._scan_pending = {}
        self._next_scan_id = 0  # Do not reuse id(download) after Qt releases it.
        self._scan_timed_out = set()
        self._download_history = []
        # Qt may complete a small download without emitting a callback after
        # accept(); a bounded event-loop check catches that completion.
        self._download_poll_timer = QTimer(self)
        self._download_poll_timer.setInterval(150)
        self._download_poll_timer.timeout.connect(self.poll_downloads)
        self.profile.downloadRequested.connect(self.handle_download)

        self.setObjectName("Window")
        base = QWidget()
        base.setObjectName("Shell")
        self.setCentralWidget(base)
        layout = QVBoxLayout(base)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)
        nav = QWidget()
        nav.setObjectName("Navigation")
        navrow = QHBoxLayout(nav)
        navrow.setContentsMargins(9, 10, 9, 9)
        navrow.setSpacing(5)
        self._chrome_buttons = {}

        self.back = self.chrome_button("back", "Back (Alt+Left)", self.go_back)
        self.forward = self.chrome_button("forward", "Forward (Alt+Right)", self.go_forward)
        self.reload = self.chrome_button("refresh", "Refresh (Ctrl+R)", self.reload_page)
        self.home_button = self.chrome_button("home", "Home (Alt+Home)", self.show_home)
        for widget in (self.back, self.forward, self.reload, self.home_button):
            navrow.addWidget(widget)

        self.omnibox = QFrame()
        self.omnibox.setObjectName("Omnibox")
        omni = QHBoxLayout(self.omnibox)
        omni.setContentsMargins(4, 0, 4, 0)
        omni.setSpacing(1)
        self.verify_button = self.chrome_button("verify", "Connection verification", self.show_verification,
                                                size=30, in_omnibox=True)
        self.permissions_button = self.chrome_button("permissions", "Website permissions", self.show_permissions,
                                                     size=30, in_omnibox=True)
        omni.addWidget(self.verify_button)
        omni.addWidget(self.permissions_button)
        self.address = QLineEdit()
        self.address.setObjectName("OmniboxEdit")
        self.address.setPlaceholderText("Search with Startpage or enter a website")
        self.address.setAccessibleName("Unified address and search bar")
        self.address.returnPressed.connect(lambda: self.navigate(self.address.text()))
        omni.addWidget(self.address, stretch=1)
        self.webspaces_button = self.chrome_button("webspaces", "Webspaces • website containers",
                                                   self.show_webspaces, size=30, in_omnibox=True)
        omni.addWidget(self.webspaces_button)
        navrow.addWidget(self.omnibox, stretch=1)

        self.bookmark_menu = QMenu(self)
        self.bookmark_menu.aboutToShow.connect(self.populate_bookmarks)
        self.bookmark_button = self.chrome_button("bookmark", "Bookmarks", None)
        self.bookmark_button.setMenu(self.bookmark_menu)
        self.bookmark_button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        navrow.addWidget(self.bookmark_button)
        self.extensions_button = self.chrome_button("extensions", "Extensions", self.show_extensions)
        navrow.addWidget(self.extensions_button)
        self.settings_button = self.chrome_button("settings", "Settings", self.show_settings)
        navrow.addWidget(self.settings_button)
        self.profile_button = self.chrome_button("profile", "Profile and Webspaces", None)
        self.profile_menu = QMenu(self)
        self.profile_menu.aboutToShow.connect(self.populate_profile_menu)
        self.profile_button.setMenu(self.profile_menu)
        self.profile_button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        navrow.addWidget(self.profile_button)
        self.toolbar_order = ("back", "forward", "refresh", "home", "omnibox",
                              "bookmark", "extensions", "settings", "profile")
        layout.addWidget(nav)

        self.progress = QProgressBar()
        self.progress.setTextVisible(False)
        self.progress.setRange(0, 100)
        self.progress.hide()
        layout.addWidget(self.progress)
        self.tabs = QTabWidget()
        self.tabs.setMovable(True)
        self.tabs.setTabsClosable(True)
        self.tabs.setDocumentMode(True)
        self.tabs.tabCloseRequested.connect(self.close_tab)
        self.tabs.currentChanged.connect(self.update_navigation)
        plus = self.chrome_button("plus", "New tab (Ctrl+T)", self.new_tab)
        plus.setFixedSize(42, 42)
        self.tabs.setCornerWidget(plus, Qt.Corner.TopRightCorner)
        layout.addWidget(self.tabs, stretch=1)
        self.statusBar().showMessage("Ready • Startpage search")
        self.install_shortcuts()
        self.apply_theme()
        self.new_tab()
        # Public-source list checks use only fixed HTTPS provider URLs. No page
        # addresses, cookies, history or profile secrets are part of requests.
        QTimer.singleShot(2500, self.auto_refresh_blocklists)

    def webspaces(self):
        return [(DEFAULT_ID, "Default")] + [
            (item["id"], item["name"]) for item in self._spaces
        ]

    def webspace_name(self, ident: str) -> str:
        return dict(self.webspaces()).get(ident, "Unknown")

    def add_webspace(self, name: str) -> str:
        self._spaces = create_space(self._spaces, name)
        self.preferences.setValue("webspaces", encode_spaces(self._spaces))
        return self._spaces[-1]["id"]

    def get_profile(self, space_id: str) -> QWebEngineProfile:
        if space_id in self.space_profiles:
            return self.space_profiles[space_id]
        if space_id not in dict(self.webspaces()):
            raise ValueError("Unknown Webspace")
        web_path, cache_path = storage_paths(self.site_storage, space_id)
        # Defensive check against storage path substitution outside the user
        # browser data root before WebEngine accesses persistent state.
        for target in (self.site_storage / "spaces", web_path.parent,
                       web_path, cache_path):
            if target.is_symlink():
                raise ValueError("Symlinked Webspace storage path refused")
        profile = QWebEngineProfile("GoreeCloudWebspace-" + space_id, self)
        profile.setPersistentStoragePath(str(web_path))
        profile.setCachePath(str(cache_path))
        profile.setHttpCacheType(QWebEngineProfile.HttpCacheType.MemoryHttpCache)
        profile.setPersistentCookiesPolicy(
            QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies
            if self.persistent_cookies else QWebEngineProfile.PersistentCookiesPolicy.NoPersistentCookies)
        profile.setPersistentPermissionsPolicy(
            QWebEngineProfile.PersistentPermissionsPolicy.StoreInMemory)
        configure_content_security(profile)
        profile.setUrlRequestInterceptor(self.interceptor)
        profile.downloadRequested.connect(self.handle_download)
        self.space_profiles[space_id] = profile
        return profile

    def configure_proxy(self, mode: str, host: str, port: int):
        try:
            selected = validate_proxy(mode, host, port)
        except ValueError as exc:
            QMessageBox.warning(self, "Proxy not saved", str(exc))
            return
        self.preferences.setValue("proxy_mode", selected.mode)
        self.preferences.setValue("proxy_host", selected.host)
        self.preferences.setValue("proxy_port", selected.port)
        self.preferences.sync()
        QMessageBox.information(
            self, "Restart required",
            "Global proxy preference saved for the next browser launch. "
            "It does not change the active session. All Webspaces share the proxy. "
            "Authentication is not stored, and custom per-Webspace routing is not available. "
            "Privacy-domain list updates are paused outside System proxy mode "
            "to avoid bypassing the selected browser proxy."
        )

    def add_site_exception(self, value: str) -> str:
        proposed = add_exception(self.site_exceptions, value)
        self.preferences.setValue("privacy_site_exceptions", encode_exceptions(proposed))
        self.preferences.sync()
        self.site_exceptions = proposed
        self.interceptor.exempted_first_party_hosts = frozenset(proposed)
        return proposed[-1]

    def remove_site_exception(self, value: str):
        proposed = remove_exception(self.site_exceptions, value)
        self.preferences.setValue("privacy_site_exceptions", encode_exceptions(proposed))
        self.preferences.sync()
        self.site_exceptions = proposed
        self.interceptor.exempted_first_party_hosts = frozenset(proposed)

    def clear_site_exceptions(self):
        self.preferences.setValue("privacy_site_exceptions", "[]")
        self.preferences.sync()
        self.site_exceptions = ()
        self.interceptor.exempted_first_party_hosts = frozenset()

    def list_enabled(self, name: str) -> bool:
        return bool(self._list_enabled.get(name, False))

    def set_list_enabled(self, name: str, enabled: bool):
        if name not in SOURCES:
            raise ValueError("Unknown filter list")
        self._list_enabled[name] = bool(enabled)
        self.preferences.setValue("filter_" + name, bool(enabled))
        if enabled and not self._filter_domains.get(name):
            self.refresh_blocklists()
        self.refresh_filter_rules()

    def refresh_filter_rules(self):
        self.interceptor.domain_rules = tuple(
            self._filter_domains.get(name, frozenset())
            for name in SOURCES if self._list_enabled.get(name, False)
            and self._filter_domains.get(name)
        )

    def filter_status_text(self) -> str:
        parts = []
        for name in SOURCES:
            if not self._list_enabled.get(name, False):
                state = "disabled"
            else:
                count = len(self._filter_domains.get(name, ()))
                state = f"{count:,} loaded domains" if count else "not yet loaded"
            if self._filter_errors.get(name):
                state += " • last update failed (cached rules retained)"
            parts.append(f"{name}: {state}")
        return "  |  ".join(parts)

    def auto_refresh_blocklists(self):
        # Offline integration tests must not make incidental network requests.
        if os.environ.get("GC_BROWSER_FILTER_NETWORK") == "off":
            return
        self.refresh_blocklists(only_stale=True)

    def refresh_blocklists(self, status_label=None, only_stale=False):
        if getattr(self, "_closing", False):
            return
        if self.proxy_config.mode != "system":
            if not only_stale:
                self.statusBar().showMessage(
                    "Filter updates paused outside System proxy mode to prevent proxy bypass.", 8000)
            return
        names = tuple(
            name for name in SOURCES if self._list_enabled.get(name, False)
            and (not only_stale or needs_update(
                self.filter_storage, name, loaded_domains=self._filter_domains.get(name),
            ))
        )
        if not names:
            return
        if self.filter_updater.start(self.filter_storage, names):
            self.statusBar().showMessage("Updating enabled privacy-domain lists from source…", 5000)
            if status_label is not None:
                status_label.setText(self.filter_status_text() + " • updating…")

    def filters_updated(self, results):
        self.filter_updater.running = False
        if getattr(self, "_closing", False):
            return
        for name, result in results.items():
            self._filter_domains[name] = result["domains"]
            self._filter_errors[name] = result["error"]
        self.refresh_filter_rules()
        for i in range(self.tabs.count()):
            tab = self.tabs.widget(i)
            if tab.settings_page is not None:
                tab.settings_page.list_status.setText(self.filter_status_text())
        self.statusBar().showMessage(self.filter_status_text(), 9000)

    def chrome_button(self, glyph: str, label: str, callback, size=42, in_omnibox=False):
        widget = QToolButton(self)
        widget.setObjectName("OmniboxGlyph" if in_omnibox else "ChromeGlyph")
        widget.setProperty("glyphName", glyph)
        widget.setText("")
        widget.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonIconOnly)
        widget.setIconSize(QSize(20 if size > 32 else 18, 20 if size > 32 else 18))
        widget.setFixedSize(size, size)
        widget.setToolTip(label)
        widget.setAccessibleName(label)
        if callback:
            widget.clicked.connect(callback)
        self._chrome_buttons.setdefault(glyph, []).append(widget)
        return widget

    def current(self):
        return self.tabs.currentWidget()

    def new_tab(self, space_id=None):
        # Qt clicked(bool) can pass False into a connected optional argument.
        # It is not a Webspace identifier and must never select one.
        if space_id is None or isinstance(space_id, bool):
            current = self.current()
            space_id = current.space_id if current else DEFAULT_ID
        tab = BrowserTab(self, self.get_profile(space_id), space_id=space_id)
        index = self.tabs.addTab(tab, "New Tab")
        self.tabs.setCurrentIndex(index)
        tab.new.search.setFocus()
        return tab

    def open_home_tab(self):
        tab = self.new_tab()
        tab.show_home()
        return tab

    def show_settings(self):
        tab = self.new_tab()
        tab.show_settings()
        self.update_navigation()

    def show_verification(self):
        address = self.current().current_address()
        parsed = QUrl(address)
        if parsed.scheme() == "https":
            summary = ("HTTPS address. Qt WebEngine manages TLS validation. "
                       "This preview does not expose a verified certificate chain or detailed security audit.")
        elif parsed.scheme() == "http":
            summary = "Unencrypted HTTP address. Do not submit sensitive information."
        else:
            summary = "Native GoreeCloud Browser page; no remote website loaded."
        QMessageBox.information(self, "Website verification",
            f"Host: {parsed.host() or 'Native browser page'}\n\n{summary}")

    def show_permissions(self):
        PermissionCenterDialog(self).exec()

    def show_site_exceptions(self):
        SiteExceptionsDialog(self).exec()

    def show_webspaces(self):
        WebspacesDialog(self).exec()

    def show_tab_manager(self):
        TabManagerDialog(self).exec()

    def show_download_manager(self):
        DownloadManagerDialog(self).exec()

    def show_cookie_manager(self):
        CookieManagerDialog(self).exec()

    def show_extensions(self):
        QMessageBox.information(self, "Extensions",
            "Extension installation is not enabled in this Linux preview. "
            "Do not use Chromium extension packages as native browser plugins. "
            "Trusted extension support requires a separate security review.")

    def populate_profile_menu(self):
        self.profile_menu.clear()
        head = self.profile_menu.addAction(
            "Local browser profile • no account synchronization")
        head.setEnabled(False)
        self.profile_menu.addSeparator()
        self.profile_menu.addAction("Manage Webspaces…", self.show_webspaces)
        for ident, label in self.webspaces():
            self.profile_menu.addAction(
                f"New tab in {label}",
                lambda checked=False, space=ident: self.new_tab(space_id=space),
            )

    def close_tab(self, index: int):
        widget = self.tabs.widget(index)
        self.tabs.removeTab(index)
        if widget is not None:
            widget.deleteLater()
        if self.tabs.count() == 0:
            self.new_tab()

    def show_home(self, tab=None):
        tab = tab if isinstance(tab, BrowserTab) else self.current()
        if tab:
            tab.show_home()
            self.update_navigation()

    def navigate(self, raw: str, tab=None):
        tab = tab if isinstance(tab, BrowserTab) else self.current()
        try:
            dest = destination_for(raw, clean_links=self.clean_links)
        except ValueError as exc:
            QMessageBox.warning(self, "Unsupported address", str(exc))
            return
        if dest.kind == "home":
            tab.show_home()
        elif dest.kind == "new-tab":
            tab.show_new()
        else:
            # Reject URLs that Qt cannot safely interpret rather than silently
            # displaying a failed/ambiguous navigation in the address bar.
            parsed = QUrl(dest.value)
            if not parsed.isValid() or parsed.scheme() not in ("http", "https") or not parsed.host():
                QMessageBox.warning(self, "Unsupported address",
                                    "The browser engine cannot safely parse this website address.")
                return
            tab.browse(dest.value)
        self.update_navigation()

    def update_tab(self, tab):
        index = self.tabs.indexOf(tab)
        if index < 0:
            return
        address = tab.current_address()
        if address == "goree://home":
            title = "Home"
        elif address == "goree://new-tab":
            title = "New Tab"
        elif address == "goree://settings":
            title = "Settings"
        else:
            title = tab.view.title() or (tab.view.url().host() or "Loading…")
        self.tabs.setTabText(index, title[:27])
        self.tabs.setTabToolTip(index, title)
        if tab is self.current():
            self.update_navigation()

    def update_navigation(self, _index=None):
        tab = self.current()
        if not tab:
            return
        if not self.address.hasFocus():
            self.address.setText(tab.current_address())
        is_web = tab.stack.currentWidget() is tab.view
        self.back.setEnabled(is_web and tab.view.history().canGoBack())
        self.forward.setEnabled(is_web and tab.view.history().canGoForward())
        self.reload.setEnabled(is_web)
        self.setWindowTitle(f"{self.tabs.tabText(self.tabs.currentIndex())} • GoreeCloud Browser")
        self.webspaces_button.setToolTip(
            f"Webspaces • active: {self.webspace_name(tab.space_id)}")
        self.webspaces_button.setAccessibleName(
            f"Webspaces, active {self.webspace_name(tab.space_id)}")

    def go_back(self):
        if self.current().view.history().canGoBack():
            self.current().stack.setCurrentWidget(self.current().view)
            self.current().view.back()

    def go_forward(self):
        if self.current().view.history().canGoForward():
            self.current().stack.setCurrentWidget(self.current().view)
            self.current().view.forward()

    def reload_page(self):
        if self.current().stack.currentWidget() is self.current().view:
            self.current().view.reload()

    def load_progress(self, value: int):
        if self.sender() is not self.current().view:
            return
        self.progress.setValue(value)
        self.progress.setVisible(value < 100)

    def read_bookmarks(self):
        try:
            items = json.loads(self.preferences.value("bookmarks", "[]"))
            return [i for i in items[:500] if isinstance(i, dict)
                    and valid_web_url(i.get("url", ""))]
        except (ValueError, TypeError):
            return []

    def add_bookmark(self):
        tab = self.current()
        url = tab.current_address()
        if not valid_web_url(url):
            self.statusBar().showMessage("Navigate to a website before bookmarking.", 4500)
            return
        items = self.read_bookmarks()
        if any(item["url"] == url for item in items):
            self.statusBar().showMessage("This page is already bookmarked.", 4500)
            return
        if len(items) >= 500:
            QMessageBox.information(self, "Bookmark limit", "The preview supports up to 500 bookmarks.")
            return
        items.append({"title": (tab.view.title() or url)[:150], "url": url})
        self.preferences.setValue("bookmarks", json.dumps(items))
        self.statusBar().showMessage("Bookmark saved locally.", 4000)

    def populate_bookmarks(self):
        self.bookmark_menu.clear()
        self.bookmark_menu.addAction("Save this page", self.add_bookmark)
        self.bookmark_menu.addSeparator()
        items = self.read_bookmarks()
        if not items:
            placeholder = self.bookmark_menu.addAction("No bookmarks yet")
            placeholder.setEnabled(False)
        for item in items:
            url = item["url"]
            self.bookmark_menu.addAction(
                item.get("title", url)[:45], lambda checked=False, u=url: self.navigate(u))

    def open_bookmark_menu(self):
        self.bookmark_button.showMenu()

    def set_preference(self, key: str, value: bool):
        self.preferences.setValue(key, bool(value))
        if key == "block_trackers":
            self.interceptor.block_trackers = bool(value)
        if key == "clean_links":
            self.clean_links = bool(value)
        if key == "persistent_cookies":
            self.persistent_cookies = bool(value)
            for profile in self.space_profiles.values():
                profile.setPersistentCookiesPolicy(
                    QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies
                    if value else QWebEngineProfile.PersistentCookiesPolicy.NoPersistentCookies)
            self.statusBar().showMessage("Cookie persistence updated. Existing site data may remain.", 6000)

    def clear_cookies(self):
        profile = self.current().profile
        profile.cookieStore().deleteAllCookies()
        profile.clearHttpCache()
        self.statusBar().showMessage(
            "Active Webspace cookies and HTTP cache clearing requested.", 5000)

    def request_site_data_reset(self):
        answer = QMessageBox.question(
            self, "Erase website data on restart",
            "Remove all GoreeCloud Browser website storage, cookies, and cache "
            "the next time you close and reopen the browser?\n\n"
            "You may be signed out of websites. Bookmarks and browser "
            "preferences will be kept. The current session is not erased yet.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer != QMessageBox.StandardButton.Yes:
            return
        try:
            new_request = schedule_site_reset(self.site_storage)
        except (OSError, ValueError) as exc:
            QMessageBox.warning(self, "Reset could not be scheduled", str(exc))
            return
        self.statusBar().showMessage(
            "Site data will be erased after you close and reopen the browser."
            if new_request else "Website data erasure is already scheduled.",
            10000,
        )

    def show_privacy(self):
        dialog = PrivacyDialog(self)
        dialog.exec()

    def toggle_theme(self):
        self.set_theme("light" if self.mode == "dark" else "dark")

    def set_theme(self, theme: str):
        if theme not in THEMES:
            raise ValueError("Unknown theme")
        self.mode = theme
        self.preferences.setValue("appearance", theme)
        self.apply_theme()

    def apply_theme(self):
        QApplication.instance().setStyleSheet(stylesheet(self.mode))
        for name, widgets in self._chrome_buttons.items():
            tint = THEMES[self.mode]["brand" if name in ("verify", "webspaces") else "text"]
            for widget in widgets:
                widget.setIcon(icon_for(name, tint))

    def handle_download(self, download: QWebEngineDownloadRequest):
        # Never permit WebEngine to write directly into a chosen final path.
        # The untrusted download lands in a private temporary directory first.
        suggested = suggested_filename(download.downloadFileName())
        directory = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.DownloadLocation)
        proposed = str(Path(directory or str(Path.home() / "Downloads")) / suggested)
        selected, _ = QFileDialog.getSaveFileName(self, "Save downloaded file", proposed)
        if not selected:
            download.cancel()
            return
        try:
            target = validate_target(Path(selected))
            staging, staged_file = new_staging()
        except (OSError, ValueError) as exc:
            download.cancel()
            QMessageBox.warning(self, "Download not started", str(exc))
            return
        try:
            download.setDownloadDirectory(str(staged_file.parent))
            download.setDownloadFileName(staged_file.name)
            key = id(download)
            self._pending_downloads[key] = (download, staging, staged_file, target)
            download.stateChanged.connect(
                lambda _state, k=key: self.finish_download(k))
            download.accept()
            self._download_poll_timer.start()
        except (OSError, ValueError, RuntimeError) as exc:
            self._pending_downloads.pop(id(download), None)
            staging.cleanup()
            download.cancel()
            QMessageBox.warning(self, "Download not started", str(exc))
            return
        self.statusBar().showMessage(f"Downloading {target.name} to a private staging area…", 6000)

    def poll_downloads(self):
        for key in list(self._pending_downloads):
            self.finish_download(key)
        if not self._pending_downloads:
            self._download_poll_timer.stop()

    def record_download(self, target: Path, status: str, progress: str = "") -> None:
        if not getattr(self, "_closing", False):
            self._download_history.append(
                {"filename": target.name, "status": status, "progress": progress})
            self._download_history = self._download_history[-100:]

    def finish_download(self, key: int):
        pending = self._pending_downloads.get(key)
        if pending is None:
            return
        download, staging, staged_file, target = pending
        if not download.isFinished():
            return
        self._pending_downloads.pop(key, None)
        if download.state() == QWebEngineDownloadRequest.DownloadState.DownloadCompleted:
            self.gate_download(key, staging, staged_file, target, download.receivedBytes())
            return
        self.record_download(target, "Not saved")
        if not getattr(self, "_closing", False):
            self.statusBar().showMessage(
                "Download cancelled or interrupted; no file saved.", 7000)
        staging.cleanup()

    def gate_download(self, key, staging, staged_file: Path,
                      target: Path, size: int) -> None:
        """Keep the completed file private until its scan result is known."""
        try:
            fingerprint = staged_fingerprint(staged_file)
            program = find_clamscan()
            if not program:
                answer = QMessageBox.question(
                    self, "Antivirus scanning unavailable",
                    "ClamAV is not installed or was not found. This downloaded "
                    "file has NOT been scanned for malware.\n\n"
                    "Save the unscanned file anyway? It will not be opened "
                    "automatically. Choose No to discard it.",
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                    QMessageBox.StandardButton.No,
                )
                if answer == QMessageBox.StandardButton.Yes:
                    if staged_fingerprint(staged_file) != fingerprint:
                        raise ValueError("Staged file changed before publication.")
                    publish_complete_file(staged_file, target)
                    self.record_download(target, "Saved — NOT SCANNED", f"{size} bytes")
                    self.statusBar().showMessage(
                        f"Saved {target.name} — NOT SCANNED; do not execute blindly.", 10000)
                else:
                    self.record_download(target, "Discarded — not scanned")
                    self.statusBar().showMessage(
                        "Unscanned download discarded; no file saved.", 7000)
                staging.cleanup()
                return

            # Running many clamscan processes simultaneously may exhaust RAM.
            # This preview blocks extra downloads rather than silently saving
            # them without a verdict. A queued scanner is a future feature.
            if len(self._scan_pending) >= MAX_PARALLEL_SCANS:
                self.record_download(target, "Discarded — scanner busy")
                staging.cleanup()
                QMessageBox.warning(
                    self, "Antivirus scanner busy",
                    "Another download is being scanned. This file was discarded "
                    "without being saved. Retry after the scan completes.")
                return

            arguments = scanner_arguments(staged_file)
            # A Qt download wrapper may be reclaimed while its independent
            # antivirus process runs. Never identify scans by that reused id.
            self._next_scan_id += 1
            key = self._next_scan_id
            process = QProcess(self)
            timer = QTimer(self)
            timer.setSingleShot(True)
            self._scan_pending[key] = (
                process, timer, staging, staged_file, target, fingerprint, size)
            process.finished.connect(
                lambda code, exit_status, k=key:
                self.finish_malware_scan(k, code, exit_status))
            process.errorOccurred.connect(
                lambda error, k=key: self.scan_process_error(k, error))
            # Never collect or display potentially sensitive scanner output.
            process.readyReadStandardOutput.connect(
                lambda p=process: p.readAllStandardOutput())
            process.readyReadStandardError.connect(
                lambda p=process: p.readAllStandardError())
            timer.timeout.connect(lambda k=key: self.timeout_malware_scan(k))
            timer.start(SCAN_TIMEOUT_MS)
            process.start(program, arguments)
            self.statusBar().showMessage(
                f"Scanning {target.name} locally with ClamAV…", 7000)
        except (OSError, ValueError) as exc:
            self.record_download(target, "Not saved — unsafe staging")
            staging.cleanup()
            if not getattr(self, "_closing", False):
                QMessageBox.warning(self, "Download not saved", str(exc))

    def scan_process_error(self, key, error) -> None:
        if error == QProcess.ProcessError.FailedToStart:
            self.finish_malware_scan(key, 2, QProcess.ExitStatus.CrashExit)

    def timeout_malware_scan(self, key) -> None:
        job = self._scan_pending.get(key)
        if job is None:
            return
        self._scan_timed_out.add(key)
        job[0].kill()

    def finish_malware_scan(self, key, code, exit_status) -> None:
        job = self._scan_pending.pop(key, None)
        if job is None:
            return
        process, timer, staging, staged_file, target, fingerprint, size = job
        timer.stop()
        timed_out = key in self._scan_timed_out
        self._scan_timed_out.discard(key)
        try:
            verdict = classify_clamscan_result(
                code, not timed_out
                and exit_status == QProcess.ExitStatus.NormalExit)
            if verdict is ScanVerdict.NO_DETECTION:
                if staged_fingerprint(staged_file) != fingerprint:
                    raise ValueError("Staged download changed during scanning.")
                publish_complete_file(staged_file, target)
                self.record_download(target, "Saved — ClamAV no detection",
                                     f"{size} bytes")
                self.statusBar().showMessage(
                    f"Saved {target.name}; ClamAV found no threat (not opened).", 8000)
            elif verdict is ScanVerdict.DETECTED:
                self.record_download(target, "Blocked — ClamAV alert")
                self.statusBar().showMessage(
                    "ClamAV flagged a file. Download discarded.", 10000)
                QMessageBox.warning(
                    self, "ClamAV alert — download blocked",
                    "ClamAV flagged malware, encrypted/uninspectable content, "
                    "or a scan-limit condition. The staged file was not "
                    "published and will be discarded. An alert is not "
                    "necessarily a confirmed malware infection.")
            else:
                self.record_download(target, "Not saved — scanning failed")
                QMessageBox.warning(
                    self, "Antivirus scan failed",
                    "The antivirus scan failed, exceeded a safety limit, "
                    "or timed out. Nothing was saved. Check your local "
                    "ClamAV installation and signature freshness before retrying.")
        except (OSError, ValueError) as exc:
            self.record_download(target, "Not saved — file changed or inaccessible")
            QMessageBox.warning(self, "Download not saved", str(exc))
        finally:
            staging.cleanup()
            timer.deleteLater()
            process.deleteLater()

    def show_about(self):
        QMessageBox.information(
            self, "About GoreeCloud Browser",
            "GoreeCloud Browser • development preview 0.2.7\n\n"
            "Native Qt Widgets interface; pages rendered by Qt WebEngine.\n"
            "Search: Startpage until GoreeCloud Search is verified stable.\n"
            "GoreeShield: privacy settings • Wardveil: security responsibility.\n\n"
            "This preview is not yet ready to replace a fully maintained browser.")

    def install_shortcuts(self):
        commands = [
            ("Ctrl+T", self.new_tab),
            ("Ctrl+W", lambda: self.close_tab(self.tabs.currentIndex())),
            ("Ctrl+L", lambda: (self.address.setFocus(), self.address.selectAll())),
            ("Ctrl+R", self.reload_page),
            ("F5", self.reload_page),
            ("Alt+Home", self.show_home),
            ("Alt+Left", self.go_back),
            ("Alt+Right", self.go_forward),
            ("Ctrl+D", self.add_bookmark),
            ("Ctrl++", lambda: self.zoom(0.1)),
            ("Ctrl+-", lambda: self.zoom(-0.1)),
            ("Ctrl+0", lambda: self.zoom(0)),
        ]
        self._shortcuts = []
        for keys, fn in commands:
            shortcut = QShortcut(QKeySequence(keys), self)
            shortcut.activated.connect(fn)
            self._shortcuts.append(shortcut)

    def zoom(self, delta: float):
        tab = self.current()
        if tab.stack.currentWidget() is not tab.view:
            return
        current = tab.view.zoomFactor()
        tab.view.setZoomFactor(1.0 if delta == 0 else min(3.0, max(0.5, current + delta)))

    def closeEvent(self, event):
        # Qt requires every page to be destroyed before its persistent profile.
        # deleteLater() alone runs too late when QApplication exits immediately.
        if getattr(self, "_closing", False):
            event.accept()
            return
        self._closing = True
        self._download_poll_timer.stop()
        for download, staging, _, _ in list(self._pending_downloads.values()):
            if not download.isFinished():
                download.cancel()
            staging.cleanup()
        self._pending_downloads.clear()
        # Terminate all still-running scanner processes before removing their
        # private files. Closing the browser never publishes unfinished scans.
        pending_scans = list(self._scan_pending.values())
        self._scan_pending.clear()
        self._scan_timed_out.clear()
        for process, timer, staging, _, _, _, _ in pending_scans:
            timer.stop()
            if process.state() != QProcess.ProcessState.NotRunning:
                process.kill()
                process.waitForFinished(500)
            staging.cleanup()
            process.deleteLater()
            timer.deleteLater()
        while self.tabs.count():
            tab = self.tabs.widget(0)
            self.tabs.removeTab(0)
            if tab is not None:
                tab.view.stop()
                destroy_qobject(tab)
        for profile in self.space_profiles.values():
            profile.setUrlRequestInterceptor(None)
            destroy_qobject(profile)
        self.space_profiles.clear()
        self.profile_lock.close()
        self.preferences.sync()
        super().closeEvent(event)


def main() -> int:
    QApplication.setOrganizationName("GoreeCloud")
    QApplication.setApplicationName("GoreeCloudBrowser")
    app = QApplication(sys.argv)
    app.setWindowIcon(browser_icon())
    app.setStyle("Fusion")
    try:
        window = BrowserWindow()
    except WebEngineSecurityError as error:
        # Surface the reason instead of silently removing a security flag or
        # falling back to an unsandboxed renderer.
        print(str(error), file=sys.stderr)
        QMessageBox.critical(None, "Unsafe browser security configuration", str(error))
        return 2
    except RuntimeError as error:
        QMessageBox.critical(None, "Browser profile unavailable", str(error))
        return 1
    window.show()
    return app.exec()

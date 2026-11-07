"""GoreeCloud Browser: native Qt Widgets chrome with Qt WebEngine pages.

No remote service is contacted by Home or New Tab. Network navigation requires
a user action. This is a development preview, not a hardened full browser.
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path

from PySide6.QtCore import Qt, QSettings, QStandardPaths, QUrl, QTimer, Signal
from PySide6.QtGui import QAction, QKeySequence, QShortcut
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QDialog, QFileDialog, QFrame, QHBoxLayout,
    QLabel, QLineEdit, QMainWindow, QMenu, QMessageBox, QProgressBar,
    QPushButton, QScrollArea, QStackedWidget, QTabWidget, QToolButton,
    QVBoxLayout, QWidget,
)
from PySide6.QtWebEngineCore import (
    QWebEngineDownloadRequest, QWebEnginePage, QWebEngineProfile,
    QWebEngineSettings, QWebEngineUrlRequestInfo, QWebEngineUrlRequestInterceptor,
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
    QProgressBar {{ border: none; background: {c['tonal']}; max-height: 3px; }}
    QProgressBar::chunk {{ background: {c['brand']}; }}
    QStatusBar {{ background: {c['surface']}; color: {c['soft']}; }}
    QScrollArea {{ background: {c['canvas']}; border: none; }}
    QScrollBar:vertical {{ background: {c['surface']}; width: 10px; margin: 0; }}
    QScrollBar::handle:vertical {{ background: {c['outline']}; border-radius: 5px; min-height: 36px; }}
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

    def interceptRequest(self, info: QWebEngineUrlRequestInfo) -> None:
        url = info.requestUrl()
        if url.scheme() not in ("http", "https"):
            return
        main_frame = (info.resourceType() ==
                      QWebEngineUrlRequestInfo.ResourceType.ResourceTypeMainFrame)
        if (self.block_trackers and not main_frame and
                is_known_tracker(url.host(), info.firstPartyUrl().host())):
            info.block(True)
            return


class BrowserPage(QWebEnginePage):
    def __init__(self, profile: QWebEngineProfile, owner, parent=None):
        super().__init__(profile, parent)
        self.owner = owner
        # Site capabilities always require consent; no silent grants.
        self.permissionRequested.connect(self.ask_permission)

    def acceptNavigationRequest(self, url: QUrl, nav_type, is_main_frame: bool) -> bool:
        if is_main_frame and url.scheme() not in ("http", "https", "about"):
            self.owner.statusBar().showMessage("Blocked an unsupported navigation scheme.", 5000)
            return False
        return super().acceptNavigationRequest(url, nav_type, is_main_frame)

    def createWindow(self, _type):
        # Open target=_blank in a contained browser tab, not an external window.
        return self.owner.new_tab().view.page()

    def ask_permission(self, permission) -> None:
        origin = permission.origin().host() or "This site"
        answer = QMessageBox.question(
            self.owner, "Website permission",
            f"{origin} requests {permission.permissionType().name}.\n\n"
            "Allow this website permission for the current session?",
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
        note = QLabel("Native pages  •  No remote content until you choose a website  •  Session-only cookies by default")
        note.setObjectName("FinePrint")
        note.setWordWrap(True)
        outer.addWidget(note)
        outer.addStretch(2)


class BrowserTab(QWidget):
    def __init__(self, browser, profile: QWebEngineProfile):
        super().__init__()
        self.browser = browser
        self.stack = QStackedWidget()
        self.view = QWebEngineView()
        self.view.setPage(BrowserPage(profile, browser, self.view))
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

    def browse(self, target: str):
        self.stack.setCurrentWidget(self.view)
        self.view.setUrl(QUrl(target))

    def current_address(self) -> str:
        current = self.stack.currentWidget()
        if current is self.new_scroll:
            return "goree://new-tab"
        if current is self.home_scroll:
            return "goree://home"
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
        self.setWindowTitle("GoreeCloud Browser")
        self.setWindowIcon(browser_icon())
        self.resize(1250, 790)
        self.setMinimumSize(700, 480)
        self.preferences = QSettings("GoreeCloud", "GoreeCloudBrowser")
        self.mode = self.preferences.value("appearance", "dark")
        if self.mode not in THEMES:
            self.mode = "dark"
        self.interceptor = PrivacyInterceptor(self)
        self.interceptor.block_trackers = self.preferences.value("block_trackers", True, type=bool)
        self.clean_links = self.preferences.value("clean_links", True, type=bool)
        self.persistent_cookies = self.preferences.value("persistent_cookies", False, type=bool)

        storage = Path(QStandardPaths.writableLocation(QStandardPaths.StandardLocation.AppDataLocation))
        self.site_storage = storage
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
        self.profile.settings().setUnknownUrlSchemePolicy(
            QWebEngineSettings.UnknownUrlSchemePolicy.DisallowUnknownUrlSchemes)
        self.profile.settings().setAttribute(
            QWebEngineSettings.WebAttribute.JavascriptCanOpenWindows, False)
        self.profile.setUrlRequestInterceptor(self.interceptor)
        self._pending_downloads = {}
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
        navrow.setContentsMargins(16, 12, 16, 10)
        navrow.setSpacing(8)

        self.back = button("‹", "Back (Alt+Left)", self.go_back)
        self.forward = button("›", "Forward (Alt+Right)", self.go_forward)
        self.reload = button("↻", "Reload page (Ctrl+R)", self.reload_page)
        self.home_button = button("⌂", "Home (Alt+Home)", self.show_home)
        for b in (self.back, self.forward, self.reload, self.home_button):
            b.setFixedWidth(45)
            navrow.addWidget(b)
        self.address = QLineEdit()
        self.address.setPlaceholderText("Search with Startpage or enter a URL")
        self.address.setAccessibleName("Address and search bar")
        self.address.returnPressed.connect(lambda: self.navigate(self.address.text()))
        navrow.addWidget(self.address, stretch=1)
        self.bookmark_menu = QMenu(self)
        self.bookmark_menu.aboutToShow.connect(self.populate_bookmarks)
        self.bookmark_button = QToolButton()
        self.bookmark_button.setText("★")
        self.bookmark_button.setToolTip("Bookmarks")
        self.bookmark_button.setAccessibleName("Bookmarks menu")
        self.bookmark_button.setMinimumSize(45, 44)
        self.bookmark_button.setMenu(self.bookmark_menu)
        self.bookmark_button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        navrow.addWidget(self.bookmark_button)

        options_menu = QMenu(self)
        options_menu.addAction("New tab", self.new_tab)
        options_menu.addAction("New home tab", self.open_home_tab)
        options_menu.addSeparator()
        options_menu.addAction("Privacy and cookies…", self.show_privacy)
        options_menu.addAction("Clear cookies and HTTP cache", self.clear_cookies)
        options_menu.addAction("Switch light / dark", self.toggle_theme)
        options_menu.addSeparator()
        options_menu.addAction("About GoreeCloud Browser", self.show_about)
        options_button = QToolButton()
        options_button.setText("⋯")
        options_button.setAccessibleName("Browser menu")
        options_button.setToolTip("Browser menu")
        options_button.setMinimumSize(45, 44)
        options_button.setMenu(options_menu)
        options_button.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        navrow.addWidget(options_button)
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
        plus = button("+", "New tab (Ctrl+T)", self.new_tab)
        plus.setFixedSize(45, 44)
        self.tabs.setCornerWidget(plus, Qt.Corner.TopRightCorner)
        layout.addWidget(self.tabs, stretch=1)
        self.statusBar().showMessage("Ready • Startpage search")
        self.install_shortcuts()
        self.apply_theme()
        self.new_tab()

    def current(self):
        return self.tabs.currentWidget()

    def new_tab(self):
        tab = BrowserTab(self, self.profile)
        index = self.tabs.addTab(tab, "New Tab")
        self.tabs.setCurrentIndex(index)
        tab.new.search.setFocus()
        return tab

    def open_home_tab(self):
        tab = self.new_tab()
        tab.show_home()
        return tab

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
            self.profile.setPersistentCookiesPolicy(
                QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies
                if value else QWebEngineProfile.PersistentCookiesPolicy.NoPersistentCookies)
            self.statusBar().showMessage("Cookie persistence updated. Existing site data may remain.", 6000)

    def clear_cookies(self):
        self.profile.cookieStore().deleteAllCookies()
        self.profile.clearHttpCache()
        self.statusBar().showMessage("Cookies and HTTP cache clearing requested.", 5000)

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
        self.mode = "light" if self.mode == "dark" else "dark"
        self.preferences.setValue("appearance", self.mode)
        self.apply_theme()

    def apply_theme(self):
        QApplication.instance().setStyleSheet(stylesheet(self.mode))

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

    def finish_download(self, key: int):
        pending = self._pending_downloads.get(key)
        if pending is None:
            return
        download, staging, staged_file, target = pending
        if not download.isFinished():
            return
        self._pending_downloads.pop(key, None)
        try:
            if download.state() == QWebEngineDownloadRequest.DownloadState.DownloadCompleted:
                publish_complete_file(staged_file, target)
                self.statusBar().showMessage(f"Saved {target.name} (not opened)", 7000)
            elif not getattr(self, "_closing", False):
                self.statusBar().showMessage("Download cancelled or interrupted; no file saved.", 7000)
        except (OSError, ValueError) as exc:
            if not getattr(self, "_closing", False):
                QMessageBox.warning(self, "Download not saved", str(exc))
        finally:
            staging.cleanup()

    def show_about(self):
        QMessageBox.information(
            self, "About GoreeCloud Browser",
            "GoreeCloud Browser • development preview 0.1.5\n\n"
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
        while self.tabs.count():
            tab = self.tabs.widget(0)
            self.tabs.removeTab(0)
            if tab is not None:
                tab.view.stop()
                destroy_qobject(tab)
        self.profile.setUrlRequestInterceptor(None)
        destroy_qobject(self.profile)
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
    except RuntimeError as error:
        QMessageBox.critical(None, "Browser profile unavailable", str(error))
        return 1
    window.show()
    return app.exec()

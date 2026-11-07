"""Native, accessible management panels for GoreeCloud Browser Linux.

These are Qt widgets, never privileged web pages. Features that are not yet
implemented explicitly say so and do not expose fake settings or controls.
"""
from __future__ import annotations
from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QComboBox, QDialog, QFrame, QGroupBox, QHBoxLayout,
    QInputDialog, QLabel, QLineEdit, QListWidget, QMessageBox, QPlainTextEdit,
    QPushButton, QScrollArea, QSpinBox, QTableWidget, QTableWidgetItem,
    QVBoxLayout, QWidget, QHeaderView,
)


def description(value: str) -> QLabel:
    label = QLabel(value)
    label.setWordWrap(True)
    label.setObjectName("HeroSubtitle")
    return label


def section(title: str, note: str) -> tuple[QGroupBox, QVBoxLayout]:
    group = QGroupBox(title)
    group.setObjectName("SettingsSection")
    layout = QVBoxLayout(group)
    layout.setContentsMargins(20, 24, 20, 18)
    layout.setSpacing(12)
    if note:
        layout.addWidget(description(note))
    return group, layout


def action(text: str, callback, name: str = "") -> QPushButton:
    button = QPushButton(text)
    button.setAccessibleName(name or text)
    button.setMinimumHeight(42)
    button.clicked.connect(callback)
    return button


class SettingsPage(QWidget):
    """Different from Home and New Tab: native multi-section settings surface."""
    def __init__(self, browser):
        super().__init__()
        self.setObjectName("SettingsPage")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        scroll = QScrollArea(self)
        scroll.setObjectName("SettingsScroll")
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        content = QWidget()
        content.setObjectName("SettingsContent")
        stack = QVBoxLayout(content)
        stack.setContentsMargins(28, 30, 28, 36)
        stack.setSpacing(20)
        heading = QLabel("Settings")
        heading.setObjectName("HeroTitle")
        stack.addWidget(heading)
        stack.addWidget(description(
            "GoreeCloud Browser • Native Linux controls. GoreeShield governs privacy; "
            "Wardveil governs security. Unimplemented protections are clearly marked."
        ))

        panel, controls = section("Appearance • Theme Manager",
            "Light is the default for new profiles. A saved user preference always wins.")
        self.theme = QComboBox()
        self.theme.setObjectName("ThemeSelector")
        self.theme.addItems(["Light", "Dark"])
        self.theme.setCurrentText(browser.mode.title())
        self.theme.currentTextChanged.connect(lambda name: browser.set_theme(name.lower()))
        controls.addWidget(self.theme)
        stack.addWidget(panel)

        panel, controls = section("Privacy Shield • Adblocker",
            "Domain filtering is not cosmetic ad removal. Enabled subscriptions "
            "may fetch fixed HTTPS lists at startup, revealing your connection IP "
            "to those providers, never your visited page URLs. Rules are enforced "
            "only after a valid list has loaded.")
        self.trackers = QCheckBox("Block known third-party tracker hosts")
        self.trackers.setChecked(browser.interceptor.block_trackers)
        self.trackers.toggled.connect(lambda enabled: browser.set_preference("block_trackers", enabled))
        controls.addWidget(self.trackers)
        self.clean = QCheckBox("Clean tracking parameters from entered addresses")
        self.clean.setChecked(browser.clean_links)
        self.clean.toggled.connect(lambda enabled: browser.set_preference("clean_links", enabled))
        controls.addWidget(self.clean)
        self.oisd = QCheckBox("OISD Small • enabled by default")
        self.oisd.setChecked(browser.list_enabled("oisd-small"))
        self.oisd.toggled.connect(lambda value: browser.set_list_enabled("oisd-small", value))
        controls.addWidget(self.oisd)
        self.stevenblack = QCheckBox("StevenBlack Unified • enabled by default")
        self.stevenblack.setChecked(browser.list_enabled("stevenblack"))
        self.stevenblack.toggled.connect(lambda value: browser.set_list_enabled("stevenblack", value))
        controls.addWidget(self.stevenblack)
        self.list_status = description(browser.filter_status_text())
        controls.addWidget(self.list_status)
        controls.addWidget(action("Update domain lists", lambda: browser.refresh_blocklists(self.list_status)))
        stack.addWidget(panel)

        panel, controls = section("Webspaces • Website Containers",
            "Each Webspace has its own Qt WebEngine profile, cookie jar and site storage. "
            "This does not isolate processes or conceal your IP address.")
        controls.addWidget(action("Manage Webspaces", browser.show_webspaces))
        stack.addWidget(panel)

        panel, controls = section("Advanced Tab Manager",
            "Search, switch, and close open tabs across Webspaces.")
        controls.addWidget(action("Manage tabs", browser.show_tab_manager))
        stack.addWidget(panel)

        panel, controls = section("Advanced Download Manager",
            "Monitor staged downloads; completed files require explicit save approval. "
            "This preview does not scan files for malware.")
        controls.addWidget(action("Manage downloads", browser.show_download_manager))
        stack.addWidget(panel)

        panel, controls = section("ChatGPT.com • Prompt Integrations",
            "Prompts are only copied when you press Copy. Opening ChatGPT.com never "
            "automatically sends the page, cookies, or prompt contents.")
        self.prompt_templates = {
            "Explain a topic": "Explain the following topic clearly, with examples:\n\n",
            "Summarize supplied text": "Summarize the text I paste below. Identify its main points and uncertainties:\n\n",
            "Draft a message": "Draft a concise and professional message about this topic:\n\n",
            "Compare options": "Compare the following options using explicit pros, cons, and tradeoffs:\n\n",
            "Check a claim": "Evaluate this claim carefully, distinguish evidence from assumptions, and identify what to verify:\n\n",
        }
        self.prompt_template = QComboBox()
        self.prompt_template.setAccessibleName("ChatGPT prompt template")
        self.prompt_template.addItems(self.prompt_templates)
        controls.addWidget(self.prompt_template)
        controls.addWidget(action("Insert selected prompt template", self.insert_prompt_template))
        self.prompt = QPlainTextEdit()
        self.prompt.setObjectName("ChatGPTPromptEditor")
        self.prompt.setAccessibleName("Editable ChatGPT prompt")
        self.prompt.setPlainText("Explain the following topic clearly, with examples:\n\n")
        self.prompt.setMinimumHeight(100)
        controls.addWidget(self.prompt)
        controls.addWidget(action("Copy prompt", self.copy_prompt))
        controls.addWidget(action("Open ChatGPT.com (no prompt sent)",
                                  lambda: browser.navigate("https://chatgpt.com/")))
        stack.addWidget(panel)

        panel, controls = section("Website Permissions",
            "Review or reset stored per-origin website capability decisions "
            "for the active Webspace. Transient active capture permissions may "
            "not appear in Qt's recorded-decision list.")
        controls.addWidget(action("Review website permissions", browser.show_permissions))
        stack.addWidget(panel)

        panel, controls = section("Cookies Manager",
            "Inspect cookie names and domains in the active Webspace without exposing "
            "cookie values in this interface.")
        self.cookies = QCheckBox("Keep cookies after browser exit")
        self.cookies.setChecked(browser.persistent_cookies)
        self.cookies.toggled.connect(lambda value: browser.set_preference("persistent_cookies", value))
        controls.addWidget(self.cookies)
        controls.addWidget(action("Inspect active Webspace cookies", browser.show_cookie_manager))
        controls.addWidget(action("Clear active Webspace cookies and HTTP cache", browser.clear_cookies))
        stack.addWidget(panel)

        panel, controls = section("DNS Manager",
            "The Linux operating system currently resolves DNS for Qt WebEngine. "
            "Encrypted DNS and custom resolvers have NOT been implemented or verified "
            "inside this browser. Changing this panel does not alter system DNS.")
        controls.addWidget(description("Active mode: System DNS (read-only)"))
        stack.addWidget(panel)

        panel, controls = section("Advanced Proxy Manager",
            "Global System, Direct, HTTP, and SOCKS5 modes. Changes take effect "
            "only after restart; all Webspaces share one proxy. No saved passwords "
            "or per-Webspace proxy routes. Filter updates are paused outside System mode.")
        self.proxy_mode = QComboBox()
        self.proxy_mode.addItems(["System", "Direct", "HTTP", "SOCKS5"])
        self.proxy_mode.setCurrentText(browser.preferences.value("proxy_mode", "system").upper()
                                       if browser.preferences.value("proxy_mode", "system") in ("http", "socks5")
                                       else browser.preferences.value("proxy_mode", "system").title())
        self.proxy_mode.setAccessibleName("Proxy connection mode")
        controls.addWidget(self.proxy_mode)
        self.proxy_host = QLineEdit()
        self.proxy_host.setPlaceholderText("Proxy host (HTTP/SOCKS5 only)")
        self.proxy_host.setAccessibleName("Proxy hostname")
        self.proxy_host.setText(browser.preferences.value("proxy_host", ""))
        controls.addWidget(self.proxy_host)
        self.proxy_port = QSpinBox()
        self.proxy_port.setRange(1, 65535)
        try:
            saved_port = int(browser.preferences.value("proxy_port", 8080) or 8080)
        except (TypeError, ValueError):
            saved_port = 8080
        self.proxy_port.setValue(max(1, min(65535, saved_port)))
        self.proxy_port.setAccessibleName("Proxy network port")
        controls.addWidget(self.proxy_port)
        controls.addWidget(action("Save proxy for next restart", self.save_proxy))
        controls.addWidget(description(f"Current session: {browser.proxy_config.mode.title()}"))
        stack.addWidget(panel)

        panel, controls = section("Extensions",
            "Native extension installation and the Chromium extension APIs are "
            "not enabled. An extension button is provided as a clear status entry.")
        controls.addWidget(action("Extensions status", browser.show_extensions))
        stack.addWidget(panel)
        stack.addStretch()
        scroll.setWidget(content)
        layout.addWidget(scroll)

    def save_proxy(self):
        self.parent_browser().configure_proxy(
            self.proxy_mode.currentText().lower(),
            self.proxy_host.text(), self.proxy_port.value())

    def parent_browser(self):
        return self.window()

    def insert_prompt_template(self):
        self.prompt.setPlainText(self.prompt_templates[self.prompt_template.currentText()])

    def copy_prompt(self):
        value = self.prompt.toPlainText()
        if not value.strip():
            return
        QApplication.clipboard().setText(value)


class TabManagerDialog(QDialog):
    def __init__(self, browser):
        super().__init__(browser)
        self.browser = browser
        self.setWindowTitle("Advanced Tab Manager")
        self.resize(680, 480)
        root = QVBoxLayout(self)
        root.addWidget(description("Find and manage open tabs. Webspaces remain isolated."))
        self.search = QLineEdit()
        self.search.setPlaceholderText("Find tabs by title, URL, or Webspace")
        self.search.setAccessibleName("Search open tabs")
        self.search.textChanged.connect(self.refresh)
        root.addWidget(self.search)
        self.list = QListWidget()
        self.list.setAccessibleName("Open tabs")
        self.list.itemDoubleClicked.connect(lambda _: self.switch_selected())
        root.addWidget(self.list)
        buttons = QHBoxLayout()
        buttons.addWidget(action("Switch", self.switch_selected))
        buttons.addWidget(action("Close tab", self.close_selected))
        buttons.addWidget(action("Refresh", self.refresh))
        buttons.addWidget(action("Done", self.accept))
        root.addLayout(buttons)
        self.refresh()

    def refresh(self, *_):
        self.list.clear()
        term = self.search.text().casefold()
        for index in range(self.browser.tabs.count()):
            tab = self.browser.tabs.widget(index)
            title = self.browser.tabs.tabText(index)
            name = self.browser.webspace_name(tab.space_id)
            text = f"{index + 1} · {title}    [{name}]    {tab.current_address()}"
            if term in text.casefold():
                self.list.addItem(text)
                self.list.item(self.list.count() - 1).setData(Qt.ItemDataRole.UserRole, index)

    def selected_index(self):
        item = self.list.currentItem()
        return item.data(Qt.ItemDataRole.UserRole) if item is not None else None

    def switch_selected(self):
        index = self.selected_index()
        if index is not None and 0 <= index < self.browser.tabs.count():
            self.browser.tabs.setCurrentIndex(index)
            self.accept()

    def close_selected(self):
        index = self.selected_index()
        if index is not None and 0 <= index < self.browser.tabs.count():
            self.browser.close_tab(index)
            self.refresh()


class DownloadManagerDialog(QDialog):
    def __init__(self, browser):
        super().__init__(browser)
        self.browser = browser
        self.setWindowTitle("Advanced Download Manager")
        self.resize(680, 430)
        root = QVBoxLayout(self)
        root.addWidget(description(
            "Session-only download activity. Downloads use private staging; "
            "they are never opened automatically."
        ))
        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(["File", "State", "Progress"])
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.table.setAccessibleName("Download activity")
        root.addWidget(self.table)
        row = QHBoxLayout()
        row.addWidget(action("Cancel selected active download", self.cancel_selected))
        row.addWidget(action("Close", self.accept))
        root.addLayout(row)
        self.poller = QTimer(self)
        self.poller.setInterval(350)
        self.poller.timeout.connect(self.refresh)
        self.poller.start()
        self.refresh()

    def refresh(self):
        active = [
            (key, entry[3].name, "Downloading",
             f"{entry[0].receivedBytes()} / {entry[0].totalBytes()} bytes")
            for key, entry in self.browser._pending_downloads.items()
        ]
        completed = [
            (None, row["filename"], row["status"], row.get("progress", ""))
            for row in self.browser._download_history[-80:]
        ]
        self.table.setRowCount(len(active) + len(completed))
        for row_no, (key, name, status, progress) in enumerate(active + completed):
            for column, value in enumerate((name, status, progress)):
                self.table.setItem(row_no, column, QTableWidgetItem(str(value)))
            self.table.item(row_no, 0).setData(Qt.ItemDataRole.UserRole, key)

    def cancel_selected(self):
        row = self.table.currentRow()
        if row < 0:
            return
        item = self.table.item(row, 0)
        key = item.data(Qt.ItemDataRole.UserRole) if item else None
        if key in self.browser._pending_downloads:
            self.browser._pending_downloads[key][0].cancel()
            self.refresh()

    def closeEvent(self, event):
        self.poller.stop()
        super().closeEvent(event)


class WebspacesDialog(QDialog):
    def __init__(self, browser):
        super().__init__(browser)
        self.browser = browser
        self.setWindowTitle("Webspaces • Isolated Website Containers")
        self.resize(560, 390)
        root = QVBoxLayout(self)
        root.addWidget(description(
            "Each Webspace uses separate cookies, cache, and site storage. "
            "Opening an existing tab in a new space creates a new tab; it never "
            "silently migrates website state."
        ))
        self.spaces = QListWidget()
        self.spaces.setAccessibleName("Available Webspaces")
        root.addWidget(self.spaces)
        row = QHBoxLayout()
        row.addWidget(action("Create Webspace", self.create))
        row.addWidget(action("Open new tab here", self.open_selected))
        row.addWidget(action("Close", self.accept))
        root.addLayout(row)
        self.refresh()

    def refresh(self):
        self.spaces.clear()
        for ident, name in self.browser.webspaces():
            self.spaces.addItem(name)
            self.spaces.item(self.spaces.count() - 1).setData(Qt.ItemDataRole.UserRole, ident)

    def create(self):
        name, ok = QInputDialog.getText(self, "New Webspace", "Webspace name")
        if not ok:
            return
        try:
            ident = self.browser.add_webspace(name)
        except (ValueError, OSError) as exc:
            QMessageBox.warning(self, "Webspace not created", str(exc))
            return
        self.refresh()
        for i in range(self.spaces.count()):
            if self.spaces.item(i).data(Qt.ItemDataRole.UserRole) == ident:
                self.spaces.setCurrentRow(i)
                break
        self.open_selected()

    def open_selected(self):
        selected = self.spaces.currentItem()
        if selected is None:
            return
        self.browser.new_tab(space_id=selected.data(Qt.ItemDataRole.UserRole))
        self.accept()


class CookieManagerDialog(QDialog):
    """Asynchronous cookie listing for active Webspace, values intentionally hidden."""
    def __init__(self, browser):
        super().__init__(browser)
        self.browser = browser
        tab = browser.current()
        self.store = tab.profile.cookieStore()
        self.setWindowTitle(f"Cookies • {browser.webspace_name(tab.space_id)}")
        self.resize(620, 420)
        self.cookies = []
        root = QVBoxLayout(self)
        root.addWidget(description(
            "Only cookie names and domains are displayed. Cookie values remain hidden. "
            "Site storage such as IndexedDB is managed separately."
        ))
        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(["Domain", "Name", "Flags"])
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        root.addWidget(self.table)
        row = QHBoxLayout()
        row.addWidget(action("Delete selected cookie", self.delete_selected))
        row.addWidget(action("Clear all cookies in this Webspace", self.clear_all))
        row.addWidget(action("Close", self.accept))
        root.addLayout(row)
        self.store.cookieAdded.connect(self.on_cookie)
        self.store.loadAllCookies()

    def on_cookie(self, cookie):
        key = (bytes(cookie.name()), cookie.domain(), cookie.path())
        if any((bytes(c.name()), c.domain(), c.path()) == key for c in self.cookies):
            return
        self.cookies.append(cookie)
        row = self.table.rowCount()
        self.table.insertRow(row)
        flags = ", ".join(
            x for x, enabled in (("Secure", cookie.isSecure()), ("HttpOnly", cookie.isHttpOnly()))
            if enabled
        ) or "Session/standard"
        for index, value in enumerate((cookie.domain(), bytes(cookie.name()).decode("utf-8", "replace"), flags)):
            self.table.setItem(row, index, QTableWidgetItem(value))

    def delete_selected(self):
        row = self.table.currentRow()
        if 0 <= row < len(self.cookies):
            self.store.deleteCookie(self.cookies.pop(row))
            self.table.removeRow(row)

    def clear_all(self):
        response = QMessageBox.question(
            self, "Clear Webspace cookies",
            "Delete all cookies in the selected Webspace? This may sign you out.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if response == QMessageBox.StandardButton.Yes:
            self.store.deleteAllCookies()
            self.cookies.clear()
            self.table.setRowCount(0)

    def closeEvent(self, event):
        try:
            self.store.cookieAdded.disconnect(self.on_cookie)
        except RuntimeError:
            pass
        super().closeEvent(event)

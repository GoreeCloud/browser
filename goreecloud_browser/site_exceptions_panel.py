"""Native site-specific Privacy Shield domain-filter exceptions UI."""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QDialog, QHBoxLayout, QLineEdit, QListWidget, QMessageBox, QVBoxLayout,
)
from goreecloud_browser.native_panels import description, action
from goreecloud_browser.site_exceptions import exact_host


class SiteExceptionsDialog(QDialog):
    def __init__(self, browser):
        super().__init__(browser)
        self.browser = browser
        self.setWindowTitle("Privacy Shield • Site Exceptions")
        self.resize(670, 450)
        root = QVBoxLayout(self)
        root.addWidget(description(
            "Exception hosts are stored only in local browser preferences. "
            "When an exact website hostname matches the first-party page, "
            "GoreeCloud's known-tracker and OISD/StevenBlack third-party "
            "domain blocking is bypassed for that site. This can expose you "
            "to advertising and tracking; other sites retain their protection. "
            "Exceptions apply across every Webspace in this browser; "
            "they do not grant camera, cookies, or device permissions."
        ))
        self.hostname = QLineEdit()
        self.hostname.setPlaceholderText("Exact hostname, e.g. news.example.org — no URL, IP or wildcard")
        self.hostname.setAccessibleName("Exact website hostname to exempt")
        self.hostname.returnPressed.connect(self.add_host)
        root.addWidget(self.hostname)
        root.addWidget(action("Allow third-party requests on this exact host…", self.add_host))

        self.list = QListWidget()
        self.list.setAccessibleName("Websites with disabled built-in domain blocking")
        root.addWidget(self.list)
        actions = QHBoxLayout()
        actions.addWidget(action("Restore protection on selected host", self.remove_selected))
        actions.addWidget(action("Restore protection on all hosts…", self.clear_all))
        actions.addWidget(action("Close", self.accept))
        root.addLayout(actions)
        root.addWidget(description(
            "These are exact first-party hostnames only. Subdomains are not "
            "implicitly exempt. Reload affected websites to apply changes "
            "to previously loaded content; only new network requests are filtered."
        ))
        self.refresh()

    def refresh(self):
        self.list.clear()
        for hostname in self.browser.site_exceptions:
            self.list.addItem(hostname)

    def add_host(self):
        try:
            hostname = exact_host(self.hostname.text())
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid hostname", str(exc))
            return
        answer = QMessageBox.question(
            self, "Allow tracking-related requests?",
            f"Allow blocked third-party requests while visiting exactly "
            f"{hostname}? This weakens Privacy Shield on that exact host "
            "across all Webspaces and can permit advertising or tracking.\n\nContinue?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer != QMessageBox.StandardButton.Yes:
            return
        try:
            self.browser.add_site_exception(hostname)
        except ValueError as exc:
            QMessageBox.warning(self, "Exception not added", str(exc))
            return
        self.hostname.clear()
        self.refresh()

    def remove_selected(self):
        item = self.list.currentItem()
        if item is None:
            return
        self.browser.remove_site_exception(item.text())
        self.refresh()

    def clear_all(self):
        count = len(self.browser.site_exceptions)
        if not count:
            return
        answer = QMessageBox.question(
            self, "Restore Privacy Shield for all sites?",
            f"Remove all {count} exact-host exceptions and restore domain "
            "filtering on the next requests from those sites?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer != QMessageBox.StandardButton.Yes:
            return
        self.browser.clear_site_exceptions()
        self.refresh()

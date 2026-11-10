"""Native per-Webspace permission controls for the Linux browser.

Qt WebEngine only exposes a session's stored permission decisions through
QWebEngineProfile.listAllPermissions(). Transient device/capture permissions
may not appear; this panel must not claim to enumerate or revoke those.
"""
from __future__ import annotations

from PySide6.QtCore import QUrl
from PySide6.QtWidgets import (
    QCheckBox, QDialog, QHBoxLayout, QLabel, QMessageBox,
    QTableWidget, QTableWidgetItem, QVBoxLayout, QHeaderView,
)
from .native_panels import description, action


def origin_key(url: QUrl) -> tuple[str, str, int] | None:
    """Scheme, hostname and effective port, never a path or a string-prefix."""
    scheme = url.scheme().lower()
    hostname = url.host().lower().rstrip(".")
    if scheme not in ("http", "https") or not url.isValid() or not hostname:
        return None
    return (scheme, hostname, url.port(443 if scheme == "https" else 80))


def display_origin(url: QUrl) -> str:
    """Return an origin only, excluding userinfo, paths, queries and fragments."""
    key = origin_key(url)
    if key is None:
        return "No website loaded"
    scheme, host, port = key
    clean_url = QUrl()
    clean_url.setScheme(scheme)
    clean_url.setHost(host)
    if port != (443 if scheme == "https" else 80):
        clean_url.setPort(port)
    return clean_url.toString(QUrl.ComponentFormattingOption.FullyEncoded)


class PermissionCenterDialog(QDialog):
    """Review and reset stored session decisions in the *active* Webspace."""
    def __init__(self, browser):
        super().__init__(browser)
        self.setWindowTitle("Website permissions")
        self.resize(760, 475)
        tab = browser.current()
        self.profile = tab.profile
        self.space_id = tab.space_id
        self._current_origin = origin_key(QUrl(tab.current_address()))
        self._rows = []

        root = QVBoxLayout(self)
        root.addWidget(description(
            f"Webspace: {browser.webspace_name(tab.space_id)}. "
            "This lists permission decisions currently held in memory by Qt "
            "WebEngine for this Webspace. Use Reset to return a stored decision "
            "to Ask. New permission requests must be approved again."
        ))
        self.site_only = QCheckBox("Only show permissions for the current website")
        self.site_only.setChecked(self._current_origin is not None)
        self.site_only.setEnabled(self._current_origin is not None)
        self.site_only.setAccessibleName("Filter permissions to current website")
        self.site_only.toggled.connect(self.refresh)
        root.addWidget(self.site_only)

        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(["Requesting origin", "Capability", "Decision"])
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.table.setSelectionMode(QTableWidget.SelectionMode.SingleSelection)
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.table.setAccessibleName("Stored website permission decisions")
        root.addWidget(self.table)

        note = QLabel(
            "Only stored decisions are shown; short-lived or active camera, "
            "microphone and screen-capture permissions may not be listed. "
            "Resetting a stored decision does not guarantee an active capture "
            "has stopped. Close the relevant tab to stop ongoing use."
        )
        note.setWordWrap(True)
        note.setObjectName("FinePrint")
        root.addWidget(note)

        row = QHBoxLayout()
        row.addWidget(action("Reset selected to Ask", self.reset_selected))
        row.addWidget(action("Reset all recorded decisions…", self.reset_all))
        row.addWidget(action("Refresh", self.refresh))
        row.addWidget(action("Close", self.accept))
        root.addLayout(row)
        self.refresh()

    def refresh(self, *_args):
        self.table.setRowCount(0)
        self._rows = []
        for permission in self.profile.listAllPermissions():
            if not permission.isValid():
                continue
            key = origin_key(permission.origin())
            if key is None:
                continue  # Do not display unsupported or malformed origins.
            if self.site_only.isChecked() and key != self._current_origin:
                continue
            # State Ask should not be presented as an existing decision.
            state = permission.state()
            if state.name not in ("Granted", "Denied"):
                continue
            row = self.table.rowCount()
            self.table.insertRow(row)
            self._rows.append(permission)
            values = (
                # Qt normally supplies an origin; still redact credentials,
                # URL paths, query tokens and fragments if a malformed or
                # adapter-supplied permission includes them.
                display_origin(permission.origin()),
                permission.permissionType().name,
                state.name,
            )
            for col, value in enumerate(values):
                self.table.setItem(row, col, QTableWidgetItem(value))

    def reset_selected(self):
        row = self.table.currentRow()
        if 0 <= row < len(self._rows):
            permission = self._rows[row]
            if permission.isValid():
                permission.reset()
            self.refresh()

    def reset_all(self):
        decisions = [permission for permission in self.profile.listAllPermissions()
                     if permission.isValid() and permission.state().name in ("Granted", "Denied")]
        if not decisions:
            self.refresh()
            return
        answer = QMessageBox.question(
            self, "Reset recorded Webspace permissions",
            f"Reset {len(decisions)} stored permission decisions for this Webspace "
            "back to Ask? Websites may ask again. No other Webspace is affected.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer != QMessageBox.StandardButton.Yes:
            return
        for permission in decisions:
            if permission.isValid():
                permission.reset()
        self.refresh()

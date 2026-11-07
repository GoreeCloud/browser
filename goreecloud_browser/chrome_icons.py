"""First-party scalable monochrome chrome glyphs for native GoreeCloud Browser.

SVG artwork is deliberately simple stroke geometry, tinted using the approved
GoreeCloud UI theme tokens; it is not a replacement brand mark.
"""
from PySide6.QtCore import QByteArray, Qt
from PySide6.QtGui import QIcon, QPainter, QPixmap
from PySide6.QtSvg import QSvgRenderer

# Small monochrome navigation/status symbols (24x24 viewport).
PATHS = {
    "back": '<path d="M15 18 9 12l6-6"/><path d="M9 12h11"/>',
    "forward": '<path d="m9 6 6 6-6 6"/><path d="M4 12h11"/>',
    "refresh": '<path d="M20 7v5h-5"/><path d="M4 17v-5h5"/><path d="M6.3 8A7.5 7.5 0 0 1 19 11M17.7 16A7.5 7.5 0 0 1 5 13"/>',
    "home": '<path d="m3 11 9-8 9 8"/><path d="M5 10v10h14V10"/><path d="M10 20v-6h4v6"/>',
    "bookmark": '<path d="M6 4h12v17l-6-4-6 4z"/>',
    "extensions": '<path d="M4 4h6a2 2 0 0 1 4 0h6v6a2 2 0 0 1 0 4v6h-6a2 2 0 0 1-4 0H4v-6a2 2 0 0 1 0-4z"/>',
    "settings": '<circle cx="12" cy="12" r="3"/><path d="M10 2h4l.7 2.5 2.2.9L19.2 4l2.8 2.8-1.4 2.3.9 2.2L24 12l-2.5.7-.9 2.2 1.4 2.3-2.8 2.8-2.3-1.4-2.2.9L14 22h-4l-.7-2.5-2.2-.9L4.8 20 2 17.2l1.4-2.3-.9-2.2L0 12l2.5-.7.9-2.2L2 6.8 4.8 4l2.3 1.4 2.2-.9z" transform="translate(2 2) scale(.83)"/>',
    "profile": '<circle cx="12" cy="12" r="9"/><circle cx="12" cy="9.5" r="3"/><path d="M6.5 19a6.1 6.1 0 0 1 11 0"/>',
    "verify": '<path d="m12 2 8 3v6c0 5-3.2 8.3-8 11-4.8-2.7-8-6-8-11V5z"/><path d="m8 12 3 3 5-6"/>',
    "permissions": '<path d="M3 7h18M3 17h18"/><circle cx="9" cy="7" r="3"/><circle cx="16" cy="17" r="3"/>',
    "webspaces": '<rect x="3" y="3" width="12" height="12" rx="2"/><rect x="9" y="9" width="12" height="12" rx="2"/>',
    "plus": '<path d="M12 5v14M5 12h14"/>',
    "download": '<path d="M12 3v12m-4-4 4 4 4-4M4 18v3h16v-3"/>',
    "tab": '<rect x="3" y="5" width="18" height="15" rx="2"/><path d="M3 10h18"/>',
}


def icon_for(name: str, color: str, size: int = 22) -> QIcon:
    if name not in PATHS:
        raise ValueError("Unknown chrome icon")
    if len(color) != 7 or color[0] != "#" or not all(
        character in "0123456789abcdefABCDEF" for character in color[1:]
    ):
        raise ValueError("Chrome icon color must be a hex RGB token")
    svg = (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" '
        f'viewBox="0 0 24 24" fill="none" stroke="{color}" '
        f'stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">'
        f'{PATHS[name]}</svg>'
    )
    renderer = QSvgRenderer(QByteArray(svg.encode("utf-8")))
    if not renderer.isValid():
        raise ValueError("Invalid native browser chrome glyph")
    pixmap = QPixmap(size, size)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    renderer.render(painter)
    painter.end()
    return QIcon(pixmap)

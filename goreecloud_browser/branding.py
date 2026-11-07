"""First-party Prism Tabs v5 GoreeCloud Browser identity.

Packaged resources are pinned derivatives of the founder-approved canonical
SVG in GoreeCloud/brand-assets. No network or runtime icon download.
"""
from pathlib import Path

_ASSETS = Path(__file__).resolve().parent / "assets"


def browser_icon_path(size: int = 256) -> Path:
    if size not in (48, 256):
        raise ValueError("Unsupported packaged browser icon size.")
    return _ASSETS / f"app-icon-{size}.png"


def browser_icon():
    """Build Qt window/taskbar icon, importing Qt only when actually needed."""
    from PySide6.QtGui import QIcon
    icon = QIcon(str(browser_icon_path(256)))
    icon.addFile(str(browser_icon_path(48)))
    return icon

"""Qt signal bridge for asynchronous, HTTPS-only domain-list updates."""
from __future__ import annotations
from pathlib import Path
import threading
from PySide6.QtCore import QObject, Signal
from goreecloud_browser.filters import (
    SOURCES, retrieve_subscription, save_validated_cache, read_cached,
)

class FilterUpdateSignal(QObject):
    completed = Signal(object)


class FilterUpdater:
    """Asynchronous fixed-source HTTPS fetch; GUI thread receives a snapshot."""
    def __init__(self):
        self.signals = FilterUpdateSignal()
        self.running = False

    def start(self, root: Path, names: tuple[str, ...]) -> bool:
        if self.running or not names:
            return False
        if any(name not in SOURCES for name in names):
            raise ValueError("Unknown filter subscription")
        self.running = True

        def work():
            result = {}
            try:
                for name in names:
                    try:
                        data = retrieve_subscription(name)
                        domains = save_validated_cache(root, name, data)
                        result[name] = {"domains": domains, "error": None}
                    except (OSError, ValueError, TimeoutError) as exc:
                        result[name] = {"domains": read_cached(root, name), "error": str(exc)[:160]}
            finally:
                self.signals.completed.emit(result)
        threading.Thread(target=work, name="GoreeBrowserFilterUpdate", daemon=True).start()
        return True

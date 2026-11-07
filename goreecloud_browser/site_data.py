"""Linux browser-profile mutual exclusion and explicitly scheduled website-data reset.

WebEngine must have no active page/profile when its storage is removed.
All operations are scoped to the application's private AppDataLocation.
"""
from __future__ import annotations

import fcntl
import os
from pathlib import Path
import shutil

LOCK_NAME = ".linux-browser.lock"
RESET_MARKER = ".clear-sites-on-next-start"
RESET_SIGNATURE = b"GoreeCloud Browser website-data-reset v1\n"
SITE_DIRS = ("web-profile", "cache")


def _storage_root(storage: Path) -> Path:
    storage = Path(storage).expanduser()
    if storage.is_symlink():
        raise ValueError("Refusing to use symlinked browser data directory.")
    storage.mkdir(parents=True, exist_ok=True, mode=0o700)
    if not storage.is_dir():
        raise ValueError("Browser data directory is not a directory.")
    return storage


class ProfileLock:
    """Advisory OS lock to prevent multiple processes sharing one Qt profile."""

    def __init__(self, storage: Path):
        storage = _storage_root(storage)
        target = storage / LOCK_NAME
        flags = os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW
        fd = os.open(target, flags, 0o600)
        try:
            if not target.is_file() or target.is_symlink():
                raise ValueError("Unsafe browser profile lock file.")
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BaseException:
            os.close(fd)
            raise
        self._fd = fd

    def close(self):
        if self._fd is not None:
            fd, self._fd = self._fd, None
            fcntl.flock(fd, fcntl.LOCK_UN)
            os.close(fd)


def schedule_site_reset(storage: Path) -> bool:
    """Write a strict marker, only after an explicit native UI confirmation."""
    storage = _storage_root(storage)
    marker = storage / RESET_MARKER
    if marker.is_symlink():
        raise ValueError("Refusing a symlinked reset marker.")
    if marker.exists():
        if marker.read_bytes() != RESET_SIGNATURE:
            raise ValueError("Refusing to replace an unrecognized reset marker.")
        return False
    fd = os.open(marker, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, "wb") as file:
        file.write(RESET_SIGNATURE)
        file.flush()
        os.fsync(file.fileno())
    return True


def apply_pending_site_reset(storage: Path) -> bool:
    """Clear only Qt-managed site and cache directories before profile creation.

    The caller MUST hold ProfileLock and must NOT have created QWebEngineProfile.
    Unrelated files, bookmarks, and preferences are never targeted.
    """
    storage = _storage_root(storage)
    marker = storage / RESET_MARKER
    if marker.is_symlink():
        raise ValueError("Refusing a symlinked reset marker.")
    if not marker.exists():
        return False
    if not marker.is_file() or marker.read_bytes() != RESET_SIGNATURE:
        raise ValueError("Reset request marker is invalid; site data is unchanged.")
    targets = [storage / name for name in SITE_DIRS]
    for target in targets:
        if target.is_symlink() or (target.exists() and not target.is_dir()):
            raise ValueError("Unsafe site data path; reset was not attempted.")
    for target in targets:
        if target.exists():
            shutil.rmtree(target)
    marker.unlink()
    return True

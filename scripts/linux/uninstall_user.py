#!/usr/bin/env python3
"""Remove only GoreeCloud Browser's Linux per-user launcher and program runtime.

Existing browsing data, QSettings preferences and bookmarks are deliberately
preserved. Never run while the application is open.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import sys

try:
    from scripts.linux.install_user import APP_ID, MARKER, data_home_from_env
except ModuleNotFoundError:
    # Support direct invocation by absolute path from any working directory.
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
    from scripts.linux.install_user import APP_ID, MARKER, data_home_from_env


def uninstall(data_home: Path) -> bool:
    if not sys.platform.startswith("linux"):
        raise RuntimeError("This uninstaller supports Linux only.")
    data_home = data_home.expanduser().resolve()
    root = data_home / "goreecloud-browser"
    runtime = root / "runtime"
    launcher = data_home / "applications" / (APP_ID + ".desktop")
    if not runtime.exists() and not launcher.exists():
        return False
    if root.is_symlink() or runtime.is_symlink() or launcher.is_symlink():
        raise ValueError("Refusing to remove a symlinked installation.")
    if not runtime.is_dir() or not launcher.is_file():
        raise ValueError("Incomplete installation: manual inspection required.")
    text = launcher.read_text(encoding="utf-8")
    if MARKER not in text.splitlines()[:4]:
        raise ValueError("Desktop launcher is not managed by GoreeCloud Browser.")
    expected = str(runtime / "venv" / "bin" / "python")
    if expected not in text:
        raise ValueError("Launcher points at an unexpected runtime; refusing removal.")
    launcher.unlink()
    shutil.rmtree(runtime)
    try:
        root.rmdir()  # only if empty; never touch browsing data
    except OSError:
        pass
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-home", type=Path, default=data_home_from_env())
    args = parser.parse_args()
    try:
        removed = uninstall(args.data_home)
    except (RuntimeError, OSError, ValueError) as error:
        print(f"Uninstall was not completed: {error}", file=sys.stderr)
        return 1
    if removed:
        print("Removed the user launcher and GoreeCloud Browser program runtime.")
        print("Browsing data, bookmarks and preferences were intentionally preserved.")
    else:
        print("No managed user installation found.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

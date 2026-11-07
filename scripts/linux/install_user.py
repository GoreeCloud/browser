#!/usr/bin/env python3
"""Linux-only, non-root GoreeCloud Browser development-preview installer.

The source is installed into an isolated per-user virtualenv. It deliberately
does not register as the system's default browser or request administrator access.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import venv


APP_ID = "com.goreecloud.browser"
MARKER = "# Managed by GoreeCloud Browser Linux user installer"
ROOT = Path(__file__).resolve().parents[2]


def data_home_from_env() -> Path:
    configured = os.environ.get("XDG_DATA_HOME")
    return Path(configured).expanduser() if configured else Path.home() / ".local" / "share"


def desktop_exec_arg(value: Path) -> str:
    """Strict freedesktop Exec escaping for an absolute Python executable."""
    raw = str(value)
    if not value.is_absolute() or any(c in raw for c in '\n\r\x00"%\\' + chr(96) + '$'):
        raise ValueError("Unsafe desktop launcher executable path.")
    return f'"{raw}"'


def desktop_entry(python_exe: Path) -> str:
    command = desktop_exec_arg(python_exe)
    return (
        "[Desktop Entry]\n"
        f"{MARKER}\n"
        "Type=Application\n"
        "Version=1.0\n"
        "Name=GoreeCloud Browser (Preview)\n"
        "Comment=Native Linux browser development preview\n"
        f"Exec={command} -m goreecloud_browser\n"
        f"TryExec={command}\n"
        "Icon=web-browser\n"
        "Terminal=false\n"
        "StartupNotify=true\n"
        "Categories=Network;WebBrowser;\n"
    )


def validate_desktop(entry: Path) -> None:
    validator = shutil.which("desktop-file-validate")
    if validator:
        subprocess.run([validator, str(entry)], check=True)


def install(data_home: Path) -> tuple[Path, Path]:
    if not sys.platform.startswith("linux"):
        raise RuntimeError("This installer currently supports Linux only.")
    data_home = data_home.expanduser().resolve()
    runtime_dir = data_home / "goreecloud-browser" / "runtime"
    application_file = data_home / "applications" / (APP_ID + ".desktop")
    # Refuse to modify any pre-existing installation without an upgrade/backup plan.
    if runtime_dir.exists() or runtime_dir.is_symlink() or application_file.exists() or application_file.is_symlink():
        raise FileExistsError("An installation or launcher already exists. No files were changed.")
    if (data_home / "goreecloud-browser").is_symlink():
        raise ValueError("Refusing a symlinked installation root.")
    runtime_dir.parent.mkdir(parents=True, exist_ok=True)
    runtime_dir.mkdir(mode=0o700)
    launcher_written = False
    try:
        venv_dir = runtime_dir / "venv"
        venv.EnvBuilder(with_pip=True).create(str(venv_dir))
        py = venv_dir / "bin" / "python"
        subprocess.run(
            [str(py), "-m", "pip", "install", "--no-input",
             "--disable-pip-version-check", str(ROOT)],
            check=True,
        )
        # Test package discoverability before registering a desktop launcher.
        subprocess.run(
            [str(py), "-c", "import goreecloud_browser; from goreecloud_browser.app import main"],
            check=True,
        )
        application_file.parent.mkdir(parents=True, exist_ok=True)
        # Write only after all dependencies installed and imported successfully.
        content = desktop_entry(py)
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", prefix=".goreecloud-browser-",
            suffix=".desktop", dir=application_file.parent, delete=False
        ) as staged:
            temp_path = Path(staged.name)
            staged.write(content)
            staged.flush()
            os.fsync(staged.fileno())
        try:
            validate_desktop(temp_path)
            if application_file.exists() or application_file.is_symlink():
                raise FileExistsError("Desktop launcher appeared during installation.")
            temp_path.chmod(0o644)
            temp_path.replace(application_file)
            launcher_written = True
        finally:
            if temp_path.exists():
                temp_path.unlink()
        return py, application_file
    except BaseException:
        if launcher_written and application_file.is_file():
            application_file.unlink()
        # Remove *only* the fresh runtime created by this invocation.
        if runtime_dir.is_dir() and not runtime_dir.is_symlink():
            shutil.rmtree(runtime_dir)
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--data-home", type=Path, default=data_home_from_env(),
        help="XDG user data directory (default: XDG_DATA_HOME or ~/.local/share)",
    )
    args = parser.parse_args()
    try:
        py, desktop = install(args.data_home)
    except (FileExistsError, OSError, ValueError, subprocess.CalledProcessError,
            RuntimeError) as error:
        print(f"Installation not completed: {error}", file=sys.stderr)
        return 1
    print("Installed GoreeCloud Browser development preview (current user only).")
    print(f"Command: {py} -m goreecloud_browser")
    print(f"Desktop launcher: {desktop}")
    print("No default-browser change, system installation or signed release occurred.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Linux-first download staging and atomic no-overwrite publication.

No remote content is executed, inspected or trusted by these functions.
The browser downloads to a private temporary directory, then publishes only
a completed regular file to the user-selected filename.
"""
from __future__ import annotations

import errno
import os
from pathlib import Path
import secrets
import shutil
import stat
import tempfile

_INVALID_FILENAME = frozenset('<>:"/\\|?*')
_BIDI = frozenset("\u200e\u200f\u202a\u202b\u202c\u202d\u202e\u2066\u2067\u2068\u2069")


def suggested_filename(source: str) -> str:
    """Remove path fragments and unsafe display/control characters."""
    basename = str(source).replace("\\", "/").rsplit("/", 1)[-1]
    safe = "".join(ch for ch in basename
                   if ch.isprintable() and ch not in _INVALID_FILENAME and ch not in _BIDI)
    safe = safe.strip().strip(".").strip()[:160].rstrip(". ")
    return safe or "download"


def validate_target(path: Path) -> Path:
    """Require a new filename inside an existing ordinary directory.

    The actual no-overwrite guarantee occurs at os.link(), not this precheck.
    """
    target = Path(path).expanduser()
    if not target.is_absolute() or target.name in ("", ".", ".."):
        raise ValueError("Select an absolute download filename.")
    parent = target.parent
    if parent.is_symlink() or not parent.is_dir():
        raise ValueError("Download destination must be an existing non-symlink directory.")
    if os.path.lexists(target):
        raise FileExistsError("File already exists. Choose a different filename.")
    return target


def new_staging() -> tuple[tempfile.TemporaryDirectory, Path]:
    """Own-private stage location and randomized name."""
    stage = tempfile.TemporaryDirectory(prefix="goreecloud-download-")
    return stage, Path(stage.name) / ("payload-" + secrets.token_hex(12))


def publish_complete_file(staged: Path, target: Path) -> Path:
    """Atomically expose the *complete* file without replacing an existing path.

    Never follow a source symlink. On EXDEV copy to a private sibling temp
    and atomically hardlink that complete temp to the requested filename.
    """
    target = validate_target(target)
    staged = Path(staged)
    st = os.lstat(staged)
    if not stat.S_ISREG(st.st_mode) or st.st_nlink != 1:
        raise ValueError("Refusing a nonregular or linked staged file.")
    os.chmod(staged, 0o600, follow_symlinks=False)
    try:
        os.link(staged, target, follow_symlinks=False)
        return target
    except OSError as exc:
        if exc.errno != errno.EXDEV:
            raise

    # The publication target may live on another filesystem. A private
    # sibling temporary file is fully copied before its atomic hardlink.
    temporary_name = None
    try:
        with tempfile.NamedTemporaryFile(
            prefix=".goreecloud-download-", dir=target.parent, delete=False
        ) as output:
            temporary_name = Path(output.name)
            os.chmod(temporary_name, 0o600)
            with staged.open("rb") as source:
                shutil.copyfileobj(source, output, length=1024 * 1024)
            output.flush()
            os.fsync(output.fileno())
        os.link(temporary_name, target, follow_symlinks=False)
        return target
    finally:
        if temporary_name is not None:
            temporary_name.unlink(missing_ok=True)

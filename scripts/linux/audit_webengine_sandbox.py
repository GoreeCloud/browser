#!/usr/bin/env python3
"""Read-only, offline Qt WebEngine process-sandbox observation for Linux.

Launches only local HTML in a temporary XDG profile and reads the process tree
from /proc. Reports observed namespace/seccomp evidence, never a blanket
security certification. No changes to system userns, AppArmor or DNS settings.
"""
from __future__ import annotations

import os
from pathlib import Path
import sys
import tempfile


# Only a fixed taxonomy is emitted. Never print arbitrary Chromium argv:
# arguments may include profile paths, cookies, URLs or future secrets.
_PROCESS_TYPES = frozenset({"renderer", "gpu-process", "utility", "zygote", "broker"})


def safe_webengine_role(raw: bytes) -> str:
    """Conservative process role hint extracted from NUL-delimited argv."""
    if not isinstance(raw, bytes):
        return "unreadable"
    markers = [arg[7:] for arg in raw.split(b"\x00") if arg.startswith(b"--type=")]
    if not markers:
        return "unspecified"
    if len(markers) != 1:
        return "ambiguous"
    try:
        name = markers[0].decode("ascii", errors="strict")
    except UnicodeDecodeError:
        return "unrecognized"
    return name if name in _PROCESS_TYPES else "unrecognized"


def observe_descendants(parent_pid: int) -> list[dict[str, object]]:
    """Read only this audit process's descendants using /proc task children."""
    proc = Path("/proc")
    discovered = {parent_pid}
    pending = [parent_pid]
    while pending:
        current_pid = pending.pop()
        tasks = proc / str(current_pid) / "task"
        try:
            tids = list(tasks.iterdir())
        except OSError:
            continue
        for task in tids:
            try:
                raw_children = (task / "children").read_text(encoding="ascii")
            except OSError:
                continue
            for value in raw_children.split():
                try:
                    child_pid = int(value)
                except ValueError:
                    continue
                if child_pid not in discovered:
                    discovered.add(child_pid)
                    pending.append(child_pid)

    findings: list[dict[str, object]] = []
    for pid in discovered - {parent_pid}:
        path = proc / str(pid)
        try:
            lines = (path / "status").read_text(
                encoding="ascii", errors="replace").splitlines()
            fields = dict(line.split(":", 1) for line in lines if ":" in line)
            # Chromium may rewrite /proc/PID/cmdline. Prefer the kernel
            # executable symlink, falling back to truncated process comm.
            try:
                executable = os.path.basename(os.readlink(path / "exe"))
            except OSError:
                executable = fields.get("Name", "").strip()
        except OSError:
            continue
        if not executable.startswith("QtWebEngineProc"):
            continue
        try:
            with (path / "cmdline").open("rb") as command:
                # Bounded read; only the recognized --type value survives.
                role = safe_webengine_role(command.read(4096))
        except OSError:
            role = "unreadable"
        try:
            own_ns = proc / str(parent_pid) / "ns"
            child_ns = path / "ns"
            namespace_diff = {
                name: os.readlink(child_ns / name) != os.readlink(own_ns / name)
                for name in ("user", "pid", "net")
            }
        except OSError:
            namespace_diff = {}
        findings.append({
            "role": role,
            "seccomp": fields.get("Seccomp", "").strip(),
            "seccomp_filters": fields.get("Seccomp_filters", "").strip(),
            "no_new_privs": fields.get("NoNewPrivs", "").strip(),
            "namespace_distinct_from_parent": namespace_diff,
        })
    return findings


def main() -> int:
    if not sys.platform.startswith("linux"):
        print("UNSUPPORTED: only Linux /proc is supported", file=sys.stderr)
        return 3

    # Support direct execution from a source checkout without requiring a
    # package install or modifying a user's existing browser runtime.
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
    from goreecloud_browser.sandbox import (
        WebEngineSecurityError, require_safe_webengine_settings,
    )
    try:
        require_safe_webengine_settings(os.environ, sys.argv[1:])
    except WebEngineSecurityError as error:
        print("REFUSED:", error, file=sys.stderr)
        return 2

    # Avoid any real Firefox/Chrome/GoreeCloud user profile or remote HTML.
    with tempfile.TemporaryDirectory(prefix="gcb-webengine-security-audit-") as home:
        os.environ["XDG_CONFIG_HOME"] = str(Path(home) / "config")
        os.environ["XDG_DATA_HOME"] = str(Path(home) / "data")
        from PySide6.QtCore import QTimer, QUrl
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineWidgets import QWebEngineView

        app = QApplication([sys.argv[0]])
        view = QWebEngineView()
        view.resize(480, 280)
        view.setHtml(
            "<!doctype html><html><title>Local sandbox diagnostic</title>"
            "<body><p>GoreeCloud local WebEngine process audit</p>"
            "<script>document.title = 'Offline renderer';</script></body></html>",
            QUrl("about:blank"),
        )
        view.show()
        findings: list[dict[str, object]] = []

        def sample():
            findings.extend(observe_descendants(os.getpid()))
            view.close()
            app.quit()

        QTimer.singleShot(2800, sample)
        QTimer.singleShot(11000, app.quit)
        app.exec()
        from shiboken6 import delete as destroy_qobject
        view.close()
        app.processEvents()
        destroy_qobject(view)
        destroy_qobject(app)

    total = len(findings)
    namespace_isolated = sum(
        1 for row in findings
        if row["namespace_distinct_from_parent"].get("user") is True
    )
    seccomp_filtered = sum(1 for row in findings if row["seccomp"] == "2")
    combined = sum(
        1 for row in findings if row["seccomp"] == "2"
        and row["no_new_privs"] == "1"
        and row["namespace_distinct_from_parent"].get("user") is True
    )
    print(f"WebEngine child descendants observed: {total}")
    # These labels are process-reported hints, not proof of kernel isolation,
    # and unrecognized roles are deliberately not guessed.
    role_counts = {
        role: sum(row["role"] == role for row in findings)
        for role in sorted({row["role"] for row in findings})
    }
    print("Subprocess role hints (bounded /proc cmdline): "
          + (", ".join(f"{role}={count}" for role, count in role_counts.items())
             if role_counts else "none"))
    renderer_combined = sum(
        1 for row in findings if row["role"] == "renderer"
        and row["seccomp"] == "2" and row["no_new_privs"] == "1"
        and row["namespace_distinct_from_parent"].get("user") is True
    )
    print(f"Renderer-role samples with combined layered evidence: {renderer_combined}")
    if not any(row["role"] == "renderer" for row in findings):
        print("LIMITATION: renderer role was not identifiable from process "
              "arguments; renderer-specific confinement remains unverified.")
    print(f"Separate user namespace observed: {namespace_isolated}")
    print(f"Seccomp-BPF active in a descendant: {seccomp_filtered}")
    print(f"Combined seccomp + user-namespace + no-new-privileges evidence: {combined}")
    print("Interpretation: evidence for some subprocesses only; neither all renderer "
          "roles nor all Chromium sandbox layers are certified by /proc alone.")
    if not combined:
        print("INCONCLUSIVE: secure renderer isolation was not established.", file=sys.stderr)
        return 3
    print("OBSERVED: at least one layered WebEngine child confinement.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

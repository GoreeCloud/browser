#!/usr/bin/env python3
"""Fail-closed Browser source-security audit.

This is intentionally narrow. It checks repository/history secret signatures and
immutable GitHub Actions references. It is not a vulnerability scanner, SBOM
generator, runtime security test, or production acceptance gate.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass

ROOT = pathlib.Path(__file__).resolve().parents[1]

SECRET_PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "private-key",
        re.compile(
            r"-----BEGIN (?:RSA |EC |DSA |OPENSSH )?PRIVATE KEY-----"
        ),
    ),
    ("github-token", re.compile(r"\bgh[pousr]_[A-Za-z0-9]{20,}\b")),
    ("aws-access-key", re.compile(r"\bAKIA[0-9A-Z]{16}\b")),
    ("google-api-key", re.compile(r"\bAIza[0-9A-Za-z_-]{35}\b")),
    ("slack-token", re.compile(r"\bxox[baprs]-[0-9A-Za-z-]{10,}\b")),
)

ACTION_USE = re.compile(r"^\s*-?\s*uses:\s*[\"']?([^\"'\s]+)")
IMMUTABLE_SHA = re.compile(r"^[0-9a-fA-F]{40}$")
WORKFLOW_SUFFIXES = {".yml", ".yaml"}


@dataclass(frozen=True)
class Finding:
    control: str
    location: str
    detail: str

    def render(self) -> str:
        return f"{self.control}: {self.location}: {self.detail}"


def run_git(*args: str) -> str:
    completed = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return completed.stdout


def tracked_text_files() -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for item in run_git("ls-files", "-z").split("\0"):
        if not item:
            continue
        path = ROOT / item
        if path.is_file():
            files.append(path)
    return files


def scan_current_tree() -> list[Finding]:
    findings: list[Finding] = []
    for path in tracked_text_files():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        rel = path.relative_to(ROOT).as_posix()
        for line_number, line in enumerate(text.splitlines(), start=1):
            for name, pattern in SECRET_PATTERNS:
                if pattern.search(line):
                    findings.append(
                        Finding(
                            "secret-current-tree",
                            f"{rel}:{line_number}",
                            name,
                        )
                    )
    return findings


def scan_history() -> list[Finding]:
    findings: list[Finding] = []
    patch = run_git(
        "log",
        "--all",
        "-p",
        "--no-ext-diff",
        "--unified=0",
        "--format=GC_AUDIT_COMMIT:%H",
        "--",
        ".",
    )

    commit = "unknown"
    file_path = "unknown"
    for raw_line in patch.splitlines():
        if raw_line.startswith("GC_AUDIT_COMMIT:"):
            commit = raw_line.split(":", 1)[1]
            continue
        if raw_line.startswith("+++ b/"):
            file_path = raw_line[6:]
            continue
        if not raw_line.startswith(("+", "-")) or raw_line.startswith(("+++", "---")):
            continue
        line = raw_line[1:]
        for name, pattern in SECRET_PATTERNS:
            if pattern.search(line):
                findings.append(
                    Finding(
                        "secret-git-history",
                        f"{commit}:{file_path}",
                        name,
                    )
                )
    return findings


def scan_action_pins() -> list[Finding]:
    findings: list[Finding] = []
    workflow_dir = ROOT / ".github" / "workflows"
    if not workflow_dir.exists():
        return [
            Finding(
                "ci-action-pinning",
                ".github/workflows",
                "workflow directory is missing",
            )
        ]

    workflow_files = sorted(
        path
        for path in workflow_dir.iterdir()
        if path.is_file() and path.suffix in WORKFLOW_SUFFIXES
    )
    if not workflow_files:
        return [
            Finding(
                "ci-action-pinning",
                ".github/workflows",
                "no workflow files found",
            )
        ]

    for path in workflow_files:
        rel = path.relative_to(ROOT).as_posix()
        text = path.read_text(encoding="utf-8")
        for line_number, line in enumerate(text.splitlines(), start=1):
            if line.lstrip().startswith("#"):
                continue
            match = ACTION_USE.search(line)
            if not match:
                continue
            target = match.group(1)
            if target.startswith("./"):
                continue
            if "@" not in target:
                findings.append(
                    Finding(
                        "ci-action-pinning",
                        f"{rel}:{line_number}",
                        f"external action/reusable workflow has no immutable ref: {target}",
                    )
                )
                continue
            _, ref = target.rsplit("@", 1)
            if not IMMUTABLE_SHA.fullmatch(ref):
                findings.append(
                    Finding(
                        "ci-action-pinning",
                        f"{rel}:{line_number}",
                        f"mutable external action/reusable-workflow ref: {target}",
                    )
                )
    return findings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--history",
        action="store_true",
        help="scan reachable git history for high-confidence secret signatures",
    )
    args = parser.parse_args()

    findings = []
    findings.extend(scan_current_tree())
    findings.extend(scan_action_pins())
    if args.history:
        findings.extend(scan_history())

    if findings:
        print("GoreeCloud Browser source-security audit: FAIL", file=sys.stderr)
        for finding in findings:
            print(f" - {finding.render()}", file=sys.stderr)
        return 1

    print("GoreeCloud Browser source-security audit: PASS")
    print("Controls checked: high-confidence current-tree secrets, immutable CI action refs"
          + (", high-confidence reachable-history secrets" if args.history else ""))
    print("Boundary: vulnerability scanning, SBOM, runtime security, production acceptance remain separate.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

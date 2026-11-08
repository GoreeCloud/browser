#!/usr/bin/env python3
"""Fail-closed audit of external GitHub Actions refs and workflow permissions.

The CI workflow is a privileged supply-chain entry point. This deliberately
small, standard-library check supports the explicit uses syntax used by
this repository. It rejects unreviewed actions, movable refs, and unexpected
workflow permission grants. It is not a complete YAML security analyzer.
"""
from __future__ import annotations

from pathlib import Path
import re
import sys

# Independently resolved from first-party GitHub refs on November 7, 2026.
# checkout v4: https://github.com/actions/checkout/commit/11d5960a326750d5838078e36cf38b85af677262
# setup-python v5.6.0: https://github.com/actions/setup-python/commit/a26af69be951a213d495a4c3e4e4022e16d87065
# Deliberate upgrades must review upstream changes, update the SHA and tests,
# then run the complete Linux CI acceptance suite on that exact commit.
APPROVED_ACTIONS = {
    "actions/checkout": "11d5960a326750d5838078e36cf38b85af677262",
    "actions/setup-python": "a26af69be951a213d495a4c3e4e4022e16d87065",
}

USES = re.compile(r"^\s*(?:-\s*)?uses\s*:\s*(.*?)\s*$")
SHA_REF = re.compile(r"^(?P<action>[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+)@(?P<sha>[a-f0-9]{40})$")
PERMISSIONS = re.compile(r"^(?P<indent>\s*)permissions\s*:\s*(?P<value>.*?)\s*$")
SETTING = re.compile(r"^\s*(?P<permission>[A-Za-z0-9_-]+)\s*:\s*(?P<value>.*?)\s*$")


class ActionPinError(ValueError):
    """A workflow requests an unreviewed action or unnecessary permission."""


def validate_workflow(text: str, name: str) -> set[str]:
    """Audit supported workflow syntax, refusing ambiguous declarations.

    Only scalar uses: owner/action@40hexsha is accepted. YAML aliases or
    multiline expressions cannot bypass the check because they fail syntax.
    """
    approved: set[str] = set()
    saw_top_permissions = False
    saw_top_contents_read = False
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if line.lstrip().startswith("#"):
            continue
        match = USES.match(line)
        if not match and re.search(r"\buses\s*:", line):
            raise ActionPinError(
                f"{name}:{index + 1}: unsupported uses syntax; use a reviewed full SHA"
            )
        if match:
            value = match.group(1).split(" #", 1)[0].strip().strip("\"'")
            ref = SHA_REF.fullmatch(value)
            if not ref:
                raise ActionPinError(
                    f"{name}:{index + 1}: external action must use a full immutable commit SHA"
                )
            action, sha = ref.group("action"), ref.group("sha")
            if APPROVED_ACTIONS.get(action) != sha:
                raise ActionPinError(
                    f"{name}:{index + 1}: unreviewed action or commit for {action}"
                )
            if action == "actions/checkout":
                # Current workflow style is deliberately explicit: the token
                # must not be kept after checkout, and an ambiguous or omitted
                # input cannot inherit the upstream default of true.
                indent = len(line) - len(line.lstrip())
                required = ("with:", "persist-credentials: false")
                for offset, expected in enumerate(required, start=1):
                    next_index = index + offset
                    if next_index >= len(lines):
                        raise ActionPinError(f"{name}: checkout must disable persisted credentials")
                    value_line = lines[next_index]
                    actual_indent = len(value_line) - len(value_line.lstrip())
                    if value_line.split(" #", 1)[0].strip() != expected or actual_indent != indent + offset * 2:
                        raise ActionPinError(f"{name}: checkout must disable persisted credentials")
                for later in lines[index + 3 :]:
                    if not later.strip() or later.lstrip().startswith("#"):
                        continue
                    later_indent = len(later) - len(later.lstrip())
                    if later_indent <= indent:
                        break
                    if re.match(r"^\s*persist-credentials\s*:", later):
                        raise ActionPinError(
                            f"{name}: duplicate checkout credential option rejected"
                        )
            approved.add(action)
        if match := PERMISSIONS.match(line):
            indent = len(match.group("indent"))
            if indent == 0:
                saw_top_permissions = True
            if match.group("value"):
                raise ActionPinError(
                    f"{name}:{index + 1}: permission shortcut or alias is prohibited"
                )
            entries = 0
            for next_line in lines[index + 1 :]:
                if not next_line.strip() or next_line.lstrip().startswith("#"):
                    continue
                child_indent = len(next_line) - len(next_line.lstrip())
                if child_indent <= indent:
                    break
                field = SETTING.fullmatch(next_line)
                if not field or child_indent != indent + 2:
                    raise ActionPinError(
                        f"{name}:{index + 1}: unexpected permission mapping syntax"
                    )
                value = field.group("value").split(" #", 1)[0].strip()
                if value not in ("read", "none"):
                    raise ActionPinError(
                        f"{name}: workflow permission must be read or none"
                    )
                if indent == 0 and field.group("permission") == "contents" and value == "read":
                    saw_top_contents_read = True
                entries += 1
            if entries == 0:
                raise ActionPinError(f"{name}: empty workflow permissions")
    if not saw_top_permissions:
        raise ActionPinError(f"{name}: explicit top-level least-privilege permissions required")
    if not saw_top_contents_read:
        raise ActionPinError(f"{name}: top-level contents: read permission required")
    if not approved:
        raise ActionPinError(f"{name}: expected reviewed actions not present")
    return approved


def validate_directory(workflows: Path) -> tuple[int, set[str]]:
    files = sorted([
        p for p in workflows.iterdir()
        if p.is_file() and p.suffix.lower() in (".yml", ".yaml")
    ])
    if not files:
        raise ActionPinError("No GitHub workflow files found")
    reviewed: set[str] = set()
    for file in files:
        if file.is_symlink():
            raise ActionPinError(f"Symlink workflow refused: {file.name}")
        reviewed.update(validate_workflow(file.read_text("utf-8"), file.name))
    missing = set(APPROVED_ACTIONS) - reviewed
    if missing:
        raise ActionPinError(f"Missing mandatory reviewed actions: {sorted(missing)}")
    return len(files), reviewed


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    try:
        files, actions = validate_directory(root / ".github" / "workflows")
    except (ActionPinError, OSError, UnicodeError) as exc:
        print(f"ACTION_PIN_POLICY_FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"ACTION_PIN_POLICY_PASS workflows={files} reviewed_actions={len(actions)}")
    print("Scope: immutable reviewed refs and read-only declared permissions only; "
          "not an upstream code audit or a guarantee of CI isolation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

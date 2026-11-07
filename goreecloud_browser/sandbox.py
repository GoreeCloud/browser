"""Preflight policy for Linux Qt WebEngine sandbox and web trust settings.

Qt WebEngine documents the Linux renderer sandbox as a layered defense using
namespaces and seccomp. This guard refuses known *explicit* sandbox-disabling
settings before BrowserWindow creates any WebEngine profiles. It does NOT
prove the kernel accepted each layer; that requires a separate process audit.

No flags are removed, modified or silently ignored. The only remediation is
to remove the unsafe setting and restart the browser.
"""
from __future__ import annotations

import shlex
from collections.abc import Mapping, Sequence

# Qt 6 documents --no-sandbox and --single-process as disabling renderer/
# plugin isolation. Other known Chromium switches disable individual layers.
UNSAFE_SWITCHES = frozenset({
    "--no-sandbox",
    "--single-process",
    "--disable-seccomp-filter-sandbox",
    "--disable-namespace-sandbox",
    "--disable-gpu-sandbox",
    "--disable-setuid-sandbox",
    "--in-process-gpu",
    "--disable-site-isolation-trials",
    # Chromium flags which bypass TLS trust, mixed-content, same-origin or
    # secure-context policy. Never enable them in a general-purpose browser.
    "--ignore-certificate-errors",
    "--ignore-certificate-errors-spki-list",
    "--allow-insecure-localhost",
    "--allow-running-insecure-content",
    "--disable-web-security",
    "--unsafely-treat-insecure-origin-as-secure",
})

UNSAFE_ENABLED_FEATURES = frozenset({
    "networkserviceinprocess",  # Qt docs: loses network-service isolation
})

ENV_FLAGS = "QTWEBENGINE_CHROMIUM_FLAGS"
ENV_DISABLE = "QTWEBENGINE_DISABLE_SANDBOX"


class WebEngineSecurityError(RuntimeError):
    """Explicit unsafe WebEngine security configuration; refuse to launch."""


def unsafe_webengine_settings(
    environment: Mapping[str, str],
    arguments: Sequence[str],
) -> tuple[str, ...]:
    """Return names of known unsafe settings, without exposing arbitrary args."""
    found: set[str] = set()

    # Qt's disable switch is a *presence* risk. Even an empty value or "0"
    # is not trusted, as handling may vary across Qt builds/versions.
    if ENV_DISABLE in environment:
        found.add(ENV_DISABLE)

    raw_flags = environment.get(ENV_FLAGS, "")
    try:
        parsed_flags = shlex.split(raw_flags, posix=True)
    except ValueError:
        found.add("malformed " + ENV_FLAGS)
        parsed_flags = []

    for item in list(parsed_flags) + list(arguments):
        if not isinstance(item, str) or not item.startswith("--"):
            continue
        name, _, value = item.partition("=")
        name = name.lower()
        if name in UNSAFE_SWITCHES:
            found.add(name)
        if name == "--enable-features":
            enabled = {feature.strip().split("<", 1)[0].lower()
                       for feature in value.split(",")}
            if enabled & UNSAFE_ENABLED_FEATURES:
                found.add("--enable-features=NetworkServiceInProcess")

    return tuple(sorted(found))


def require_safe_webengine_settings(
    environment: Mapping[str, str],
    arguments: Sequence[str],
) -> None:
    """Refuse explicit security bypasses, never silently strip user flags."""
    found = unsafe_webengine_settings(environment, arguments)
    if found:
        raise WebEngineSecurityError(
            "GoreeCloud Browser refuses to start because Qt WebEngine was "
            "configured with settings that weaken browser sandbox, TLS, or "
            "web-origin protections: "
            + ", ".join(found)
            + ". Remove the setting from the environment, desktop launcher "
              "or command line, then restart. This check does not certify "
              "renderer sandbox availability on the operating system."
        )

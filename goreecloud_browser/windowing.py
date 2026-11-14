"""Pure startup geometry policy for the native GoreeCloud Browser window.

Qt supplies the current screen's *available* work area, excluding panels/docks.
The default is a normal, resizable window rather than an immersive session.
"""

from __future__ import annotations

import re
from collections.abc import Iterable


DEFAULT_WINDOW_SIZE = (1250, 790)
MIN_WINDOW_SIZE = (700, 480)


def restrictive_work_area(
    areas: Iterable[tuple[int, int]],
) -> tuple[int, int] | None:
    """Choose a startup size safe across all currently detected displays.

    Before a first window is shown, an OS window manager can place it on a
    non-primary monitor. Use the smallest valid logical work-area width and
    height among active displays, independent of enumeration order. Invalid
    geometry is ignored rather than letting it force a zero-size window.

    This returns only dimensions: no monitor names, coordinates or identity
    are read, stored or sent anywhere.
    """
    valid = [
        (width, height) for width, height in areas
        if type(width) is int and type(height) is int
        and width > 0 and height > 0
    ]
    if not valid:
        return None
    return min(w for w, _ in valid), min(h for _, h in valid)


def initial_window_size(available_width: int, available_height: int) -> tuple[int, int]:
    """Fit a first-launch window within 90% of a usable screen, if possible.

    The documented 700x480 Qt layout minimum takes precedence on tiny screens.
    Invalid/unavailable screen geometry falls back to the existing default.
    No previously saved window state is read or overwritten here.
    """
    if available_width <= 0 or available_height <= 0:
        return DEFAULT_WINDOW_SIZE

    return (
        max(MIN_WINDOW_SIZE[0], min(DEFAULT_WINDOW_SIZE[0], available_width * 9 // 10)),
        max(MIN_WINDOW_SIZE[1], min(DEFAULT_WINDOW_SIZE[1], available_height * 9 // 10)),
    )


def restored_window_size(
    saved: object, available_width: int, available_height: int,
) -> tuple[int, int]:
    """Use a user-opted-in normal window size; refuse malformed saved data.

    Only a bounded width/height pair is stored, never screen coordinates,
    desktop identifiers, maximize/full-screen state, or website/profile data.
    On display changes clamp to 90% of the available work area; the native
    700x480 content minimum still takes precedence on very small displays.
    """
    fallback = initial_window_size(available_width, available_height)
    if available_width <= 0 or available_height <= 0:
        return fallback
    if not isinstance(saved, str) or len(saved) > 11:
        return fallback
    match = re.fullmatch(r"([0-9]{3,4})x([0-9]{3,4})", saved)
    if match is None:
        return fallback
    width, height = map(int, match.groups())
    if width < MIN_WINDOW_SIZE[0] or height < MIN_WINDOW_SIZE[1]:
        return fallback
    return (
        max(MIN_WINDOW_SIZE[0], min(width, available_width * 9 // 10)),
        max(MIN_WINDOW_SIZE[1], min(height, available_height * 9 // 10)),
    )

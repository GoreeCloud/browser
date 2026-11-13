"""Pure startup geometry policy for the native GoreeCloud Browser window.

Qt supplies the current screen's *available* work area, excluding panels/docks.
The default is a normal, resizable window rather than an immersive session.
"""

from __future__ import annotations


DEFAULT_WINDOW_SIZE = (1250, 790)
MIN_WINDOW_SIZE = (700, 480)


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

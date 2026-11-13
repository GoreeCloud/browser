"""Pure first-launch desktop window geometry regression tests."""

import unittest

from goreecloud_browser.windowing import (
    DEFAULT_WINDOW_SIZE, MIN_WINDOW_SIZE, initial_window_size,
)


class InitialWindowSizeTests(unittest.TestCase):
    def test_large_desktop_uses_comfortable_nonmaximized_size(self):
        self.assertEqual(initial_window_size(3840, 2160), DEFAULT_WINDOW_SIZE)

    def test_typical_laptop_leaves_room_for_other_windows(self):
        self.assertEqual(initial_window_size(1366, 768), (1229, 691))

    def test_small_usable_work_area_shrinks_window(self):
        self.assertEqual(initial_window_size(800, 600), (720, 540))

    def test_dock_or_panel_reduces_available_height(self):
        self.assertEqual(initial_window_size(1920, 650), (1250, 585))

    def test_minimum_layout_size_is_respected_on_tiny_screens(self):
        self.assertEqual(initial_window_size(640, 360), MIN_WINDOW_SIZE)

    def test_absent_or_invalid_work_area_uses_previous_default(self):
        for size in ((0, 0), (-1, 800), (800, -1)):
            with self.subTest(size=size):
                self.assertEqual(initial_window_size(*size), DEFAULT_WINDOW_SIZE)


if __name__ == '__main__':
    unittest.main()

"""Pure first-launch desktop window geometry regression tests."""

import unittest

from goreecloud_browser.windowing import (
    DEFAULT_WINDOW_SIZE, MIN_WINDOW_SIZE, initial_window_size, restored_window_size,
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


class RestoredWindowSizeTests(unittest.TestCase):
    def test_explicit_saved_normal_size_restores_when_it_fits(self):
        self.assertEqual(restored_window_size("1024x680", 1920, 1080), (1024, 680))

    def test_smaller_display_clamps_previous_normal_size_without_offscreen_position(self):
        self.assertEqual(restored_window_size("1900x900", 1280, 800), (1152, 720))

    def test_tiny_screen_preserves_existing_native_minimum(self):
        self.assertEqual(restored_window_size("1100x760", 640, 360), MIN_WINDOW_SIZE)

    def test_invalid_saved_state_reverts_to_windowed_default(self):
        invalid = (
            "", None, 1200, [], {}, "0x0", "699x480", "700x479",
            "9000x99999", "700x480junk", "700,480", "-800x600",
            " 800x600", "８００x６００", "a" * 20000,
        )
        for value in invalid:
            with self.subTest(value=str(value)[:30]):
                self.assertEqual(
                    restored_window_size(value, 1366, 768),
                    initial_window_size(1366, 768),
                )

    def test_unavailable_display_geometry_uses_default(self):
        self.assertEqual(restored_window_size("1100x700", 0, 0), DEFAULT_WINDOW_SIZE)


if __name__ == '__main__':
    unittest.main()

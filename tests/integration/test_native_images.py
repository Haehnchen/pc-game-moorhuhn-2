#!/usr/bin/env python3
"""Check the pixel oracle's scaling, exclusion masks and mismatch detection."""
import unittest

from round_native import RgbImage, scaled_screen, screen_difference


class NativeImageTests(unittest.TestCase):
    def test_integer_scaling_preserves_pixels_and_black_borders(self):
        source = RgbImage((2, 1), bytes((255, 0, 0, 0, 128, 255)))
        scaled = scaled_screen(source, (6, 4), 2, 1, 1)
        for y in range(4):
            for x in range(6):
                expected = (0, 0, 0)
                if 1 <= x < 5 and 1 <= y < 3:
                    expected = (255, 0, 0) if x < 3 else (0, 128, 255)
                self.assertEqual(scaled.pixel((x, y)), expected)

    def test_mask_does_not_hide_adjacent_mismatches(self):
        expected = RgbImage((3, 2), bytes(18))
        actual = RgbImage((3, 2), bytes((1, 2, 3, 4, 5, 6)) + bytes(12))
        difference = screen_difference(actual, expected, ((0, 0, 1, 1),), None)
        self.assertEqual(difference.pixel((0, 0)), (0, 0, 0))
        self.assertEqual(difference.pixel((1, 0)), (4, 5, 6))
        self.assertTrue(any(difference.pixels))
        matched = screen_difference(actual, expected, ((0, 0, 2, 1),), None)
        self.assertFalse(any(matched.pixels))

    def test_region_keeps_only_its_mismatches(self):
        expected = RgbImage((3, 2), bytes(18))
        actual = RgbImage((3, 2), bytes((10, 20, 30)) + bytes(9) + bytes((40, 50, 60)) + bytes(3))
        difference = screen_difference(actual, expected, (), (1, 1, 3, 2))
        self.assertEqual(difference, RgbImage((2, 1), bytes((40, 50, 60, 0, 0, 0))))
        self.assertFalse(any(screen_difference(actual, expected, (), (1, 0, 3, 1)).pixels))


if __name__ == "__main__":
    unittest.main(verbosity=2)

"""Reviewed source-margin restoration preserves alpha and generated artwork."""
import unittest
from PIL import Image, ImageChops

from match_brightness import source_straight_rgb, restore_source_regions


class SourceRegionTests(unittest.TestCase):
    def test_premultiplied_source_restores_every_representable_byte(self):
        # Each row has one alpha value; exercise every possible stored RGB <= alpha.
        source = Image.new('RGBA', (256, 256))
        for alpha in range(1, 256):
            for value in range(alpha + 1):
                source.putpixel((value, alpha), (value, value, value, alpha))
        rgb = source_straight_rgb(source, source.size, True)
        restored = ImageChops.multiply(rgb.getchannel('R'), source.getchannel('A'))
        self.assertEqual(restored.tobytes(), source.getchannel('R').tobytes())

    def test_bounded_restore_keeps_generated_pixels_and_alpha(self):
        source = Image.new('RGBA', (4, 4), (2, 1, 3, 128))
        rgb = source_straight_rgb(source, (8, 8), True)
        generated = Image.new('RGBA', (8, 8), (200, 100, 50, 7))
        result, count = restore_source_regions(generated, rgb, [[0, 2, 4, 4]], (0, 0, 8, 8))
        self.assertEqual(count, 8)
        self.assertEqual(result.getpixel((0, 2)), (4, 2, 6, 7))
        self.assertEqual(result.getpixel((4, 2)), (200, 100, 50, 7))
        self.assertEqual(result.getchannel('A').tobytes(), generated.getchannel('A').tobytes())


if __name__ == '__main__':
    unittest.main()

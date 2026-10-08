"""An alpha-test source may intentionally keep opaque black unused UV texels."""
import contextlib
import io
import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch
from PIL import Image
import match_brightness


class BlackUVTest(unittest.TestCase):
    def calibrate(self, padding, full_raw_alpha):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            original = Image.new('RGBA', (4, 4), (0, 0, 0, 255))
            original.paste((100, 100, 100, 255), (1, 1, 3, 3))
            original.putpixel((0, 0), (0, 0, 0, 0))
            original.save(root / 'original.png')
            raw = Image.new('RGBA', (16, 16), (0, 0, 0, 255 if full_raw_alpha else 0))
            raw.paste((150, 150, 150, 255), (4, 4, 12, 12))
            raw.save(root / 'raw.png')
            asset = {'id': 100001, 'uncalibrated_png': 'raw.png', 'reference_png': 'original.png',
                     'native_alpha_reference': 'original.png', 'calibration_grid': [1, 1]}
            if padding is False:
                asset.update(source_mask_rgb_padding=False,
                             source_mask_rgb_padding_reason='Preserve reviewed opaque black UV gaps in alpha-test source.')
            (root / 'sources.json').write_text(json.dumps({'textures': [asset]}))
            with patch.object(match_brightness, 'ROOT', root), contextlib.redirect_stdout(io.StringIO()):
                match_brightness.main({100001})
            result = Image.open(root / 'raw-balanced.png').convert('RGBA')
            self.assertEqual(result.getchannel('A').tobytes(), raw.getchannel('A').tobytes())
            calibration = json.loads((root / 'sources.json').read_text())['textures'][0]['brightness_calibration']
            self.assertLess(abs(calibration['corrected_luma'] - calibration['original_luma']), .1)
            return result, calibration

    def test_cutout_rgb_does_not_fill_alpha_positive_black_uv(self):
        result, calibration = self.calibrate(False, False)
        self.assertEqual(result.getpixel((0, 8))[:3], (0, 0, 0))
        self.assertEqual(result.getpixel((8, 8))[:3], (100, 100, 100))
        self.assertEqual(calibration['cells'][0]['rgb_padding_texels'], 0)
        self.assertFalse(calibration['source_mask_rgb_padding'])

    def test_default_is_unchanged_when_generated_canvas_is_opaque(self):
        default, _ = self.calibrate(None, True)
        explicit, _ = self.calibrate(False, True)
        self.assertEqual(default.tobytes(), explicit.tobytes())


if __name__ == '__main__':
    unittest.main()

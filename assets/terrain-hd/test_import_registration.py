"""Check reviewed atlas-island registration without changing production metadata."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from PIL import Image
import import_generated


class RegistrationTests(unittest.TestCase):
    def test_islands_restore_logical_uv_and_keep_untouched_raw(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'expanded/original').mkdir(parents=True)
            original = Image.new('RGBA', (8, 8), (0, 0, 0, 255))
            original.paste((80, 0, 0, 255), (0, 0, 4, 4))
            original.paste((0, 80, 0, 255), (0, 6, 8, 8))
            original.save(root / 'expanded/original/1.png')
            source = Image.new('RGBA', (64, 64), (0, 0, 0, 255))
            source.paste((160, 0, 0, 255), (4, 8, 28, 32))
            source.paste((0, 0, 160, 255), (4, 8, 16, 32))
            source.paste((0, 160, 0, 255), (0, 52, 64, 64))
            source.save(root / 'raw.png')
            raw_hash = hashlib.sha256((root / 'raw.png').read_bytes()).hexdigest()
            item = {'id': 1, 'category': 'buildings-environment', 'role': 'model-diffuse',
                    'texture': {'Type': 'Ordinary', 'UserName': 'fixture', 'SrcName': 'fixture'},
                    'logical_size': [8, 8], 'alpha_extrema': [255, 255],
                    'source_rgba_sha256': hashlib.sha256(original.tobytes()).hexdigest(),
                    'original_png': 'expanded/original/1.png', 'status': 'pending'}
            (root / 'sources.json').write_text(json.dumps({'textures': []}))
            (root / 'expanded/queue.json').write_text(json.dumps([item]))
            job = {'id': 1, 'generated': str(root / 'raw.png'), 'prompt': 'fixture',
                   'uv_regions': [{'source_bbox': [0, 0, 4, 4], 'generated_bbox': [4, 8, 28, 32], 'rotate': 180},
                                  {'source_bbox': [0, 6, 8, 8], 'generated_bbox': [0, 52, 64, 64]}],
                   'uv_registration_reason': 'Fixture with displaced islands'}
            previous_root = import_generated.ROOT
            try:
                import_generated.ROOT = root
                import_generated.import_jobs([job])
            finally:
                import_generated.ROOT = previous_root
            result = Image.open(root / 'expanded/generated/1.png')
            self.assertEqual(result.size, (32, 32))
            self.assertEqual(result.getpixel((0, 0)), (160, 0, 0, 255))
            self.assertEqual(result.getpixel((15, 15)), (0, 0, 160, 255))
            self.assertEqual(result.getpixel((16, 15)), (0, 0, 0, 255))
            self.assertEqual(result.getpixel((31, 23)), (0, 0, 0, 255))
            self.assertEqual(result.getpixel((31, 24)), (0, 160, 0, 255))
            self.assertEqual(hashlib.sha256((root / 'expanded/generated/1-raw.png').read_bytes()).hexdigest(), raw_hash)
            asset = json.loads((root / 'sources.json').read_text())['textures'][0]
            self.assertEqual(asset['uv_registration']['regions'], job['uv_regions'])


if __name__ == '__main__':
    unittest.main()

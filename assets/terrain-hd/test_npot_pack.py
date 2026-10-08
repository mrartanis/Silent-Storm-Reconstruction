"""Actual NPOT archives must preserve whole UVs, source alpha and partial mips."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
from PIL import Image, ImageChops, ImageStat
import build_pack
import validate_pack


class NPOTPackTest(unittest.TestCase):
    def test_native_odd_mips_and_single_premultiply(self):
        source = Image.new('RGBA', (107, 39), (80, 120, 160, 128))
        source.putpixel((0, 0), (30, 60, 90, 0))
        source.putpixel((106, 38), (90, 60, 30, 255))
        original = source.tobytes()
        physical = source.resize((428, 156), Image.Resampling.NEAREST)
        data = build_pack.mmp(physical, premultiply=True, alpha_reference=source)
        signature, fmt, average, width, height, levels = struct.unpack_from('<6I', data)
        self.assertEqual((signature, fmt, width, height, levels), (0x504d4d, 6, 428, 156, 8))
        rgba = physical.copy()
        rgba.putalpha(source.getchannel('A').resize(physical.size, Image.Resampling.LANCZOS))
        r, g, b, a = rgba.split()
        expected = Image.merge('RGBA', (ImageChops.multiply(r, a), ImageChops.multiply(g, a),
                                       ImageChops.multiply(b, a), a))
        offset = 24
        observed_sizes = []
        for level in range(levels):
            w, h = width >> level, height >> level
            observed_sizes.append((w, h))
            actual = Image.frombytes('RGBA', (w, h), data[offset:offset + w * h * 4], 'raw', 'BGRA')
            self.assertEqual(actual.tobytes(), expected.tobytes())
            offset += w * h * 4
            if level + 1 < levels:
                expected = Image.merge('RGBA', tuple(c.resize((w // 2, h // 2), Image.Resampling.BOX)
                                                    for c in expected.split()))
        self.assertEqual(observed_sizes[-2:], [(6, 2), (3, 1)])
        self.assertEqual(offset, len(data), 'Every mip byte is consumed; neither dimension reaches zero.')
        self.assertEqual(source.tobytes(), original)

    def test_build_and_validate_real_archives_without_padding(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            assets = []
            # Wide 2D cache overflow, NPOT square, and existing rectangular POT.
            for identifier, logical in enumerate(((515, 130), (27, 27), (32, 64)), start=100001):
                size = tuple(n * 4 for n in logical)
                image = Image.new('RGBA', size, (40, 80, 120, 255))
                for xy, color in (((0, 0), (10, 20, 30, 255)),
                                  ((size[0] - 1, 0), (200, 30, 80, 255)),
                                  ((0, size[1] - 1), (60, 90, 220, 255)),
                                  ((size[0] - 1, size[1] - 1), (170, 140, 10, 255))):
                    image.putpixel(xy, color)
                filename = f'{identifier}.png'
                image.save(root / filename)
                means = ImageStat.Stat(image.convert('RGB')).mean
                assets.append({'id': identifier, 'png': filename, 'logical_size': list(logical),
                               'brightness_calibration': {'original_luma':
                                   sum(w * m for w, m in zip((.2126, .7152, .0722), means))}})
            (root / 'sources.json').write_text(json.dumps({'textures': assets}), encoding='utf-8')
            local_builder = root / 'build_pack.py'
            local_builder.write_bytes(Path(build_pack.__file__).read_bytes())
            pack = root / 'res-hd'
            with patch.object(build_pack, 'ROOT', root), patch.object(validate_pack, 'ROOT', root), \
                    patch.object(build_pack, '__file__', str(local_builder)), \
                    contextlib.redirect_stdout(io.StringIO()):
                build_pack.build(pack)
                validate_pack.validate(pack)
            manifest = json.loads((pack / 'manifest.json').read_text())
            reports = json.loads((pack / 'validation.json').read_text())
            self.assertEqual(len(reports), 3)
            self.assertEqual(manifest['archives'][0]['keys'], 6)
            self.assertEqual([a['physical_size'] for a in manifest['textures']],
                             [[2060, 520], [108, 108], [128, 256]])
            self.assertEqual([a['mips'] for a in manifest['textures']], [10, 7, 8])
            for asset in manifest['textures']:
                image = Image.open(root / asset['png'])
                self.assertEqual(asset['mmp_sha256'], hashlib.sha256(build_pack.mmp(image)).hexdigest())


if __name__ == '__main__':
    unittest.main()

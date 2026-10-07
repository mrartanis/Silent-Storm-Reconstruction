"""Native RGB565 and read-only archive lookup regression checks."""
import struct
import tempfile
import unittest
from pathlib import Path

from build_pack import chunk
from prepare_sources import decode_mmp, ReleaseResources


class NativeSourceTests(unittest.TestCase):
    def test_rgb565_channel_order_opaque_alpha_and_top_mip(self):
        data = struct.pack('<6I', 0x504d4d, 8, 0, 4, 1, 2)
        data += struct.pack('<4H', 0xF800, 0x07E0, 0x001F, 0xFFFF) + b'ignored lower mip'
        image = decode_mmp(data)
        self.assertEqual(image.tobytes(), bytes([255, 0, 0, 255, 0, 255, 0, 255,
                                               0, 0, 255, 255, 255, 255, 255, 255]))

    def test_archive_index_and_loose_override_are_read_only(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            payload = b'packed original'
            index_offset = 8 + len(payload)
            pair = chunk(1, struct.pack('<i', 5076)) + chunk(2, struct.pack('<II', 8, len(payload)))
            data = struct.pack('<II', 0x96948a22, index_offset) + payload + chunk(1, chunk(1, pair))
            archive = root / 'Textures.res'
            archive.write_bytes(data)
            resources = ReleaseResources(root)
            self.assertEqual(resources.read(5076), payload)
            (root / 'Textures').mkdir()
            (root / 'Textures/5076').write_bytes(b'loose original')
            self.assertEqual(resources.read(5076), b'loose original')
            self.assertEqual(archive.read_bytes(), data)

    def test_unsupported_format_is_explicit(self):
        with self.assertRaisesRegex(ValueError, 'Unsupported native MMP format'):
            decode_mmp(struct.pack('<6I', 0x504d4d, 99, 0, 1, 1, 1))


if __name__ == '__main__':
    unittest.main()

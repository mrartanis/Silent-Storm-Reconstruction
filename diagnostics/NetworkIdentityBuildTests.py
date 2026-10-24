import importlib.util
from pathlib import Path
import tempfile
import unittest
import subprocess
import sys

spec = importlib.util.spec_from_file_location('identity', Path(__file__).with_name('Generate-NetworkIdentity.py'))
identity = importlib.util.module_from_spec(spec)
spec.loader.exec_module(identity)


class BuildIdentityTests(unittest.TestCase):
    def test_engine_release_build_id_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'game.db').write_bytes(b'player data')
            (root / 'network-build-id.txt').write_text('a' * 64 + '\n', encoding='ascii')
            output = root / 'network-version.txt'
            subprocess.run([sys.executable, str(Path(__file__).with_name('Generate-NetworkIdentity.py')),
                            '--game-data', str(root), '--build-id-file', str(root / 'network-build-id.txt'),
                            '--output', str(output)], check=True)
            self.assertEqual(output.read_text().splitlines()[:2], ['S2-NETWORK-1', 'a' * 64])

    def test_gameplay_changes_and_visual_overlay(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in ('res', 'scripts', 'res-hd'):
                (root / name).mkdir()
            (root / 'game.db').write_bytes(b'database')
            (root / 'res/Units.res').write_bytes(b'units')
            (root / 'scripts/Common.lua').write_bytes(b'rules')
            output = root / 'network-version.txt'
            build = 'a' * 64
            first = identity.generate(root, build, output, True)
            self.assertEqual(first, identity.generate(root, build, output, True))
            (root / 'res-hd/Textures.res').write_bytes(b'optional visuals')
            self.assertEqual(first, identity.generate(root, build, output, True))
            (root / 'res/Units.res').write_bytes(b'changed units')
            second = identity.generate(root, build, output, True)
            self.assertNotEqual(first, second)
            self.assertEqual(second, identity.generate(root, 'b' * 64, output, True))
            self.assertEqual(output.read_text().splitlines()[1], 'b' * 64)
            (root / 'scripts/Common.lua').write_bytes(b'new rules')
            self.assertNotEqual(second, identity.generate(root, build, output, True))


if __name__ == '__main__':
    unittest.main()

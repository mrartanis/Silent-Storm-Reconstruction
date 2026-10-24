"""Exercise the release boundary: allowlist, hashes and complete runtime chain."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import zipfile

spec = importlib.util.spec_from_file_location('engine_package', Path(__file__).with_name('Package-Engine.py'))
engine = importlib.util.module_from_spec(spec)
spec.loader.exec_module(engine)


class EnginePackageTests(unittest.TestCase):
    def fixture(self, root):
        source, build, deps, crt = (root / n for n in ('source', 'build', 'deps', 'crt'))
        for directory in (source, build, deps, crt):
            directory.mkdir()

        def write(path, content=b'fixture'):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)

        binary = bytearray(128)
        struct.pack_into('<I', binary, 60, 64)
        binary[68:70] = b'\x64\x86'
        for name in ('Game.exe', 'SDL3.dll', 'zlib.dll', 'binkw32.dll'):
            write(build / name, binary)
        for name in ('game.db', 'res/Textures.res', 'res-hd/Textures.res', 'save/private.sav'):
            write(build / name, b'never distribute')
        manifest = b'S2-NETWORK-1\n1\nsilent-storm-1.2\n'
        write(build / 'network-version.txt', manifest)
        write(source / 'network-version.txt', manifest)
        for name in ('msvcp140.dll', 'vcruntime140.dll'):
            write(crt / name, binary)
        for name in ('avcodec-62.dll', 'avformat-62.dll', 'avutil-60.dll', 'swscale-9.dll', 'swresample-6.dll'):
            write(deps / 'install/bin' / name, binary)
        for module in ('SDL3-3.4.16', 'bgfx.cmake', 'bgfx', 'bimg', 'bx', 'ffmpeg-8.1'):
            write(deps / module / 'LICENSE')
        for name in ('miniaudio.h', 'sources.json', 'ffmpeg-8.1.tar.xz', 'ffmpeg-config.log'):
            write(deps / name)
        for name in ('assets/fonts/LICENSE', 'assets/fonts/test.ttf', 'LICENSE.md',
                     'packaging/README-engine.txt', 'packaging/Play.cmd',
                     '.github/scripts/build-deps.sh',
                     'third_party/squish/LICENSE.txt', 'third_party/zlib/zlib.h',
                     'third_party/libpng/png.h', 'Script/lua.h'):
            write(source / name)
        args = argparse.Namespace(build=build, deps=deps, crt_runtime=crt,
                                  output=root / 'dist', version='v0.1.0-rc1', platform='windows')
        return source, args

    def test_engine_allowlist_and_integrity(self):
        with tempfile.TemporaryDirectory() as temporary:
            source, args = self.fixture(Path(temporary))
            with patch.object(engine, 'ROOT', source), patch.object(engine, 'pe_imports', return_value=['avcodec-62.dll']), \
                 patch.object(engine.subprocess, 'check_output', return_value='a' * 40):
                engine.package(args)
            archive = next(args.output.glob('*.zip'))
            with zipfile.ZipFile(archive) as package:
                names = package.namelist()
                self.assertFalse(any('/res/' in p or '/res-hd/' in p or '/save/' in p or p.endswith('/game.db') for p in names))
                info_path = next(p for p in names if p.endswith('/engine-build.json'))
                info = json.loads(package.read(info_path))
                self.assertFalse(info['gameData'])
                self.assertFalse(info['hdTextures'])
                for relative, expected in info['sha256'].items():
                    self.assertEqual(hashlib.sha256(package.read(names[0].split('/')[0] + '/' + relative)).hexdigest(), expected)
            self.assertEqual(archive.with_name(archive.name + '.sha256').read_text().split()[0], engine.sha256(archive))

    def test_missing_transitive_dll_blocks_release(self):
        with tempfile.TemporaryDirectory() as temporary:
            source, args = self.fixture(Path(temporary))
            with patch.object(engine, 'ROOT', source), patch.object(engine, 'pe_imports', return_value=['unpackaged-codec.dll']):
                with self.assertRaisesRegex(ValueError, 'Unpackaged runtime dependency'):
                    engine.package(args)
            self.assertEqual(list(args.output.glob('*.zip')), [])

    def test_stale_network_version_blocks_release(self):
        with tempfile.TemporaryDirectory() as temporary:
            source, args = self.fixture(Path(temporary))
            (args.build / 'network-version.txt').write_text('S2-NETWORK-1\nold\nsilent-storm-1.2\n')
            with patch.object(engine, 'ROOT', source):
                with self.assertRaisesRegex(ValueError, 'different network compatibility version'):
                    engine.package(args)


if __name__ == '__main__':
    unittest.main()

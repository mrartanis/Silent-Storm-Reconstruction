"""Create a standalone Windows game ZIP or solid 7z, including HD by default.

The build must already be complete. Runtime DLLs come from the build output
and the supplied x64 Microsoft.VC143.CRT directory, never from the old game.
"""
import argparse
import importlib.util
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import os
import shutil
import subprocess
import zipfile


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def package(args):
    output = args.output.resolve()
    if output.suffix.lower() not in ('.zip', '.7z'):
        raise ValueError('Output must end in .zip or .7z')
    if output.exists():
        raise FileExistsError(output)
    files = {}

    def add(path, relative):
        if not path.is_file():
            raise FileNotFoundError(path)
        if path.is_symlink() or path.is_junction():
            raise ValueError(f'Package inputs must be regular files: {path}')
        files[relative] = path

    def tree(directory, relative):
        if not directory.is_dir():
            raise FileNotFoundError(directory)
        for path in sorted(directory.rglob('*')):
            if path.is_symlink() or path.is_junction():
                raise ValueError(f'Package inputs must not contain links: {path}')
            if path.is_file():
                add(path, relative + '/' + path.relative_to(directory).as_posix())

    for name in ('game.db', 'start.cfg', 'tree.mma'):
        add(args.game_data / name, name)
    for name in ('cfg', 'Docs', 'res', 'scripts'):
        tree(args.game_data / name, name)
    add(args.build / 'Game.exe', 'Game.exe')
    identity = args.build / 'network-version.txt'
    build_id = identity.read_text(encoding='ascii').splitlines()[1]
    spec = importlib.util.spec_from_file_location('network_identity', Path(__file__).with_name('Generate-NetworkIdentity.py'))
    generator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(generator)
    # Fingerprint the actual packaging inputs, including modified gameplay data.
    generator.generate(args.game_data.resolve(), build_id, identity)
    add(identity, 'network-version.txt')
    tree(args.build / 'fonts', 'fonts')
    for path in sorted(args.build.glob('*.dll')):
        add(path, path.name)
    for path in sorted(args.crt_runtime.glob('*.dll')):
        add(path, path.name)
    for name in ('SDL3.dll', 'zlib.dll', 'binkw32.dll', 'msvcp140.dll', 'vcruntime140.dll'):
        if name not in files:
            raise FileNotFoundError(f'Missing runtime DLL: {name}')
    for name, path in files.items():
        if name.endswith(('.exe', '.dll')):
            with path.open('rb') as stream:
                stream.seek(0x3c)
                stream.seek(struct.unpack('<I', stream.read(4))[0] + 4)
                if stream.read(2) != b'\x64\x86':
                    raise ValueError(f'Expected Windows x64 binary: {path}')

    hd_count = 0
    if not args.no_hd_textures:
        hd_root = args.build / 'res-hd'
        manifest_path = hd_root / 'manifest.json'
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
        if not manifest['textures'] or not manifest['archives']:
            raise ValueError('HD texture pack is empty')
        hd_count = len(manifest['textures'])
        add(manifest_path, 'res-hd/manifest.json')
        for part in manifest['archives']:
            name = part['file']
            if Path(name).name != name or not name.endswith('.res'):
                raise ValueError(f'Invalid HD archive name: {name}')
            path = hd_root / name
            if path.stat().st_size != part['bytes'] or sha256(path) != part['sha256']:
                raise ValueError(f'HD archive integrity check failed: {path}')
            add(path, 'res-hd/' + name)
        print(f'HD verified: {hd_count} textures, {len(manifest["archives"])} archives', flush=True)

    info = {
        'createdUtc': datetime.now(timezone.utc).isoformat(), 'architecture': 'x64',
        'hdTextures': not args.no_hd_textures, 'hdTextureCount': hd_count,
        'networkBuild': build_id, 'networkData': identity.read_text(encoding='ascii').splitlines()[2],
        'compression': ({'format':'7z','method':'LZMA2','level':9,'dictionaryMiB':args.dictionary_mib,'fastBytes':273,'solid':True,'threads':2}
                        if output.suffix.lower()=='.7z' else {'format':'zip','method':'deflate','level':4}),
        'sha256': {name: sha256(path) for name, path in files.items()
                   if name.endswith(('.exe', '.dll')) or name in ('game.db', 'res-hd/manifest.json')},
    }
    readme = Path(__file__).with_name('NETWORK_TEST_RU.txt').read_text(encoding='utf-8-sig')
    if hd_count:
        readme += f'\nHD-ТЕКСТУРЫ\nВключены {hd_count} HD-текстур. По умолчанию они включены.\nПереключатель: Настройки → Видео → HD текстуры.\n'
    else:
        readme += '\nЭта сборка упакована без HD-текстур.\n'
    output.parent.mkdir(parents=True, exist_ok=True)
    root = output.stem
    print(f'Packaging {len(files)} files ({sum(p.stat().st_size for p in files.values())} bytes)', flush=True)
    extra = {
        'README-RU.txt': '\ufeff' + readme,
        'BUILD-INFO.json': json.dumps(info, ensure_ascii=False, indent=2),
        'Play.cmd': '@echo off\r\ncd /d "%~dp0"\r\nset "S2_USER_DATA_DIR=%~dp0user-data"\r\nstart "" /D "%~dp0" "%~dp0Game.exe"\r\n',
    }
    if output.suffix.lower() == '.7z':
        seven_zip = args.seven_zip or shutil.which('7z') or r'C:\Program Files\7-Zip\7z.exe'
        if not Path(seven_zip).is_file():
            raise FileNotFoundError('Install 7-Zip or pass --seven-zip')
        # Keep an inspectable unpacked build alongside the archive. Hard links
        # avoid copying gigabytes on the same volume; archive ordinary contents.
        staging = output.parent / root
        staging.mkdir(exist_ok=False)
        for name, path in sorted(files.items()):
            destination = staging / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            try:
                os.link(path, destination)
            except OSError:
                shutil.copy2(path, destination)
        for name, content in extra.items():
            (staging / name).write_bytes(content.encode('utf-8'))
        command = [str(seven_zip), 'a', '-t7z', '-mx=9',
                   f'-m0=LZMA2:d={args.dictionary_mib}m:fb=273',
                   '-mmt=2', '-ms=on', '-mqs=on', '-bsp1', str(output), root]
        print('7z: solid LZMA2, dictionary '+str(args.dictionary_mib)+' MiB, fast bytes 273', flush=True)
        subprocess.run(command, cwd=output.parent, check=True)
        print('Checking 7z integrity...', flush=True)
        subprocess.run([str(seven_zip), 't', '-bsp0', str(output)], check=True)
    else:
        with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=4, allowZip64=True) as archive:
            for name, path in sorted(files.items()):
                archive.write(path, root + '/' + name)
            for name, content in extra.items():
                archive.writestr(root + '/' + name, content)
        print('Checking ZIP integrity...', flush=True)
        with zipfile.ZipFile(output) as archive:
            bad = archive.testzip()
            if bad is not None:
                raise ValueError(f'ZIP integrity check failed: {bad}')
    output.with_suffix('.sha256').write_text(sha256(output) + '  ' + output.name + '\n', encoding='ascii')
    print(f'Package verified: {output} ({output.stat().st_size} bytes)', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True, help='Directory containing Game.exe and res-hd')
    parser.add_argument('--crt-runtime', type=Path, required=True, help='x64 Microsoft.VC143.CRT directory')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--no-hd-textures', action='store_true', help='Explicitly opt out of the default HD pack')
    parser.add_argument('--seven-zip', type=Path, help='7z executable; used for .7z output')
    parser.add_argument('--dictionary-mib', type=int, choices=(64, 128, 256, 512, 1024), default=512,
                        help='7z LZMA2 dictionary in MiB (default: 512; unpacking needs this much RAM)')
    package(parser.parse_args())

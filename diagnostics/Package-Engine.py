"""Package only the engine/runtime, never original game data or the HD pack."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tarfile
import tempfile
import zipfile
from CheckBgfxImports import imports as pe_imports

ROOT = Path(__file__).resolve().parents[1]


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def copy(source, destination):
    if not source.is_file():
        raise FileNotFoundError(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def copy_matches(directory, pattern, destination):
    paths = sorted(directory.glob(pattern))
    if not paths:
        raise FileNotFoundError(f'{directory}/{pattern}')
    for path in paths:
        copy(path, destination / path.name)


def package(args):
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', args.version):
        raise ValueError('Version must be a filename-safe tag')
    build = args.build.resolve()
    deps = args.deps.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    name = f'silent-storm-engine-{args.version}-{args.platform}-x64'
    extension = '.zip' if args.platform == 'windows' else '.tar.gz'
    archive = args.output.resolve() / (name + extension)
    if archive.exists():
        raise FileExistsError(archive)
    with tempfile.TemporaryDirectory(prefix='s2-engine-') as temporary:
        stage = Path(temporary) / name
        stage.mkdir()
        copy(build / ('Game.exe' if args.platform == 'windows' else 'Game'),
             stage / ('Game.exe' if args.platform == 'windows' else 'Game'))
        build_id = (build / 'network-build-id.txt').read_text(encoding='ascii').strip()
        if not re.fullmatch('[0-9a-f]{64}', build_id):
            raise ValueError('Missing or invalid network build ID')
        copy(build / 'network-build-id.txt', stage / 'network-build-id.txt')
        shutil.copytree(ROOT / 'assets/fonts', stage / 'fonts')
        copy(ROOT / 'diagnostics/Generate-NetworkIdentity.py', stage / 'Generate-NetworkIdentity.py')
        copy(ROOT / 'packaging/README-engine.txt', stage / 'README.txt')
        copy(ROOT / 'LICENSE.md', stage / 'LICENSE.md')
        runtime = stage if args.platform == 'windows' else stage / 'lib'
        runtime.mkdir(exist_ok=True)
        if args.platform == 'windows':
            if not args.crt_runtime:
                raise ValueError('--crt-runtime is required on Windows')
            for dll in ('SDL3.dll', 'zlib.dll', 'binkw32.dll'):
                copy(build / dll, runtime / dll)
            copy_matches(args.crt_runtime, '*.dll', runtime)
            for component in ('avcodec', 'avformat', 'avutil', 'swscale', 'swresample'):
                copy_matches(deps / 'install/bin', component + '-*.dll', runtime)
            for path in runtime.iterdir():
                if path.suffix.lower() in ('.exe', '.dll'):
                    with path.open('rb') as stream:
                        stream.seek(0x3c)
                        stream.seek(struct.unpack('<I', stream.read(4))[0] + 4)
                        if stream.read(2) != b'\x64\x86':
                            raise ValueError(f'Expected Windows x64: {path}')
            # Check imports against the staged files, not DLLs on the build PATH.
            system = Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'System32'
            dlls = {p.name.lower() for p in runtime.glob('*.dll')}
            for path in (stage / 'Game.exe', *runtime.glob('*.dll')):
                for dependency in pe_imports(path):
                    if dependency in dlls or dependency.startswith(('api-ms-win-', 'ext-ms-win-')):
                        continue
                    if not (system / dependency).is_file():
                        raise ValueError(f'Unpackaged runtime dependency: {path.name}: {dependency}')
            copy(ROOT / 'packaging/Play.cmd', stage / 'Play.cmd')
        else:
            # Preserve each SONAME as an ordinary file; no absolute vendor links.
            for pattern in ('libSDL3.so*', 'libavcodec.so*', 'libavformat.so*',
                            'libavutil.so*', 'libswscale.so*', 'libswresample.so*'):
                copy_matches(deps / 'install/lib', pattern, runtime)
            copy(ROOT / 'packaging/Play.sh', stage / 'Play.sh')
            (stage / 'Play.sh').chmod(0o755)
            (stage / 'Game').chmod(0o755)
            env = dict(os.environ, LD_LIBRARY_PATH=str(runtime))
            for path in (stage / 'Game', *runtime.glob('*.so.*')):
                result = subprocess.check_output(['ldd', str(path)], env=env, text=True)
                if 'not found' in result:
                    raise ValueError(f'Unresolved Linux runtime dependency: {path}\n{result}')
                for line in result.splitlines():
                    if any(component in line for component in ('libSDL3', 'libavcodec', 'libavformat', 'libavutil', 'libswscale', 'libswresample')):
                        if str(runtime) not in line:
                            raise ValueError(f'Runtime resolved outside package: {line}')
        # Include notices for bundled code and the exact unmodified FFmpeg source.
        licenses = stage / 'licenses'
        for module in ('SDL3-3.4.16', 'bgfx.cmake', 'bgfx', 'bimg', 'bx', 'ffmpeg-8.1'):
            directory = deps / module
            notices = [p for p in directory.rglob('*') if p.is_file() and
                       (p.name.upper().startswith(('LICENSE', 'COPYING')) or p.name == 'UNLICENSE') and '.git' not in p.parts]
            if not notices:
                raise FileNotFoundError(f'Missing notices for {module}')
            for source in notices:
                copy(source, licenses / module / source.relative_to(directory))
        for relative in ('third_party/squish/LICENSE.txt', 'third_party/zlib/zlib.h',
                         'third_party/libpng/png.h', 'Script/lua.h'):
            copy(ROOT / relative, licenses / relative)
        copy(deps / 'miniaudio.h', licenses / 'miniaudio.h')
        copy(deps / 'sources.json', stage / 'dependencies/sources.json')
        copy(deps / 'ffmpeg-8.1.tar.xz', stage / 'dependencies/ffmpeg-8.1.tar.xz')
        copy(deps / 'ffmpeg-config.log', stage / 'dependencies/ffmpeg-config.log')
        copy(ROOT / '.github/scripts/build-deps.sh', stage / 'dependencies/build-deps.sh')
        revision = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
        info = {'version': args.version, 'revision': revision, 'platform': args.platform,
                'architecture': 'x64', 'networkBuild': build_id,
                'gameData': False, 'hdTextures': False,
                'sha256': {p.relative_to(stage).as_posix(): sha256(p)
                           for p in sorted(stage.rglob('*')) if p.is_file()}}
        (stage / 'engine-build.json').write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
        if args.platform == 'windows':
            with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as output:
                for path in sorted(stage.rglob('*')):
                    if path.is_file():
                        output.write(path, name + '/' + path.relative_to(stage).as_posix())
            with zipfile.ZipFile(archive) as output:
                if output.testzip():
                    raise ValueError('ZIP integrity check failed')
        else:
            with tarfile.open(archive, 'w:gz', dereference=True) as output:
                output.add(stage, arcname=name)
            with tarfile.open(archive) as output:
                for member in output:
                    if member.isfile():
                        relative = Path(member.name).relative_to(name).as_posix()
                        expected = info['sha256'].get(relative)
                        if expected and hashlib.sha256(output.extractfile(member).read()).hexdigest() != expected:
                            raise ValueError(f'Archive integrity check failed: {relative}')
    archive.with_name(archive.name + '.sha256').write_text(sha256(archive) + '  ' + archive.name + '\n', encoding='ascii')
    print(f'Engine package verified: {archive} ({archive.stat().st_size} bytes)', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--deps', type=Path, required=True)
    parser.add_argument('--platform', choices=('windows', 'linux'), required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--crt-runtime', type=Path)
    package(parser.parse_args())

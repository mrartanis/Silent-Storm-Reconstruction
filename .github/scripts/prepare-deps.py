"""Fetch pinned release dependencies; no game data or HD assets are needed."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request
import zipfile

GITHUB_SOURCES = {
    'bgfx.cmake': ('bkaradzic/bgfx.cmake', 'de08a6080b39994ab8a9eddb82e79e18bc3df7bd'),
    'bgfx': ('bkaradzic/bgfx', '81d81fba72c42d348c589514c774bbfe01e110fa'),
    'bimg': ('bkaradzic/bimg', '87aaad3ac882e741889fdd4263224e5d12c26f99'),
    'bx': ('bkaradzic/bx', '25315498841259323e18f549d1ad9d9aba6632cc'),
}
DOWNLOADS = {
    'ffmpeg-8.1.tar.xz': (
        'https://ffmpeg.org/releases/ffmpeg-8.1.tar.xz',
        'b072aed6871998cce9b36e7774033105ca29e33632be5b6347f3206898e0756a'),
    'SDL3-3.4.16.tar.gz': (
        'https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz',
        '7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68'),
    'SDL3-devel-3.4.16-VC.zip': (
        'https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-devel-3.4.16-VC.zip',
        '1a784cb2a5c64d56fe7a62090fe9d242d9865f235e4ea9678f1a6ba4e693e7de'),
    'miniaudio.h': (
        'https://raw.githubusercontent.com/mackron/miniaudio/0.11.25/miniaudio.h',
        'ac7af4de748b7e26b777f37e01cee313a308a7296a3eb080e2906b320cc55c89'),
}


def run(*args, **kwargs):
    subprocess.run(args, check=True, **kwargs)


def download(root, name):
    url, expected = DOWNLOADS[name]
    path = root / name
    if not path.exists():
        print(f'Downloading {url}', flush=True)
        request = urllib.request.Request(url, headers={'User-Agent': 'Silent-Storm-release'})
        with urllib.request.urlopen(request, timeout=180) as response, path.open('wb') as output:
            shutil.copyfileobj(response, output)
    with path.open('rb') as stream:
        actual = hashlib.file_digest(stream, 'sha256').hexdigest()
    if actual != expected:
        raise ValueError(f'Checksum mismatch: {name}: {actual}')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--platform', choices=('windows', 'linux'), required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    root.mkdir(parents=True, exist_ok=True)
    for name, (repo, revision) in GITHUB_SOURCES.items():
        destination = root / name
        if not destination.exists():
            run('git', 'init', str(destination))
            run('git', '-C', str(destination), 'remote', 'add', 'origin', f'https://github.com/{repo}.git')
        run('git', '-C', str(destination), 'fetch', '--depth=1', 'origin', revision)
        run('git', '-C', str(destination), 'checkout', '--detach', revision)
        actual = subprocess.check_output(['git', '-C', str(destination), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != revision:
            raise ValueError(f'Unexpected revision: {name}')
    archive = download(root, 'ffmpeg-8.1.tar.xz')
    if not (root / 'ffmpeg-8.1').exists():
        with tarfile.open(archive) as package:
            package.extractall(root, filter='data')
    sdl_name = 'SDL3-devel-3.4.16-VC.zip' if args.platform == 'windows' else 'SDL3-3.4.16.tar.gz'
    archive = download(root, sdl_name)
    if not (root / 'SDL3-3.4.16').exists():
        if args.platform == 'windows':
            with zipfile.ZipFile(archive) as package:
                package.extractall(root)
        else:
            with tarfile.open(archive) as package:
                package.extractall(root, filter='data')
    download(root, 'miniaudio.h')
    (root / 'sources.json').write_text(json.dumps({
        'git': GITHUB_SOURCES, 'downloads': DOWNLOADS,
    }, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()

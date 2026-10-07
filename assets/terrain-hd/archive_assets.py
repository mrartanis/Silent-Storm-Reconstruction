"""Store HD PNGs in stable ZIP bundles; download, restore and build offline.

python assets/terrain-hd/archive_assets.py pack
python assets/terrain-hd/archive_assets.py unpack --download --build --validate
PNG files already contain compressed image data; ZIP stores their original bytes.
Identical files share one object. Numeric resource ranges keep updates local.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parent
INDEX = ROOT / 'archives/index.json'
BUFFER = 1024 * 1024


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def within(root, relative):
    name = PurePosixPath(relative)
    if name.is_absolute() or '..' in name.parts or '\\' in relative or ':' in relative:
        raise ValueError(f'Unsafe archive path: {relative}')
    destination = root.joinpath(*name.parts)
    if not destination.resolve().is_relative_to(root.resolve()):
        raise ValueError(f'Path escapes output directory: {relative}')
    return destination


def pack():
    directory = ROOT / 'archives'
    directory.mkdir(exist_ok=True)
    previous = json.loads(INDEX.read_text(encoding='utf-8')) if INDEX.exists() else {}
    # A clone may contain the index and LFS bundles without unpacked images.
    # Never replace that complete archive set with a partial working directory.
    missing = [entry['path'] for entry in previous.get('files', [])
               if not within(ROOT, entry['path']).is_file()]
    if missing:
        raise ValueError(f'{len(missing)} previously archived PNG files are missing '
                         f'(first: {missing[0]}). Run unpack before pack; '
                         'the existing archives and index have been preserved.')
    objects = {}
    files = []
    groups = {}
    for path in sorted(ROOT.rglob('*.png')):
        relative = path.relative_to(ROOT).as_posix()
        sha = digest(path)
        size = path.stat().st_size
        match = re.match(r'(\d+)', path.name)
        group = f'textures-{int(match[1]) // 128:04d}' if match else 'prototype'
        files.append({'path': relative, 'sha256': sha, 'bytes': size})
        if sha not in objects:
            archive = f'{group}.zip'
            member = f'objects/{sha}.png'
            objects[sha] = {'archive': archive, 'member': member, 'bytes': size}
            groups.setdefault(archive, []).append((path, sha, member))
    assert files, 'No unpacked PNG files to archive'
    archives = []
    for name, members in sorted(groups.items()):
        destination = directory / name
        temporary = destination.with_suffix('.zip.tmp')
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_STORED) as bundle:
            for path, sha, member in members:
                info = zipfile.ZipInfo(member, date_time=(1980, 1, 1, 0, 0, 0))
                info.external_attr = 0o100644 << 16
                with path.open('rb') as source, bundle.open(info, 'w') as target:
                    shutil.copyfileobj(source, target, BUFFER)
        size = temporary.stat().st_size
        sha = digest(temporary)
        if destination.exists() and digest(destination) == sha:
            temporary.unlink()
        else:
            temporary.replace(destination)
        archives.append({'file': name, 'sha256': sha, 'bytes': size, 'objects': len(members)})
        print(f'{name}: {len(members)} objects, {size} bytes', flush=True)
    current = {a['file'] for a in archives}
    for old in previous.get('archives', []):
        if old['file'] in current:
            continue
        obsolete = within(directory, old['file'])
        assert obsolete.parent.resolve() == directory.resolve()
        if obsolete.exists():
            assert digest(obsolete) == old['sha256'], 'Modified obsolete bundle; preserve it'
            obsolete.unlink()
    report = {'schema': 1, 'format': 'ZIP stored PNG objects, SHA-256 deduplicated',
              'resource_range': 128, 'archives': archives, 'objects': objects, 'files': files,
              'materialized_bytes': sum(f['bytes'] for f in files),
              'archive_bytes': sum(a['bytes'] for a in archives)}
    old_archives = {a['file']: a for a in previous.get('archives', [])}
    for archive in archives:
        old = old_archives.get(archive['file'], {})
        if old.get('sha256') == archive['sha256'] and old.get('download_url'):
            archive['download_url'] = old['download_url']
    INDEX.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'{len(files)} PNG files, {len(objects)} unique objects, {len(archives)} archives; '
          f'{report["archive_bytes"]} bytes stored', flush=True)


def download():
    report = json.loads(INDEX.read_text(encoding='utf-8'))
    for archive in report['archives']:
        path = within(INDEX.parent, archive['file'])
        if path.exists():
            if path.stat().st_size == archive['bytes'] and digest(path) == archive['sha256']:
                continue
            with path.open('rb') as stream:
                pointer = stream.read(256).startswith(b'version https://git-lfs.github.com/spec/v1\n')
            if not pointer:
                raise ValueError(f'Local archive differs; preserve it before downloading: {path}')
        url = archive.get('download_url')
        if not url or not url.startswith('https://github.com/'):
            raise ValueError(f'No published GitHub download URL for {archive["file"]}')
        temporary = path.with_suffix('.zip.download-tmp')
        print(f'Downloading {archive["file"]}: {archive["bytes"]} bytes', flush=True)
        try:
            request = urllib.request.Request(url, headers={'User-Agent': 'Silent-Storm-HD-assets'})
            hasher = hashlib.sha256()
            size = 0
            with urllib.request.urlopen(request, timeout=60) as source, temporary.open('wb') as target:
                while block := source.read(BUFFER):
                    size += len(block)
                    if size > archive['bytes']:
                        raise ValueError(f'Unexpected download size: {archive["file"]}')
                    hasher.update(block)
                    target.write(block)
            if size != archive['bytes'] or hasher.hexdigest() != archive['sha256']:
                raise ValueError(f'Download hash/size mismatch: {archive["file"]}')
            temporary.replace(path)
        finally:
            if temporary.exists():
                temporary.unlink()
    print(f'All {len(report["archives"])} archives verified', flush=True)


def unpack(output, force=False):
    report = json.loads(INDEX.read_text(encoding='utf-8'))
    assert report['schema'] == 1
    output.mkdir(parents=True, exist_ok=True)
    needed = {}
    for entry in report['files']:
        path = within(output, entry['path'])
        if path.exists():
            if path.stat().st_size == entry['bytes'] and digest(path) == entry['sha256']:
                continue
            if not force:
                raise ValueError(f'Local image differs; preserve it or use --force: {path}')
        obj = report['objects'][entry['sha256']]
        assert obj['bytes'] == entry['bytes']
        needed.setdefault(obj['archive'], []).append((entry, obj, path))
    archive_map = {a['file']: a for a in report['archives']}
    restored = 0
    for name, entries in sorted(needed.items()):
        path = within(INDEX.parent, name)
        archive = archive_map[name]
        if path.stat().st_size != archive['bytes'] or digest(path) != archive['sha256']:
            raise ValueError(f'Archive corrupt or still an LFS pointer: {path}; run unpack --download')
        with zipfile.ZipFile(path) as bundle:
            for entry, obj, destination in entries:
                destination.parent.mkdir(parents=True, exist_ok=True)
                temporary = destination.with_suffix('.png.restore-tmp')
                try:
                    hasher = hashlib.sha256()
                    size = 0
                    with bundle.open(obj['member']) as source, temporary.open('wb') as target:
                        while block := source.read(BUFFER):
                            hasher.update(block)
                            size += len(block)
                            target.write(block)
                    assert size == entry['bytes'] and hasher.hexdigest() == entry['sha256']
                    temporary.replace(destination)
                finally:
                    if temporary.exists():
                        temporary.unlink()
                restored += 1
        print(f'Restored {name}: {len(entries)} files', flush=True)
    print(f'Restored {restored} files; {len(report["files"]) - restored} already matched', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('pack', 'unpack'))
    parser.add_argument('--output', type=Path, default=ROOT)
    parser.add_argument('--force', action='store_true', help='Replace changed local images during unpack')
    parser.add_argument('--download', action='store_true', help='Fetch missing archives from pinned GitHub releases')
    parser.add_argument('--build', action='store_true', help='Build ready native resources after unpack')
    parser.add_argument('--validate', action='store_true', help='Validate all rebuilt native resources')
    args = parser.parse_args()
    if args.command == 'pack':
        if args.download or args.build or args.validate:
            parser.error('--download/--build/--validate require unpack')
        pack()
    else:
        if args.download:
            download()
        unpack(args.output, args.force)
        if args.build or args.validate:
            if args.output.resolve() != ROOT.resolve():
                parser.error('Build/validation use the repository asset directory; omit --output')
            if args.build:
                subprocess.run([sys.executable, str(ROOT / 'build_pack.py')], check=True)
            if args.validate:
                subprocess.run([sys.executable, str(ROOT / 'validate_pack.py')], check=True)


if __name__ == '__main__':
    main()

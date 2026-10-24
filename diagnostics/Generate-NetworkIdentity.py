"""Compute compatibility data once during build/packaging, never in the game."""
import argparse
import hashlib
import json
from pathlib import Path


def generate(game_data, build_id, output, incremental=False):
    if len(build_id) != 64 or any(c not in '0123456789abcdef' for c in build_id):
        raise ValueError('Invalid build ID')
    paths = [game_data / 'game.db']
    for name in ('res', 'scripts'):
        directory = game_data / name
        if not directory.is_dir():
            continue
        paths += sorted(p for p in directory.rglob('*') if p.is_file())
    inputs = [[p.relative_to(game_data).as_posix(), p.stat().st_size, p.stat().st_mtime_ns] for p in paths]
    cache_path = output.with_suffix('.cache.json')
    cache = json.loads(cache_path.read_text()) if incremental and cache_path.exists() else {}
    if cache.get('inputs') == inputs:
        digest = cache['digest']
    else:
        checksum = hashlib.sha256()
        for path, (name, size, _) in zip(paths, inputs):
            checksum.update(name.lower().encode('utf-8') + b'\0' + str(size).encode('ascii') + b'\0')
            with path.open('rb') as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    checksum.update(block)
        digest = checksum.hexdigest()
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix('.tmp')
    temporary.write_text(f'S2-NETWORK-1\n{build_id}\n{digest}\n', encoding='ascii')
    temporary.replace(output)
    if incremental:
        cache_path.write_text(json.dumps(dict(inputs=inputs, digest=digest)), encoding='utf-8')
    print(f'Network identity: {build_id[:12]} / {digest[:12]}', flush=True)
    return digest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', required=True, type=Path)
    parser.add_argument('--build-id', required=True)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--incremental', action='store_true')
    args = parser.parse_args()
    generate(args.game_data.resolve(), args.build_id, args.output, args.incremental)

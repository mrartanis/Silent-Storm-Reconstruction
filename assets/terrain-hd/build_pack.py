"""Build the visual overlay offline: Pillow PNGs -> native BGRA MMP + .res.

Run: python assets/terrain-hd/build_pack.py [--output res-hd]
No AI processing or conversion takes place in the game.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image, ImageChops, ImageStat

ROOT = Path(__file__).resolve().parent


def chunk(tag, payload):
    n = len(payload)
    return bytes([tag]) + (bytes([n << 1]) if n < 128 else struct.pack('<I', (n << 1) | 1)) + payload


def mmp(image, premultiply=False, alpha_reference=None):
    image = image.convert('RGBA')
    if alpha_reference is not None:
        image.putalpha(alpha_reference.convert('RGBA').getchannel('A').resize(image.size, Image.Resampling.LANCZOS))
    if premultiply:
        r, g, b, a = image.split()
        image = Image.merge('RGBA', (ImageChops.multiply(r, a),
                                    ImageChops.multiply(g, a), ImageChops.multiply(b, a), a))
    # Stop at a one-pixel height: the legacy GPU loader shifts both dimensions.
    levels = []
    level = image
    while True:
        levels.append(level.tobytes('raw', 'BGRA'))
        if min(level.size) == 1:
            break
        # Average stored channels directly. Premultiplied sprite colors must
        # not be multiplied a second time by Pillow's RGBA resampler.
        size = (level.width // 2, level.height // 2)
        level = Image.merge('RGBA', tuple(c.resize(size, Image.Resampling.BOX) for c in level.split()))
    r, g, b, a = [round(x) for x in ImageStat.Stat(image).mean]
    average = b | (g << 8) | (r << 16) | (a << 24)
    return struct.pack('<6I', 0x504d4d, 6, average, image.width, image.height, len(levels)) + b''.join(levels)


def build(output):
    output.mkdir(parents=True, exist_ok=True)
    previous = json.loads((output / 'manifest.json').read_text(encoding='utf-8')) if (output / 'manifest.json').exists() else {}
    manifest = json.loads((ROOT / 'sources.json').read_text(encoding='utf-8'))
    payload = bytearray(8)
    entries = []
    archives = []
    max_archive_bytes = 64 * 1024 * 1024
    def flush():
        nonlocal payload, entries
        if not entries:
            return
        index_offset = len(payload)
        index = b''.join(chunk(1, struct.pack('<i', id)) + chunk(2, struct.pack('<II', offset, length))
                         for id, offset, length in sorted(entries))
        payload.extend(chunk(1, chunk(1, index)))
        struct.pack_into('<II', payload, 0, 0x96948a22, index_offset)
        name = 'Textures.res' if not archives else f'Textures-{len(archives):04d}.res'
        (output / name).write_bytes(payload)
        archives.append({'file': name, 'sha256': hashlib.sha256(payload).hexdigest(),
                         'bytes': len(payload), 'keys': len(entries)})
        payload = bytearray(8)
        entries = []
    for asset in manifest['textures']:
        atlas = Image.open(ROOT / asset['png'])
        logical = asset.get('logical_size', [256, 64])
        assert atlas.size == tuple(n * 4 for n in logical), (asset['id'], atlas.size)
        assert all(n > 0 and n & (n - 1) == 0 for n in atlas.size)
        alpha = Image.open(ROOT / asset['native_alpha_reference']) if asset.get('native_alpha_reference') else None
        data = mmp(atlas, asset.get('alpha_encoding') == 'premultiplied', alpha)
        if entries and len(payload) + len(data) + 4096 > max_archive_bytes:
            flush()
        offset = len(payload)
        payload.extend(data)
        # Both DXT-mode keys select this ready-to-use uncompressed replacement.
        for id in (asset['id'], asset['id'] | 0x01000000):
            entries.append((id, offset, len(data)))
        asset['mmp_sha256'] = hashlib.sha256(data).hexdigest()
        asset['logical_size'] = logical
        asset['physical_size'] = list(atlas.size)
        asset['density'] = 4
        asset['mips'] = struct.unpack_from('<I', data, 20)[0]
        asset['png_sha256'] = hashlib.sha256((ROOT / asset['png']).read_bytes()).hexdigest()
    flush()
    current_names = {a['file'] for a in archives}
    for old in previous.get('archives', []):
        if old['file'] in current_names:
            continue
        path = output / old['file']
        # Remove only files owned by the previous manifest and still unchanged.
        assert path.resolve().parent == output.resolve(), old['file']
        if path.exists():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == old['sha256'], path
            path.unlink()
    manifest['archives'] = archives
    manifest['pack_sha256'] = archives[0]['sha256']
    (output / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'{output}: {len(manifest["textures"])} textures, {len(archives)} archives, '
          f'{sum(a["bytes"] for a in archives)} bytes')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT.parents[1] / 'res-hd')
    build(parser.parse_args().output)

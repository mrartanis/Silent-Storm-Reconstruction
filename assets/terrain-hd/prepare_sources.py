"""Read-only native source verification and additive HD queue preparation.

Input: a JSON list of {texture, category, role, optional layout/relation}.
An audited source_resource_id can select the original uncompressed resource alias;
source_resource_selection_reason must document the actual loader selection.
No historical or release resource is modified. Existing queue entries are retained.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct

from PIL import Image

ROOT = Path(__file__).resolve().parent


def chunks(data):
    pos = 0
    while pos < len(data):
        if pos + 2 > len(data):
            raise ValueError('Truncated chunk header')
        tag = data[pos]
        count = 4 if data[pos + 1] & 1 else 1
        length = int.from_bytes(data[pos + 1:pos + 1 + count], 'little') >> 1
        start = pos + 1 + count
        if start + length > len(data):
            raise ValueError('Truncated chunk')
        yield tag, data[start:start + length]
        pos = start + length


def package_index(path):
    with path.open('rb') as stream:
        signature, offset = struct.unpack('<II', stream.read(8))
        if signature != 0x96948a22:
            raise ValueError(f'Invalid resource archive: {path}')
        stream.seek(offset)
        root = dict(chunks(stream.read()))[1]
    pairs = list(chunks(dict(chunks(root))[1]))
    result = {}
    for n in range(0, len(pairs), 2):
        assert pairs[n][0] == 1 and pairs[n + 1][0] == 2
        result[struct.unpack('<i', pairs[n][1])[0]] = struct.unpack('<II', pairs[n + 1][1])
    return result


class ReleaseResources:
    def __init__(self, root):
        self.root = Path(root)
        self.index = {}
        for archive in sorted(self.root.glob('Textures*.res')):
            for key, (offset, size) in package_index(archive).items():
                if key in self.index:
                    raise ValueError(f'Duplicate release resource {key}')
                self.index[key] = (archive, offset, size)

    def read(self, resource_id):
        loose = self.root / 'Textures' / str(resource_id)
        if loose.exists():
            return loose.read_bytes()
        archive, offset, size = self.index[resource_id]
        with archive.open('rb') as stream:
            stream.seek(offset)
            data = stream.read(size)
        assert len(data) == size
        return data


def decode_mmp(data):
    signature, fmt, average, width, height, mips = struct.unpack_from('<6I', data)
    if signature != 0x504d4d or width < 1 or height < 1:
        raise ValueError('Invalid MMP header')
    if fmt == 6:
        image = Image.frombytes('RGBA', (width, height), data[24:24 + width * height * 4], 'raw', 'BGRA')
    elif fmt == 8:
        # Native CF_R5G6B5 is ordinary RGB, with opaque alpha.
        values = struct.unpack_from('<' + 'H' * (width * height), data, 24)
        pixels = bytearray()
        for value in values:
            r, g, b = (value >> 11) & 31, (value >> 5) & 63, value & 31
            pixels.extend(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2), 255))
        image = Image.frombytes('RGBA', (width, height), bytes(pixels))
    elif fmt in (1, 2, 3, 4, 5):
        fourcc = {1: b'DXT1', 2: b'DXT3', 3: b'DXT3', 4: b'DXT5', 5: b'DXT5'}[fmt]
        header = (struct.pack('<7I', 124, 0x1007, height, width, 0, 0, 1) + bytes(44)
                  + struct.pack('<II4s5I', 32, 4, fourcc, 0, 0, 0, 0, 0)
                  + struct.pack('<5I', 0x1000, 0, 0, 0, 0))
        image = Image.open(io.BytesIO(b'DDS ' + header + data[24:])).convert('RGBA')
    else:
        raise ValueError(f'Unsupported native MMP format {fmt}')
    return image


def prepare(candidates, historical, release, output, append=False, revisit_originals=()):
    resources = ReleaseResources(release)
    queue_path = ROOT / 'expanded/queue.json'
    queue = json.loads(queue_path.read_text(encoding='utf-8'))
    known = {entry['id'] for entry in queue}
    queue_by_id = {entry['id']: entry for entry in queue}
    revisited = set()
    revisit_originals = set(revisit_originals)
    accepted = {entry['id'] for entry in json.loads((ROOT / 'sources.json').read_text(encoding='utf-8'))['textures']}
    prepared = []
    for row in candidates:
        texture = row['texture']
        resource_id = texture['ID']
        if resource_id in accepted:
            if resource_id in revisit_originals:
                raise ValueError(f'{resource_id}: accepted HD sources cannot be revisited')
            continue
        if resource_id in known and resource_id not in revisit_originals:
            continue
        prior = queue_by_id.get(resource_id) if resource_id in revisit_originals else None
        if prior is not None:
            if prior['status'] != 'structural-mask-or-solid' or not row.get('reviewed_original_reclassification_reason'):
                raise ValueError(f'{resource_id}: individual structural-original review is required')
        source_id = row.get('source_resource_id', resource_id)
        if not isinstance(source_id, int) or source_id not in (resource_id, resource_id | 0x01000000):
            raise ValueError(f'{resource_id}: invalid original resource alias')
        if source_id != resource_id and not row.get('source_resource_selection_reason'):
            raise ValueError(f'{resource_id}: audited resource selection reason is required')
        item = {'id': resource_id, 'texture': texture, 'category': row['category'],
                'role': row['role'], 'status': 'pending'}
        if prior is not None:
            item['previous_original_disposition'] = prior
            item['reviewed_original_reclassification_reason'] = row['reviewed_original_reclassification_reason']
        for key in ('layout', 'relation', 'usage', 'review_requirements'):
            if key in row:
                item[key] = row[key]
        if 'source_resource_id' in row:
            item['source_resource_id'] = source_id
            item['source_resource_selection_reason'] = row.get('source_resource_selection_reason', 'Primary original resource')
        source = Path(historical) / str(source_id)
        if not source.exists():
            item.update(status='missing-historical', reason='Historical MMP file unavailable')
        else:
            data = source.read_bytes()
            try:
                image = decode_mmp(data)
                released = decode_mmp(resources.read(source_id))
                if image.size != released.size or image.tobytes() != released.tobytes():
                    item.update(status='release-mismatch', reason='Decoded dimensions or RGBA differ from release')
                else:
                    item.update(logical_size=list(image.size), source_sha256=hashlib.sha256(data).hexdigest(),
                                source_rgba_sha256=hashlib.sha256(image.tobytes()).hexdigest(),
                                release_rgba_sha256=hashlib.sha256(released.tobytes()).hexdigest(),
                                source_native_format=struct.unpack_from('<I', data, 4)[0],
                                alpha_extrema=list(image.getchannel('A').getextrema()),
                                original_png=(f'expanded/original/{resource_id}.png' if source_id == resource_id
                                              else f'expanded/original/{resource_id}-resource-{source_id}.png'),
                                source_match='Exact decoded RGBA and dimensions match release')
                    if texture['Type'].lower() == 'bump' or texture['Format'].lower() in ('normal', 'normals'):
                        item.update(status='original-technical-map', reason='DB Bump/normal type')
                    elif image.size != (texture['Width'], texture['Height']):
                        item.update(status='physical-logical-size-audit-required',
                                    db_logical_size=[texture['Width'], texture['Height']],
                                    reason='Historical/release physical size differs from DB logical dimensions; review loader/UV before generation')
                    destination = ROOT / item['original_png']
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    if destination.exists():
                        assert Image.open(destination).convert('RGBA').tobytes() == image.tobytes()
                    else:
                        image.save(destination)
            except KeyError:
                item.update(status='missing-release', reason='Release resource ID unavailable')
            except ValueError as error:
                item.update(status='unsupported-source-format', reason=str(error))
        if prior is not None:
            if item['status'] != 'pending':
                raise ValueError(f'{resource_id}: reviewed original source failed fresh verification')
            revisited.add(resource_id)
        prepared.append(item)
        known.add(resource_id)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(prepared, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    if append:
        queue_path.write_text(json.dumps([entry for entry in queue if entry['id'] not in revisited] + prepared,
                                       ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    return prepared


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidates', type=Path, required=True)
    parser.add_argument('--historical', type=Path, required=True)
    parser.add_argument('--release', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--append', action='store_true')
    parser.add_argument('--revisit-originals', nargs='+', type=int, default=[],
                        help='Explicit individually reviewed structural-original IDs; accepted HD entries are protected')
    args = parser.parse_args()
    records = prepare(json.loads(args.candidates.read_text(encoding='utf-8')), args.historical,
                      args.release, args.output, args.append, args.revisit_originals)
    from collections import Counter
    print(len(records), dict(Counter(item['status'] for item in records)))

"""Import saved built-in image-generation results into the offline HD pipeline.

python assets/terrain-hd/import_generated.py --batch jobs.json
Each job contains id, generated (saved PNG path), and prompt (exact text).
Generation remains a separate explicit built-in image_gen operation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parent


def import_jobs(jobs, replace=False):
    manifest = json.loads((ROOT / 'sources.json').read_text(encoding='utf-8'))
    queue = json.loads((ROOT / 'expanded/queue.json').read_text(encoding='utf-8'))
    by_id = {x['id']: x for x in queue}
    known = {x['id'] for x in manifest['textures']}
    out = ROOT / 'expanded/generated'
    out.mkdir(parents=True, exist_ok=True)
    for job in jobs:
        id = job['id']
        if id in known and not replace:
            raise ValueError(f'{id} already imported; use --replace for a reviewed retry')
        item = by_id[id]
        reuse_id = job.get('reuse_source_id')
        if reuse_id is not None:
            donor = by_id[reuse_id]
            assert donor['source_rgba_sha256'] == item['source_rgba_sha256'], (id, reuse_id, 'Different source pixels')
            assert donor['texture']['Type'] == item['texture']['Type'], (id, reuse_id, 'Different alpha encoding')
            assert donor['logical_size'] == item['logical_size']
            assert donor.get('layout') == item.get('layout'), (id, reuse_id, 'Different atlas layout')
            assert reuse_id in known, (id, reuse_id, 'Unreviewed donor')
        original = Image.open(ROOT / item['original_png']).convert('RGBA')
        assert hashlib.sha256(original.tobytes()).hexdigest() == item['source_rgba_sha256']
        raw = out / f'{id}-raw.png'
        saved = Path(job['generated'])
        if saved.resolve() != raw.resolve():
            shutil.copy2(saved, raw)
        image = Image.open(raw).convert('RGBA')
        logical = item['logical_size']
        assert list(original.size) == logical
        normalized = out / f'{id}.png'
        target_size = tuple(n * 4 for n in logical)
        registration = None
        if job.get('uv_regions'):
            # Explicitly reviewed atlas islands only. Coordinates are recorded
            # in raw generated pixels and original logical pixels, respectively.
            assert not job.get('register_alpha_bbox') and not job.get('register_rgb_bbox')
            assert original.getchannel('A').getextrema() == (255, 255)
            registered = Image.new('RGBA', target_size, (0, 0, 0, 255))
            occupied = Image.new('L', target_size)
            regions = []
            for region in job['uv_regions']:
                source_bbox = region['source_bbox']
                generated_bbox = region['generated_bbox']
                assert len(source_bbox) == len(generated_bbox) == 4
                assert all(isinstance(n, int) for n in source_bbox + generated_bbox)
                assert 0 <= source_bbox[0] < source_bbox[2] <= original.width
                assert 0 <= source_bbox[1] < source_bbox[3] <= original.height
                assert 0 <= generated_bbox[0] < generated_bbox[2] <= image.width
                assert 0 <= generated_bbox[1] < generated_bbox[3] <= image.height
                target_bbox = tuple(n * 4 for n in source_bbox)
                assert occupied.crop(target_bbox).getbbox() is None, (id, 'Overlapping UV regions')
                occupied.paste(255, target_bbox)
                material = image.crop(generated_bbox)
                rotation = region.get('rotate', 0)
                assert rotation in (0, 180), (id, 'Unreviewed island rotation')
                if rotation == 180:
                    material = material.transpose(Image.Transpose.ROTATE_180)
                material = material.resize(
                    (target_bbox[2] - target_bbox[0], target_bbox[3] - target_bbox[1]), Image.Resampling.LANCZOS)
                registered.paste(material, target_bbox[:2])
                regions.append(region)
            registered.save(normalized)
            registration = {'method': 'Reviewed per-island UV registration; opaque black unused atlas regions',
                            'regions': regions, 'reason': job['uv_registration_reason']}
        elif job.get('register_alpha_bbox') or job.get('register_rgb_bbox'):
            # Remove generator letterboxing on extreme-aspect UV strips, then
            # register the material to the original occupied canvas region.
            # Native packing still restores the complete original alpha mask.
            rgb_registration = bool(job.get('register_rgb_bbox'))
            assert not (rgb_registration and job.get('register_alpha_bbox'))
            def occupied_bbox(im):
                if not rgb_registration:
                    channel = im.getchannel('A')
                else:
                    # Reviewed opaque UV atlases use black unused regions.
                    # This removes only verified generator letterboxing.
                    source_ignores_alpha = (im is original and
                        item['texture']['Type'].lower() == 'ordinary' and
                        im.getchannel('A').getextrema() == (0, 0))
                    assert im.getchannel('A').getextrema() == (255, 255) or source_ignores_alpha
                    r, g, b, _ = im.split()
                    channel = ImageChops.lighter(ImageChops.lighter(r, g), b)
                return channel.point(lambda value: 255 if value > 8 else 0).getbbox()
            source_bbox = occupied_bbox(original)
            generated_bbox = occupied_bbox(image)
            assert source_bbox and generated_bbox, (id, 'Empty registration mask')
            target_bbox = tuple(n * 4 for n in source_bbox)
            registered = Image.new('RGBA', target_size,
                                   (0, 0, 0, 255 if rgb_registration else 0))
            material = image.crop(generated_bbox).resize(
                (target_bbox[2]-target_bbox[0], target_bbox[3]-target_bbox[1]), Image.Resampling.LANCZOS)
            registered.paste(material, target_bbox[:2])
            registered.save(normalized)
            registration = {'method': ('RGB occupied bounding-box UV registration' if rgb_registration
                                       else 'Alpha bounding-box UV registration'),
                            'generated_bbox': list(generated_bbox), 'source_bbox': list(source_bbox)}
        else:
            image.resize(target_size, Image.Resampling.LANCZOS).save(normalized)
        prompt = out / f'{id}-prompt.txt'
        # Preserve CRLF already present in exact service arguments on Windows.
        prompt.write_bytes((job['prompt'] + '\n').encode('utf-8'))
        grid = [4, 1] if item['role'] == 'base-tile' else [1, 1]
        if item['role'] == 'grass-sprite':
            grid = [item.get('layout', {}).get('SideSize', 4)] * 2
        asset = {
            'id': id, 'material': item['texture']['UserName'] or item['texture']['SrcName'],
            'category': item['category'], 'png': normalized.relative_to(ROOT).as_posix(),
            'uncalibrated_png': normalized.relative_to(ROOT).as_posix(),
            'reference_png': item['original_png'], 'logical_size': logical,
            'calibration_grid': grid, 'source': f'Nival historical Complete/Textures/{id}',
            'source_rgba_sha256': item['source_rgba_sha256'],
            'source_match': 'Exact decoded RGBA match to baseline release resource',
            'generated_png': raw.relative_to(ROOT).as_posix(), 'generated_size': list(image.size),
            'prompt_file': prompt.relative_to(ROOT).as_posix(),
            'generation': 'Built-in image_gen; source-preserving upscale; offline native-size resampling',
        }
        if item['texture']['Type'].lower() == 'transparent':
            asset['alpha_encoding'] = 'premultiplied'
        if registration:
            asset['uv_registration'] = registration
        if job.get('source_rgb_regions'):
            for region in job['source_rgb_regions']:
                bounds = region['source_bbox']
                assert len(bounds) == 4 and all(isinstance(n, int) for n in bounds)
                assert 0 <= bounds[0] < bounds[2] <= original.width
                assert 0 <= bounds[1] < bounds[3] <= original.height
                assert region['reason']
            asset['source_rgb_regions'] = job['source_rgb_regions']
        if job.get('content_replacement'):
            asset['content_replacement'] = job['content_replacement']
            asset['generation'] = 'Built-in image_gen; authorized printed-cover replacement; offline native-size resampling'
        if reuse_id is not None:
            asset['generation_reuse'] = {'resource_id': reuse_id,
                'reason': 'Exactly identical source RGBA, dimensions, alpha encoding and atlas layout'}
        if item['alpha_extrema'][0] < 255:
            asset['native_alpha_reference'] = item['original_png']
        manifest['textures'] = [x for x in manifest['textures'] if x['id'] != id] + [asset]
        item['status'] = 'generated-pending-validation'
        known.add(id)
        print(id, logical, '->', [n * 4 for n in logical])
    (ROOT / 'sources.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    (ROOT / 'expanded/queue.json').write_text(json.dumps(queue, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--batch', type=Path, required=True)
    parser.add_argument('--replace', action='store_true')
    args = parser.parse_args()
    import_jobs(json.loads(args.batch.read_text(encoding='utf-8')), args.replace)

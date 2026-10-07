"""Reuse reviewed AI results only for exactly identical historical textures.

Run after importing and brightness-matching a reviewed batch. Identical pixels,
dimensions, native alpha encoding and atlas layout are all mandatory. Every
resource keeps its own original, prompt, calibrated image and provenance.
"""
import json
from pathlib import Path
from import_generated import import_jobs
from match_brightness import main as match_brightness

ROOT = Path(__file__).resolve().parent


def reuse():
    queue = json.loads((ROOT/'expanded/queue.json').read_text(encoding='utf-8'))
    manifest = json.loads((ROOT/'sources.json').read_text(encoding='utf-8'))
    assets = {x['id']: x for x in manifest['textures']}
    def key(item):
        return (item.get('source_rgba_sha256'), tuple(item.get('logical_size', [])),
                item['texture']['Type'], json.dumps(item.get('layout'), sort_keys=True))
    donors = {}
    for item in queue:
        asset = assets.get(item['id'])
        if asset and asset.get('generated_png') and asset.get('brightness_calibration'):
            donors.setdefault(key(item), asset)
    jobs = []
    for item in queue:
        donor = donors.get(key(item))
        if item['status'] != 'pending' or item['id'] in assets or not donor:
            continue
        prompt = (ROOT/donor['prompt_file']).read_text(encoding='utf-8').rstrip()
        prompt += (f"\nResource {item['id']} reuses the reviewed generation for "
                   f"{donor['id']}: exact historical RGBA, dimensions, alpha encoding and atlas layout.")
        job = {'id': item['id'], 'generated': str(ROOT/donor['generated_png']),
               'prompt': prompt, 'reuse_source_id': donor['id']}
        registration = donor.get('uv_registration', {}).get('method')
        if registration == 'Alpha bounding-box UV registration':
            job['register_alpha_bbox'] = True
        elif registration == 'RGB occupied bounding-box UV registration':
            job['register_rgb_bbox'] = True
        jobs.append(job)
    if jobs:
        import_jobs(jobs)
        match_brightness({job['id'] for job in jobs})
    print('Exact-source duplicate reuse:', len(jobs))


if __name__ == '__main__':
    reuse()

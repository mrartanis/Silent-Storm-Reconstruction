"""Refresh coverage only from a fully validated native Game pack."""
import argparse
from collections import Counter
from datetime import date
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def update(pack):
    queue = json.loads((ROOT / 'expanded/queue.json').read_text(encoding='utf-8'))
    sources = json.loads((ROOT / 'sources.json').read_text(encoding='utf-8'))
    manifest = json.loads((pack / 'manifest.json').read_text(encoding='utf-8'))
    validation = json.loads((pack / 'validation.json').read_text(encoding='utf-8'))
    ids = {item['id'] for item in sources['textures']}
    assert ids == {item['id'] for item in manifest['textures']} == {item['id'] for item in validation}
    assert all(item['status'] == 'validated' for item in queue if item['id'] in ids)
    assert len(queue) == len({item['id'] for item in queue})
    pending_statuses = {'pending', 'generated-pending-validation', 'pending-visual-review'}
    report = {
        'date': date.today().isoformat(), 'scope_resources': len(queue),
        'full_db_inventory_resources': len(json.loads((ROOT / 'expanded/full-texture-inventory.json').read_text(encoding='utf-8'))),
        'hd_textures': len(ids), 'native_aliases': len(ids) * 2,
        'native_archives': len(manifest['archives']), 'native_bytes': sum(a['bytes'] for a in manifest['archives']),
        'status_counts': dict(Counter(item['status'] for item in queue)),
        'hd_categories': dict(Counter(item.get('category', 'terrain') for item in sources['textures'])),
        'generation_mode': 'Built-in image_gen, offline conversion and numerical brightness matching',
        'book_cover_replacements': [item['id'] for item in sources['textures'] if item.get('content_replacement')],
        'remaining_pending': [item['id'] for item in queue if item['status'] in pending_statuses],
        'fallback_resources': [{'id': item['id'], 'category': item['category'], 'status': item['status'],
                               'reason': item.get('reason'), 'source': item['texture']['SrcName']}
                              for item in queue if item['status'] != 'validated' and item['status'] not in pending_statuses],
        'validation': 'Every native archive hash, alias, dimensions, original mask and brightness passed; see Game/res-hd/validation.json',
    }
    (ROOT / 'expanded/coverage.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print({key: value for key, value in report.items() if key not in ('fallback_resources', 'remaining_pending')})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', type=Path, required=True)
    update(parser.parse_args().pack)

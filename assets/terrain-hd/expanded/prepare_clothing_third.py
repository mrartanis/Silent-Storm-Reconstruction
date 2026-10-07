import hashlib
import json
from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
IDS = [2009, 2013, 2015, 2151, 2155, 2157, 2235, 2236, 2350, 2352, 2364, 2366]
records = json.loads((BASE / 'groups/source-queues/characters-clothing.json').read_text(encoding='utf-8'))
accepted = json.loads((BASE.parent / 'sources.json').read_text(encoding='utf-8'))['textures']
accepted_ids = {int(x['id']) for x in accepted}
queue = {x['id']: x for x in json.loads((BASE / 'queue.json').read_text(encoding='utf-8'))}
selected = [next(x for x in records if x['id'] == id_) for id_ in IDS]
for record in selected:
    id_ = record['id']
    assert id_ not in accepted_ids and id_ != 2011, id_
    assert id_ not in queue or queue[id_]['status'] == 'pending', id_
    assert record['texture']['Type'] == 'Ordinary', id_
    assert record['texture']['SrcName'].replace('\\', '/').split('/')[-1].lower() in ('body.tga', 'legs.tga'), id_
    uses = record['usage']['direct_texture_uses']
    assert uses and all(x['role'] == 'color' and x['consumers'] for x in uses), id_
    image = Image.open(BASE.parent / record['original_png']).convert('RGBA')
    assert list(image.size) == record['logical_size'], id_
    assert image.getchannel('A').getextrema() == (255, 255), id_
    assert hashlib.sha256(image.tobytes()).hexdigest() == record['source_rgba_sha256'] == record['release_rgba_sha256'], id_
    dest = BASE / f'generated/{id_}-enlarged-source.png'
    helper = image.resize((image.width * 4, image.height * 4), Image.Resampling.NEAREST)
    if dest.exists():
        assert Image.open(dest).convert('RGBA').tobytes() == helper.tobytes(), dest
    else:
        helper.save(dest)
(BASE / 'selected-clothing-third.json').write_text(json.dumps(selected, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
print('Selected verified IDs:', IDS)

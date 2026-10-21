import hashlib
import json
import sys
from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
sys.path.insert(0, str(BASE.parent))
from prepare_sources import ReleaseResources, decode_mmp

IDS = [3084,3086,3399,3401,3860,3861,3868,3869,3879,3881,3883,3885]
records = json.loads((BASE/'groups/source-queues/characters-clothing.json').read_text(encoding='utf-8'))
excluded = {x['id'] for x in json.loads((BASE.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
excluded.update(x['id'] for x in json.loads((BASE/'queue.json').read_text(encoding='utf-8')))
for path in BASE.glob('selected-clothing-*.json'):
    if path.name != 'selected-clothing-fifth.json':
        excluded.update(x['id'] for x in json.loads(path.read_text(encoding='utf-8')))
excluded.update([2011,2151,2235,2443,2542,2546])
release = ReleaseResources('G:/SS/lab/baseline/res')
selected, checks = [], []
for id_ in IDS:
    assert id_ not in excluded, id_
    record = next(x for x in records if x['id'] == id_)
    assert record['texture']['Type'] == 'Ordinary'
    assert record['texture']['SrcName'].replace('\\','/').split('/')[-1].lower() in ('body.tga','legs.tga')
    uses = record['usage']['direct_texture_uses']
    assert uses and all(x['role']=='color' and x['consumers'] for x in uses)
    data = (Path('G:/SS/Silent-Storm/Complete/Textures')/str(id_)).read_bytes()
    historical = decode_mmp(data)
    released = decode_mmp(release.read(id_))
    original = Image.open(BASE.parent/record['original_png']).convert('RGBA')
    assert historical.size == released.size == original.size == tuple(record['logical_size'])
    assert historical.tobytes() == released.tobytes() == original.tobytes(), id_
    rgba_hash = hashlib.sha256(original.tobytes()).hexdigest()
    assert rgba_hash == record['source_rgba_sha256'] == record['release_rgba_sha256']
    assert hashlib.sha256(data).hexdigest() == record['source_sha256']
    assert original.getchannel('A').getextrema() == (255,255)
    helper = original.resize((original.width*4,original.height*4),Image.Resampling.NEAREST)
    target = BASE/f'generated/{id_}-enlarged-source.png'
    if target.exists():
        assert Image.open(target).convert('RGBA').tobytes() == helper.tobytes()
    else:
        helper.save(target)
    selected.append(record)
    checks.append({'id':id_,'source_rgba_sha256':rgba_hash,'source_binary_sha256':hashlib.sha256(data).hexdigest(),
                   'logical_size':list(original.size),'alpha_extrema':[255,255],'typed_roles':['color'],
                   'parity':'Actual decoded historical MMP == actual decoded release resource == existing original PNG, byte-for-byte RGBA and dimensions.'})
(BASE/'selected-clothing-fifth.json').write_text(json.dumps(selected,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'source-check-clothing-fifth.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Verified historical/release/original:',IDS)

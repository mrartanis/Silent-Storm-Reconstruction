import json,re,hashlib
from pathlib import Path
from PIL import Image
b=Path(__file__).resolve().parent
allrecords=[]
for p in (b/'groups/source-queues').glob('*.json'):
    rows=json.loads(p.read_text(encoding='utf-8'))
    if isinstance(rows,list):allrecords.extend(x for x in rows if isinstance(x,dict) and 'id' in x)
lookup={r['id']:r for r in allrecords}
ex={x['id'] for x in json.loads((b.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
ex.update(x['id'] for x in json.loads((b/'queue.json').read_text(encoding='utf-8')))
for p in b.glob('selected-*.json'):
    rows=json.loads(p.read_text(encoding='utf-8'))
    if isinstance(rows,list):ex.update(x['id'] for x in rows if isinstance(x,dict) and 'id' in x)
for p in b.glob('review-*.json'):
    j=json.loads(p.read_text(encoding='utf-8'))
    if not isinstance(j,dict):continue
    for k in ['pending_ids','held_ids','source_only_ids','source_only_deferred_ids']:
        if isinstance(j.get(k),list):ex.update(x for x in j[k] if isinstance(x,int))
    for k in ['source_only_records','source_only_deferred']:
        if isinstance(j.get(k),list):ex.update(x['id'] for x in j[k] if isinstance(x,dict) and isinstance(x.get('id'),int))
for p in (b/'generated').rglob('*.png'):
    if 'raw' in p.name:
        m=re.match(r'(\d+)-',p.name)
        if m:ex.add(int(m[1]))
ex.update([597,1089])
hashes={lookup[i]['source_rgba_sha256'] for i in ex if i in lookup and lookup[i].get('source_rgba_sha256')}
for i in ex:
    p=b/f'original/{i}.png'
    if p.exists():hashes.add(hashlib.sha256(Image.open(p).convert('RGBA').tobytes()).hexdigest())
records=[r for r in allrecords if r['category'] in ['effects','clues','final']]
remaining=[r for r in records if r['id'] not in ex and r.get('source_rgba_sha256') not in hashes]
(b/'audit-effects-fifteenth-remaining.json').write_text(json.dumps(remaining,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(b/'exclusions-effects-fifteenth.json').write_text(json.dumps({'rule':'accepted/mainqueue/allpriorselected/allraw/allpriorpending/held/source-only reviews and same native sourceRGBA; no stopped-family retry','excluded_texture_ids':sorted(ex),'excluded_source_RGBA_sha256':sorted(hashes),'remaining_ids':[r['id'] for r in remaining],'calls':0},indent=2)+'\n',encoding='utf-8')
print('Truly fresh',len(remaining),[(r['id'],r['texture']['SrcName']) for r in remaining])

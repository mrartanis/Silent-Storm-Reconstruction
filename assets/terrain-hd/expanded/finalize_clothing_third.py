import hashlib
import json
from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
GENERATED = BASE / 'generated'
PREVIEW = ROOT / 'build/hd-clothing-third'
PREVIEW.mkdir(parents=True, exist_ok=True)
selected = json.loads((BASE / 'selected-clothing-third.json').read_text(encoding='utf-8'))
pending = {
    2151: 'The very faint lower-right horizontal outline was interpreted as a complete rounded rectangle. Main belt/straps and diffuse UV are retained, but this possible invented contour needs coordinator review.',
    2235: 'Both calls increase original ribbon repeat frequency: original has five coarse paired gold segments, output approximately nine to ten small pairs. Six shirt buttons retained. No source artwork patches used. Leave pending-visual-review.',
    2350: 'Generator interprets an ambiguous upper junction of the narrow right cloth strip as a large rectangular buckle. The reference has no sufficiently clear outline to authorize this hardware. Existing faded red marks preserved. Leave pending-visual-review.'
}
notes = {
    2009: 'Existing full-body muted camo and original central oval camo island retained; two main buckle assemblies and far-right flaps retained. Fine pattern edges now crisp, original pattern scale retained.',
    2013: 'Single belt buckle, original small marks within buckle area, two lower panels, gray ribbed collars and original strap attachment count retained.',
    2015: 'Three original lower-right rectangular tabs retained, along with narrow ribbed dark accessory and existing plain square left patch.',
    2155: 'Full-source retry removes the three invented belt holes. Plain belt sections retained. Exactly five original central lower buttons and source ribbing retained.',
    2157: 'Three principal cloth sections, horizontal right ribbing and bottom original leather hardware retained.',
    2236: 'Opaque tan panel, olive/charcoal cloth, lower-left paired hardware and bottom leather band retained.',
    2352: 'Original single oval patch and vertical stripe frequency retained; two existing leather buckles at far-right retained.',
    2364: 'Very dark full atlas preserved, with the two original dotted fastener tracks and no added zipper teeth. Track contrast is stronger and needs numerical brightness review, not regeneration.',
    2366: 'Original single square right patch, paired lower-left buckle islands, dark cloth and narrow left leather strip retained.'
}
outputs = {
    2009:'385bb1da-6167-4e6b-854a-1233ad1d46a9',2013:'84191cb9-58dd-406d-87a3-218a2d67c15a',
    2015:'6dfce671-48d7-4d96-bf36-68320dbca8f4',2151:'27355cac-78fc-4bf5-ae5d-88aecd83ee79',
    2155:'6aa4d26b-078a-4ff1-ab6f-b1a74d143b85',2157:'29303845-1271-47e7-be82-9e9149eccd18',
    2235:'f2f0293f-c56c-4a96-be8e-27d835dbda6d',2236:'f1f3e1af-ea89-4491-b49d-d65fedd07b71',
    2350:'5b302e0a-42be-48cd-8de0-07d23613db71',2352:'3be18618-49b6-4683-9cc9-324d8a67ff74',
    2364:'46233b07-029e-407e-b256-b74d46332819',2366:'2a511561-553f-44aa-a9cb-e7043dbe17fd'
}
def portable(path):
    return path.relative_to(ROOT).as_posix()
def hashfile(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
jobs = []
reviews = []
calls = []
for record in selected:
    id_ = record['id']
    raw = GENERATED / f'{id_}-raw.png'
    prompt_path = GENERATED / f'{id_}-prompt.txt'
    calls.append({'id':id_, 'call':'initial', 'tool':'built-in image_gen edit', 'tool_output':f'exec-{outputs[id_]}.png', 'saved_raw':portable(raw), 'prompt_file':portable(prompt_path)})
    if id_ in (2155,2235):
        raw = GENERATED / f'{id_}-retry-raw.png'
        prompt_path = GENERATED / f'{id_}-retry-prompt.txt'
        uuid = {2155:'85e55e07-ee1c-4379-814e-77dbcbc1c0d0',2235:'8a7d963e-71dc-420c-a7ae-a1378e9f8a7b'}[id_]
        calls.append({'id':id_, 'call':'full-source retry', 'tool':'built-in image_gen edit', 'tool_output':f'exec-{uuid}.png', 'saved_raw':portable(raw), 'prompt_file':portable(prompt_path)})
    generated = Image.open(raw).convert('RGBA')
    normalized_size = [x * 4 for x in record['logical_size']]
    normalized = generated.resize(tuple(normalized_size), Image.Resampling.LANCZOS)
    original = Image.open(BASE.parent / record['original_png']).convert('RGBA')
    normalized.putalpha(original.getchannel('A').resize(tuple(normalized_size), Image.Resampling.NEAREST))
    preview = PREVIEW / f'{id_}-normalized.png'
    normalized.save(preview)
    review = {'id':id_, 'status':'pending-visual-review' if id_ in pending else 'ready-for-coordinator-review',
              'source_type':'Ordinary', 'typed_roles':['color'], 'original_size':record['logical_size'],
              'source_match':'Decoded original RGBA hash equals historical/release record; alpha255',
              'raw_png':portable(raw), 'raw_size':list(generated.size), 'raw_sha256':hashfile(raw),
              'private_normalized_preview':portable(preview), 'normalized_size':normalized_size,
              'reference_png':portable(GENERATED/f'{id_}-enlarged-source.png'),
              'reference_recipe':'Full original canvas enlarged exactly4x with nearest-neighbor for edit input only; faithful imagegen output normalized by direct full-canvas Lanczos resize. No crop, UV registration or artistic RGB patch insertion.',
              'review_concern':pending.get(id_), 'visual_notes':notes.get(id_,pending.get(id_)),
              'alpha_restoration':'Original opaque alpha expanded nearest-neighbor; no RGB region restoration',
              'brightness':'Not regenerated or calibrated; coordinator will calibrate numerically.'}
    reviews.append(review)
    if id_ not in pending:
        jobs.append({'id':id_, 'generated':portable(raw), 'prompt':prompt_path.read_text(encoding='utf-8').rstrip('\n'),
                     'reference_png':review['reference_png'], 'reference_recipe':review['reference_recipe']})
report = {'version':1, 'selected_ids':[x['id'] for x in selected], 'ready_ids':[x['id'] for x in jobs],
          'pending_visual_review_ids':list(pending), 'accepted_ids':[], 'tool_calls':calls, 'textures':reviews,
          'rejections':[
              {'id':2155,'raw':'assets/terrain-hd/expanded/generated/rejected/2155-extra-belt-holes-raw.png','reason':'Three invented belt holes; successful full-source retry selected.'},
              {'id':2235,'raw':'assets/terrain-hd/expanded/generated/rejected/2235-ribbon-frequency-raw.png','reason':'Original five-pair ribbon changed into approximately nine small pairs.'},
              {'id':2235,'raw':'assets/terrain-hd/expanded/generated/rejected/2235-ribbon-frequency-retry-raw.png','reason':'Full-source retry still has approximately ten small pairs rather than original five.'}
          ], 'shared_metadata_changed':False, 'artistic_source_rgb_regions_used':False,
          'private_review_complete':False}
(BASE/'jobs-clothing-third.json').write_text(json.dumps(jobs,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
(BASE/'review-clothing-third.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('Ready IDs:',report['ready_ids'],'Pending:',list(pending))

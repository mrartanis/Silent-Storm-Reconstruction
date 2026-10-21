import hashlib
import json
import shutil
from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
GEN = BASE / 'generated'
PREVIEW = ROOT / 'build/hd-clothing-fourth'
PREVIEW.mkdir(parents=True, exist_ok=True)
selected = json.loads((BASE/'selected-clothing-fourth.json').read_text(encoding='utf-8'))
pending = {
    2443:'Original tiny faded belt mark is replaced by a crisp multiple-bar cross-like glyph. Exact symbol fidelity is not established. Leave pending-visual-review.',
    2542:'Original tiny pale belt mark has become an anchor-like recognizable mark with a small circle. Main hardware counts and UV retained, but exact emblem fidelity needs review. Leave pending-visual-review.',
    2546:'Original tiny faint belt mark is interpreted as a large segmented checker cross-like shape. Four original central buttons and red cloth islands retained, but glyph identity not established. Leave pending-visual-review.'
}
notes = {
    2367:'Two original triangular ribbed panels, exact four long webbing straps and original six buckles/frames retained. Opaque black gaps and neck arches retained.',
    2369:'Original long left leather island, paired small buckle islands, upper-center leather band and main two cloth folds retained.',
    2439:'Original four flap buttons, two central visible body buttons and tiny collar button retained; existing six belt holes and diagonal strap fasteners retained, no new pockets.',
    2441:'Original left leather boot including small lace area, separate oval/buckle island, five upper-center ribs and subtle center tab retained.',
    2445:'Original two dark cloth panels, shallow upper seam, tiny upper-right hardware and dark bottom leather strip retained.',
    2544:'Exactly two rectangular patches and two oval ribbed patches retained, with original dotted bottom edging frequency and far-left leather accessory hardware.',
    2548:'Original narrow lacing strip preserves original crossing/eyelet-row count (seven rows) and scale; dark oval ribbing, left strap hardware, small cloth tab and UV retained.',
    3068:'Original four small belt holes verified against magnified source crop; two diagonal straps, tiny under-strap points, original two bottom tabs and bottom-right seam strip retained.',
    3070:'Two cloth panels with original fold patterns, shallow left seam, existing bottom leather islands and black padding retained.'
}
outputs = {
    2367:'f1a9a55b-2789-48d3-9f4f-381134632cd3',2369:'d2172703-6e1b-4966-ac94-3a9186bef3a9',
    2439:'42148d64-83ec-4955-a352-4371c2c3090d',2441:'90b872d2-fe71-46a8-9101-ba822ecb965e',
    2443:'54fce899-f7e5-4793-90ec-31e18399dc17',2445:'636ca65e-6590-42e4-a87f-51304a25da51',
    2542:'0d83356c-9693-4e20-b7bf-5028fbb03254',2544:'0ed36faf-7cf9-4d29-a770-b95a59c27019',
    2546:'ba84619f-f897-4c09-8bd1-7c0cad443ef7',2548:'49abbf1a-86f0-4139-b6ee-b567e94374bf',
    3068:'11a38a2c-3e0a-4f9b-989a-be7416107d75',3070:'5eaf5031-a36f-48ce-ae78-72035a514469'
}
def rel(path):
    return path.relative_to(ROOT).as_posix()
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
jobs, reviews, calls, rejections = [], [], [], []
for source in selected:
    id_ = source['id']
    raw = GEN/f'{id_}-raw.png'
    prompt = GEN/f'{id_}-exact-mark-prompt.txt' if id_ in (2542,2546) else GEN/f'{id_}-prompt.txt'
    image = Image.open(raw).convert('RGBA')
    size = [x*4 for x in source['logical_size']]
    normalized = image.resize(tuple(size),Image.Resampling.LANCZOS)
    original = Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    normalized.putalpha(original.getchannel('A').resize(tuple(size),Image.Resampling.NEAREST))
    preview = PREVIEW/f'{id_}-normalized.png'
    normalized.save(preview)
    saved_raw, saved_prompt = raw, prompt
    if id_ in pending:
        saved_raw = GEN/f'rejected/{id_}-belt-mark-fidelity-raw.png'
        saved_prompt = GEN/f'rejected/{id_}-belt-mark-fidelity-prompt.txt'
        shutil.copy2(raw,saved_raw)
        shutil.copy2(prompt,saved_prompt)
        (GEN/f'rejected/{id_}-belt-mark-fidelity-reason.txt').write_text(pending[id_]+'\n',encoding='utf-8')
        rejections.append({'id':id_,'raw':rel(saved_raw),'prompt_file':rel(saved_prompt),'reason':pending[id_]})
    calls.append({'id':id_,'tool':'built-in image_gen edit','tool_output':f'exec-{outputs[id_]}.png',
                  'saved_raw':rel(saved_raw),'raw_sha256':sha(saved_raw),'prompt_file':rel(saved_prompt),'prompt_sha256':sha(saved_prompt)})
    recipe = 'Original full canvas enlarged exactly4x nearest-neighbor solely for edit input. Final raw is separate built-in imagegen edit output; preview is direct full-canvas Lanczos normalization with original alpha, no UV/bbox registration or artistic RGB restoration.'
    review = {'id':id_,'status':'pending-visual-review' if id_ in pending else 'ready-for-coordinator-review',
              'source_type':'Ordinary','typed_roles':['color'],'source_match':'Decoded original RGBA hash equals historical/release; all alpha255',
              'logical_size':source['logical_size'],'raw_png':rel(raw),'raw_size':list(image.size),'raw_sha256':sha(raw),
              'normalized_size':size,'private_normalized_preview':rel(preview),'reference_png':rel(GEN/f'{id_}-enlarged-source.png'),
              'reference_recipe':recipe,'visual_notes':notes.get(id_,pending.get(id_)),'review_concern':pending.get(id_),
              'brightness':'Not regenerated or calibrated. Coordinator will calibrate numerically.'}
    reviews.append(review)
    if id_ not in pending:
        jobs.append({'id':id_,'generated':rel(raw),'prompt':prompt.read_text(encoding='utf-8').rstrip('\n'),
                     'reference_png':review['reference_png'],'reference_recipe':recipe})
report = {'version':1,'selected_ids':[x['id'] for x in selected],'ready_ids':[x['id'] for x in jobs],
          'pending_visual_review_ids':list(pending),'accepted_ids':[],'tool_calls':calls,'textures':reviews,'rejections':rejections,
          'original_inspection_supports':[{'png':rel(GEN/f'{id_}-belt-mark-original-support.png'),'id':id_,
                                          'source_bbox':[43,38,61,53],'scale':20,'resampling':'nearest',
                                          'role':'Private original mark inspection; not used as generated output'} for id_ in (2443,2542)],
          'shared_metadata_changed':False,'artistic_source_rgb_regions_used':False,'private_review_complete':False,
          'pending_reason':'Artistic exact-symbol fidelity, not service refusal. Ready jobs exclude all pending variants.'}
(BASE/'jobs-clothing-fourth.json').write_text(json.dumps(jobs,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
(BASE/'review-clothing-fourth.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('Ready:',report['ready_ids'],'Pending:',list(pending))

import hashlib
import json
import shutil
from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
GEN = BASE / 'generated'
REPORT = BASE / 'review-clothing-third.json'
report = json.loads(REPORT.read_text(encoding='utf-8'))
additional_calls = [
    (2151,'retry','c938a082-dbf8-4c6e-80d5-737e466e46cb','Fixed horizontal seam but converted original tiny gray Y-junction point into a large round metal rivet.'),
    (2350,'retry','e2a7d2ba-f91c-4271-af0e-0ad5f47e3775',None),
    (2235,'support','19f1036e-b30a-43f4-8e37-bf1e9ce6edbd','Even with original ribbon support, repeat frequency remains increased to approximately eight to nine pairs; source has five.'),
    (2151,'final','b114a061-044e-45d7-8481-147e9f7bb7d3','Fixed seam and reduced Y-junction point but changed original circular top belt emblem into a rectangular frame. Not faithful.')
]
jobs = []
for id_,variant,uuid,reason in additional_calls:
    raw = GEN / f'{id_}-{variant}-raw.png'
    prompt = GEN / f'{id_}-{variant}-prompt.txt'
    report['tool_calls'].append({'id':id_,'call':variant,'tool':'built-in image_gen edit','tool_output':f'exec-{uuid}.png',
                                 'saved_raw':raw.relative_to(ROOT).as_posix(),'prompt_file':prompt.relative_to(ROOT).as_posix()})
    record = next(x for x in report['textures'] if x['id'] == id_)
    generated = Image.open(raw).convert('RGBA')
    normalized = generated.resize(tuple(record['normalized_size']),Image.Resampling.LANCZOS)
    original = Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    normalized.putalpha(original.getchannel('A').resize(tuple(record['normalized_size']),Image.Resampling.NEAREST))
    preview = ROOT/f'build/hd-clothing-third/{id_}-{variant}-normalized.png'
    normalized.save(preview)
    if reason:
        rejected = GEN/f'rejected/{id_}-{variant}-rejected-raw.png'
        shutil.copy2(raw,rejected)
        shutil.copy2(prompt,GEN/f'rejected/{id_}-{variant}-rejected-prompt.txt')
        (GEN/f'rejected/{id_}-{variant}-rejected-reason.txt').write_text(reason+'\n',encoding='utf-8')
        report['rejections'].append({'id':id_,'raw':rejected.relative_to(ROOT).as_posix(),'reason':reason})
        record['review_concern'] = reason
        record['status'] = 'pending-visual-review'
        record['last_attempt_raw'] = raw.relative_to(ROOT).as_posix()
        record['last_attempt_preview'] = preview.relative_to(ROOT).as_posix()
    else:
        record.update({'status':'ready-for-coordinator-review','review_concern':None,'raw_png':raw.relative_to(ROOT).as_posix(),
                       'raw_size':list(generated.size),'raw_sha256':hashlib.sha256(raw.read_bytes()).hexdigest(),
                       'private_normalized_preview':preview.relative_to(ROOT).as_posix(),
                       'visual_notes':'Full-source localized instruction preserves original plain cloth junction with no invented buckle. Existing red stain marks, ribs, silhouettes and UV retained.'})
        jobs.append({'id':id_,'generated':record['raw_png'],'prompt':prompt.read_text(encoding='utf-8').rstrip('\n'),
                     'reference_png':record['reference_png'],'reference_recipe':record['reference_recipe']})
        report['ready_ids'].append(id_)
        report['pending_visual_review_ids'].remove(id_)
report['rejections'].extend([
    {'id':2151,'raw':'assets/terrain-hd/expanded/generated/rejected/2151-invented-contour-raw.png','reason':'Initial call invented rounded rectangular outline under Y strap.'},
    {'id':2350,'raw':'assets/terrain-hd/expanded/generated/rejected/2350-invented-buckle-raw.png','reason':'Initial call invented rectangular buckle at plain cloth junction.'}
])
report['support_recipe_2235'] = {'png':'assets/terrain-hd/expanded/generated/2235-ribbon-original-support.png',
                               'source_bbox':[130,180,152,226],'original_size':[256,256],
                               'resize_scale':12,'resampling':'nearest','role':'Original artistic ribbon reference only; no RGB patch insertion.'}
report['private_review_complete'] = False
REPORT.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
(BASE/'jobs-clothing-third-retries.json').write_text(json.dumps(jobs,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('Additional ready:',[x['id'] for x in jobs],'Pending:',report['pending_visual_review_ids'])

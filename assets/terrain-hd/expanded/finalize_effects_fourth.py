import hashlib,json
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
IDS=[680,682,683,684,685,686,5189,5190]
checks=json.loads((BASE/'source-check-effects-fourth.json').read_text(encoding='utf-8'))
for check in checks:
    check['helper_recipe']['input']=f"assets/terrain-hd/expanded/original/{check['id']}.png"
(BASE/'source-check-effects-fourth.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-fourth.json').read_text(encoding='utf-8'))}
jobs=[];reviews=[]
for id_ in IDS:
    call=json.loads((BASE/f'generated/{id_}-effects-fourth-call.json').read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    exact=prompt.read_text(encoding='utf-8');assert exact==call['exact_prompt']
    jobs.append({'id':id_,'generated':f'assets/terrain-hd/expanded/generated/{id_}-raw.png','prompt':exact})
    if id_<5000:
        note='One continuous soft gray smoke cloud remains at original full-canvas coordinates and scale; original brighter/darker patches retained, finer soft paint shading only. No detached components, flame, rocks or atlas. Native all-zero alpha is intentionally retained for TransparentAdd RGB resource. Native stored-RGB private preview reviewed after pure numeric exposure correction.'
    else:
        note='Original connected snow-powder mass, projections and internal light/dark topology retained. Raw generated alpha has firmer outer boundary; exact original native alpha, Lanczos resampled and native premultiplication from the existing packer, restore original fuzzy edge in stored-RGB private preview. That native preview was reviewed; no ice chunks, stones, detached flakes or new objects. Numeric exposure correction only.'
    reviews.append({'id':id_,'status':'ready-for-coordinator-art-review','call_count':1,
       'review_note':note,'review_concern':('Native alpha0 is intentional; do not replace with generated opaque alpha.' if id_<5000 else 'Judge native premultiplied stored-RGB preview, not raw cutout edge; retain original native alpha.'),
       'source_check':f'assets/terrain-hd/expanded/source-check-effects-fourth.json',
       'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-fourth-call.json',
       'reference_pngs':call['reference_pngs'],'generated':call['generated'],'prompt_file':call['prompt_file'],
       'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
       'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],
       'reference_sha256':{p:hashlib.sha256((BASE.parents[2]/p).read_bytes()).hexdigest() for p in call['reference_pngs']},
       'metrics':metrics[id_],'alpha_pipeline':'Native original RGBA alpha restored by root pipeline; no generated alpha acceptance.',
       'uv_registration':'None. Full source canvas4x. No crop, bbox fitting, repack, rotation or source RGB insertion.'})
(BASE/'jobs-effects-fourth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-fourth.json').write_text(json.dumps({'status':'ready-subset-awaiting-coordinator-review','selected_ids':IDS,
   'ready_ids':IDS,'pending_ids':[],'rejected_ids':[],'tool':'built-in image_gen;8 independent original-derived edits',
   'ownership':'Only independent package artifacts and private build previews changed. Shared metadata, import, calibration, pack and Git are coordinator-owned.',
   'source_parity':'Actual Complete historical native files, release resource RGBA and original PNG exact match. Fresh actual typed ParticleInstances and Particles records reviewed read-only.',
   'records':reviews},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Ready subset:',IDS)

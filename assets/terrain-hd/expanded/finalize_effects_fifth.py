import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[1089,2524,2525,2526,2527,2528,5265,5266]
READY=[2527,2528,5265,5266]
PENDING=[1089,2524,2525,2526]
REJECTED=[2524,2525,2526]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-fifth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-fifth.json').read_text(encoding='utf-8'))}
jobs=[];reviews=[]
for id_ in IDS:
    call=json.loads((BASE/f'generated/{id_}-effects-fifth-call.json').read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    exact=prompt.read_text(encoding='utf-8');assert exact==call['exact_prompt']
    if id_ in READY:
        jobs.append({'id':id_,'generated':f'assets/terrain-hd/expanded/generated/{id_}-raw.png','prompt':exact})
    if id_ in [5265,5266]:
        note='Exact original continuous soft blue-gray powder cloud, major projections, dark cavities and scale preserved. Full raw RGB material and native stored-RGB4x preview reviewed; native alpha0 intentionally preserved. No new rocks, rays, flakes or subclouds.'
        concern='Existing ParticleInstances3182 PlasmaExplode sibling Texture0=4737 has no Textures record; selected binding and actual particle exist, other uses resolve. Fresh all-frame proof records this pre-existing missing sibling. No DB/runtime mutation. Retain native alpha0.'
    elif id_ in [2527,2528]:
        note='Original extremely faint two main brown painted smoke masses remain at exact source UV scale and placement, with tiny original brown marks. Material remains diffuse smoke, no stone shapes or added rays. Full raw and scalar-corrected native stored-RGB4x preview reviewed; no white star-like glints accepted.'
        concern='Very faint tiny source marks remain: coordinator should review their shape/count in private full-canvas original RGB support and native4x stored-RGB preview.2527 retains block-like source dots;2528 softens dots to tiny brown spots. No source artistic RGB insertions.'
    elif id_==1089:
        note='Raw original-derived HD cloud retains faithful source UV footprint, diffuse gray/beige material, dark voids and swirl topology. Existing native Transparent premultiplication plus scalar match_cell calibration gain1.7014 clips central highlights and changes relative tonal structure in private stored-RGB preview. Raw itself was not rejected for geometry; final native material is pending numeric-calibration/art review. Do not regenerate for brightness.'
        concern='Native stored-RGB preview has center highlights approaching native alpha223 whereas original maxRGB is182/179/168. Review private raw/gain/stored-RGB before accepting. Numeric correction may address this; no source RGB artistic restoration and no regeneration solely for brightness.'
    else:
        note='Main source cloud placement, two diffuse masses and scale broadly retained, but original faint brown source flecks become whitish luminous glints. This change remains visible after numerical scalar exposure correction. Candidate rejected for tiny-mark material/fidelity; keep resource pending, not permanently refused.'
        concern='Do not import this candidate: white glints differ from original faint brown painted specks. Rejected raw, exact prompt and call metadata saved unchanged; no repeated attempts or source-RGB artistic substitutions.'
    status='ready-for-coordinator-art-review' if id_ in READY else ('pending-calibration-art-review' if id_==1089 else 'pending-visual-review')
    entry={'id':id_,'status':status,'call_count':1,'review_note':note,'review_concern':concern,
       'source_check':'assets/terrain-hd/expanded/source-check-effects-fifth.json',
       'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-fifth-call.json',
       'reference_pngs':call['reference_pngs'],'generated':call['generated'],'prompt_file':call['prompt_file'],
       'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
       'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],
       'reference_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in call['reference_pngs']},
       'metrics':metrics[id_],'alpha_pipeline':'Native original alpha restored by root pipeline; premultiply only1089 (Transparent); other7 TransparentAdd retain alpha0 useful RGB.',
       'uv_registration':'None. Full source canvas4x. No crop, bbox fitting, repack, rotation or source RGB insertion.',
       'missing_sibling_frames':[u['missing_sibling_texture_slots'] for u in proof[id_]['fresh_typed_usage'] if u['missing_sibling_texture_slots']]}
    if id_ in REJECTED:
        directory=BASE/'rejected/effects-fifth';directory.mkdir(parents=True,exist_ok=True)
        for source,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(BASE/f'generated/{id_}-effects-fifth-call.json','call.json')]:
            target=directory/f'{id_}-attempt1-{suffix}'
            if target.exists():assert target.read_bytes()==source.read_bytes()
            else:shutil.copyfile(source,target)
        reason=directory/f'{id_}-attempt1-reason.json'
        reason.write_text(json.dumps({'id':id_,'candidate_status':'rejected','resource_status':'pending-visual-review','reason':note,'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['rejected_raw']=f'assets/terrain-hd/expanded/rejected/effects-fifth/{id_}-attempt1-raw.png'
        entry['rejected_reason']=f'assets/terrain-hd/expanded/rejected/effects-fifth/{id_}-attempt1-reason.json'
    reviews.append(entry)
(BASE/'jobs-effects-fifth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-fifth.json').write_text(json.dumps({'status':'ready-subset-awaiting-coordinator-review','selected_ids':IDS,
   'ready_ids':READY,'pending_ids':PENDING,'rejected_candidate_ids':REJECTED,'tool':'built-in image_gen;8 independent original-derived edits, one call per ID',
   'ownership':'Only independent package artifacts and private build previews changed. Shared metadata, import, calibration, pack and Git remain coordinator-owned.',
   'source_parity':'Actual Complete historical native files, release resource RGBA and original PNG exact match. Fresh read-only actual ParticleInstances/Particles records, blends, pivots, Wrap and all frame bindings verified. Missing sibling4737 for3182 explicitly recorded.',
   'records':reviews},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Ready subset:',READY,'pending:',PENDING,'rejected candidates:',REJECTED)

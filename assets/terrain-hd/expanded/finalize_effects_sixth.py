import hashlib,json,shutil
from pathlib import Path
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[679,681,891,892,1068,1069,1070,1071,1072]
READY=[679,681,891,892,1068,1069,1070]
PENDING=[1071,1072]
SOURCE_ONLY=[4739,4744,5261]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-sixth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-sixth.json').read_text(encoding='utf-8'))}
jobs=[];reviews=[]
for id_ in GENERATED:
    call=json.loads((BASE/f'generated/{id_}-effects-sixth-call.json').read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    exact=prompt.read_text(encoding='utf-8');assert exact==call['exact_prompt']
    if id_ in READY:jobs.append({'id':id_,'generated':call['generated'],'prompt':exact})
    if id_ in [679,681]:
        note='Original orange painted flame UV mass, every main dark hole/channel, source tips and black gaps retained. Full raw material and native stored-RGB4x reviewed. Plain soft source shading; no added fractal curls or new rays. Numeric brightness preview only; native alpha0 preserved.'
    elif id_ in [891,892]:
        note='One original irregular softly fading yellow/orange painted flash, source interior dark/pale blotches and perimeter at source coordinates retained. No separate puffs, rays, rock shapes or white glints. Full raw and native stored-RGB4x reviewed, original alpha0 preserved.'
    else:
        note='Small source pale yellow/white flash core keeps exact source-scale outline/lobes, pink/orange internal color regions and broad black margins. Source bloom falloff stays soft. Full raw and native stored-RGB4x reviewed; source alpha0 retained. No atlas or UV fitting.'
    concern='Coordinator art/material review remains final; pure scalar QA previews do not change shared calibration.'
    if id_==1069:
        concern='Apparent tiny lower-left ember is genuinely in native source: pixel(26,45) RGB70/21/0, source3x3max70/23/0, candidate3x3max78/28/1 after4x scalar QA. Position/component retained, not invented.'
    if id_ in PENDING:
        note+=' Candidate is held for tiny-fleck material review, not declared ready.'
        concern='Existing very faint red source flecks become relatively brighter orange spots after scalar correction. Counts/positions broadly source-matched, but delicate material intensity/hue not confidently faithful. Example1071 sourcepixel(20,52)=13/0/0 versus candidate3x3max23/5/0;1072 source3x3max at(14,26)=8/0/0 versus14/2/0. No repeated generation for brightness and no artistic source RGB insertions.'
    entry={'id':id_,'status':'ready-for-coordinator-art-review' if id_ in READY else 'pending-visual-review',
       'call_count':1,'review_note':note,'review_concern':concern,
       'source_check':'assets/terrain-hd/expanded/source-check-effects-sixth.json',
       'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-sixth-call.json',
       'reference_pngs':call['reference_pngs'],'generated':call['generated'],'prompt_file':call['prompt_file'],
       'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
       'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],
       'reference_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in call['reference_pngs']},
       'metrics':metrics[id_],'alpha_pipeline':'All9 TransparentAdd useful native RGB, alpha uniformly0 preserved by root packer. Raw opaque helper alpha never replaces source alpha.',
       'uv_registration':'None. Complete square native source canvas4x. No crop/reposition/rotate/repack/source artistic RGB insertion.',
       'missing_sibling_frames':[u['missing_sibling_texture_slots'] for u in proof[id_]['fresh_typed_usage'] if u['missing_sibling_texture_slots']]}
    if 'save_recovery' in call:entry['save_recovery']=call['save_recovery']
    if id_ in PENDING:
        directory=BASE/'rejected/effects-sixth';directory.mkdir(parents=True,exist_ok=True)
        for source,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(BASE/f'generated/{id_}-effects-sixth-call.json','call.json')]:
            target=directory/f'{id_}-attempt1-{suffix}'
            if target.exists():assert target.read_bytes()==source.read_bytes()
            else:shutil.copyfile(source,target)
        (directory/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-material-review','resource_status':'pending-visual-review','reason':concern,'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-sixth/{id_}-attempt1-raw.png'
    reviews.append(entry)
deferred=json.loads((BASE/'source-only-effects-sixth.json').read_text(encoding='utf-8'))
for x in deferred:reviews.append({'id':x['id'],'status':x['status'],'call_count':0,'review_note':x['reason'],'generated':None,'source_check':x['source_check']})
(BASE/'jobs-effects-sixth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-sixth.json').write_text(json.dumps({'status':'ready-subset-awaiting-coordinator-review',
   'selected_ids':GENERATED+SOURCE_ONLY,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':SOURCE_ONLY,
   'tool':'Built-in image_gen9 independent native-RGB source edits, one call each. Three structural light-glow fields source-only with zero calls.',
   'ownership':'Only independent assets/terrain-hd/expanded package artifacts and private PNGs changed. Shared metadata, Git and build not changed.',
   'source_parity':'Fresh actual historical native MMP, release resource RGBA and original PNG exact byte/dimension parity. Actual ParticleInstances/Particles rows, blend/Wrap/pivot/all TextureN bindings freshly reviewed read-only. Full native texture UV with no inferred atlas.',
   'records':reviews},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Ready:',READY,'pending tiny material:',PENDING,'source-only:',SOURCE_ONLY)

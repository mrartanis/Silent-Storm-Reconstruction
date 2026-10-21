import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[602,1073,1074,1075,1076,1077,2329,2330,2519,2520,2521,2522]
READY=[602,1073,1077,2329]
PENDING=[1074,1075,1076,2330,2519,2520,2521,2522]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-ninth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-ninth.json').read_text(encoding='utf-8'))}
detail={x['id']:x for x in json.loads((BASE/'detail-metrics-effects-ninth.json').read_text(encoding='utf-8'))}
notes={
602:'Original fullcanvas soft grey powdered radial dust silhouette retained, same central pale source patch and elongated original lobes/channels/gaps. No extra rock/object, new sharp ray, new puff or decorative curl identified in full raw. HD subtly finer dust texture and sharper existing pale lobe centers require final native same-scale material review.',
1073:'Original compact irregular amber/yellow cloud core, upper-right pale lobe, middle/lower diagonal pale mass and original sparse separated source patches retained in fullcanvas. Original component/black-gap layout close in native4x; no new independent puff/white fleck identified. Soft grain becomes more distinct. Some original weak pixels within the painted core become brighter locally after scalar calibration, requiring coordinator final relative material review.',
1074:'Original dim orange/brown fire core and existing separated spots stay close in UV, but muted little source points and some central dark source patches become more luminous orange/gold. Hold faithful relative material/intensity rather than claiming brightness scalar alone solves this.',
1075:'Original very dim brown diffuse right patch and sparse original central/left flecks stay close in UV, but several existing points become brighter/golden and the weak center develops more distinct hot spots. Hold native material; no repeated call or artistic source-RGB patch.',
1076:'Original nearly-black remnant footprint and sparse points remain close, but some dark source points become warmer/brighter after scalar QA and the diffuse weak right patch is more defined. Hold material pending coordinator review.',
1077:'Original nearly-black sparse brown remnant spots retained at their existing positions with broad untouched black gaps and no new bright core, white dots or extra glow network identified. Private scalar preview preserves very weak brown paint; no weak-source pixel with source max<60 and candidate max-source max>12 in the original-space diagnostic. Fullraw/nativesource visual evidence remains authoritative, not that diagnostic alone.',
2329:'Original connected pale-pink/cream impact cloud, brighter upper/left source areas, right rounded lobe, dark lower-right channel and weak lower extension retained. Fullraw material remains diffuse softly painted without new detached dots, curls or hard outline. Fine paint/edge is more distinct and relative pink/brown shades need coordinator native review.',
2330:'Original asymmetric orange puff and original bright bent right arc follow broad source UV, but the arc/rim becomes more sharply bright and faint lower/left contours develop stronger wispy paint, particularly the lower end of the existing source arc. Hold material/contour intensity; no blanket invented-component claim where weak source RGB already exists.',
2519:'Original connected pale lavender cloud core, brown source periphery, darker core regions and broad margin stay close in UV. Existing thin contour marks/peripheral points become more distinct and some weak brown/lavender edge pixels intensify, creating sharper glints; hold native peripheral material.',
2520:'Original lavender/brown broad cloud and source perimeter retain approximate source UV, but existing faint outer tips/points become brighter/sharper and some very dim brown region expands into stronger orange edge paint. Hold native material, no added rays accepted.',
2521:'Original lavender/brown cloudy core and existing perimeter spikes retain source UV pose, but faint source spikes/points become more prominent with stronger little pale tips and the core/shading contrasts change. Hold native material without regeneration for scalar brightness.',
2522:'Original dim lavender/brown cloud and existing perimeter strokes/points retain broad source footprint. However outer weak source points brighten and source contours sharpen; fullraw has more conspicuous glints after scalar native correction. Hold faithful relative peripheral material.'}
jobs=[];records=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-effects-ninth-call.json'
    call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-effects-ninth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(n>0 and n&(n-1)==0 for n in native.size)
    alpha=original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS)
    assert alpha.tobytes()==native.getchannel('A').tobytes() and alpha.getextrema()==(0,0)
    raw_image=Image.open(raw).convert('RGBA')
    assert raw_image.getchannel('A').getextrema()==(255,255)
    assert call['reference_pngs']==[f'assets/terrain-hd/expanded/generated/{id_}-effects-ninth-source-rgb-support.png']
    helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB')
    assert helper.size==(original.width*8,original.height*8)
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    call.update({'reference_sha256':{x:hashlib.sha256((ROOT/x).read_bytes()).hexdigest() for x in call['reference_pngs']},
        'source_transform_recipe':proof[id_]['helper_recipe'],'source_rgba_sha256':proof[id_]['source_rgba_sha256'],
        'original_source_size':list(original.size),'raw_size':list(raw_image.size),
        'prompt_serialization':'Exact actual built-in prompt argument preserves original CRLF and equals UTF-8 prompt file bytes without trailing newline.',
        'reference_argument_serialization':'Actual reference tool arguments were absolute paths resolved against current repository root; metadata retains exact image identities via portable repo-relative paths with byte SHA256.',
        'reproducible_tool_arguments':{'prompt':exact,'referenced_image_paths':call['reference_pngs'],'transparent_background':False},
        'native_texture_type':'TransparentAdd','native_alpha_extrema':[0,0]})
    call_path.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    if id_ in READY:jobs.append({'id':id_,'generated':call['generated'],'prompt':exact})
    concern=notes[id_]
    if id_ in detail:
        concern+=' Weak-original-pixel examples (original RGB vs calibrated native wholecanvas inverse RGB) saved in detail-metrics-effects-ninth.json. Nonzero source pixels selected; local increases do not alone prove invented components.'
    entry={'id':id_,'status':'ready-for-coordinator-art-review' if id_ in READY else 'pending-material-review','call_count':1,
        'review_note':notes[id_]+' Original and complete native stored-RGB guide privately viewed before call; full raw, original native4x and calibrated storedRGB/native alpha previews checked privately. No UV autoBBox, crop, composite, artistic source-RGB patch, raw alteration or repeated call.',
        'review_concern':concern,'source_check':'assets/terrain-hd/expanded/source-check-effects-ninth.json',
        'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-ninth-call.json',
        'generated':call['generated'],'prompt_file':call['prompt_file'],'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
        'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],
        'source_transform_recipe':proof[id_]['helper_recipe'],'metrics':metrics[id_],
        'detail_metrics':detail.get(id_),
        'source_native4x_rgb_preview':f'assets/terrain-hd/expanded/private-effects-ninth/native4x/{id_}-source-rgb4x-private.png',
        'source_native4x_alpha_preview':f'assets/terrain-hd/expanded/private-effects-ninth/native4x/{id_}-source-alpha4x-private.png',
        'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
        'native_alpha_preview':f'assets/terrain-hd/expanded/private-effects-ninth/native4x/{id_}-calibrated-native-alpha-private.png',
        'alpha_pipeline':'Actual source TransparentAdd alpha0 with useful native RGB. Opaque originalRGB-only helper/raw alpha never substitutes source A. Native preview sourceA4x byte-exact0. No native premultiplication for TransparentAdd. Pure private pad/match numerical calibration only; root final alpha/native calibration/pack responsibility.',
        'uv_registration':'None. Entire raw LANCZOS to exact original4x. No crop/padding/recenter/rotation/fitting/island composite/artistic RGB insertion. Complete native texture binding proved in source checks.',
        'actual_typed_use_count':len(proof[id_]['fresh_typed_usage']),
        'actual_blend_values':proof[id_]['material_evidence']['actual_instance_alpha_blend_values'],
        'actual_Wrap_pairs':proof[id_]['material_evidence']['actual_particle_Wrap_pairs'],
        'missing_sibling_frames':[u['missing_sibling_texture_slots'] for u in proof[id_]['fresh_typed_usage'] if u['missing_sibling_texture_slots']]}
    if id_ in PENDING:
        folder=BASE/'rejected/effects-ninth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-native-material-review','resource_status':'pending-material-review','permanent_refusal':False,'reason':concern,'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-ninth/{id_}-attempt1-raw.png'
    records.append(entry)
(BASE/'jobs-effects-ninth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-ninth.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':GENERATED,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':[],
    'tool':'Built-in imagegen12 separate original-RGB fullcanvas edits, exactly one call per ID. No retries or analytical gradient calls.',
    'ownership':'Only independent expanded package artifacts/private ordinary PNGs changed. Shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh actual historical MMP == release resource == original PNG RGBA bytes/dimensions/hash. Complete actual ParticleInstances TextureN referencing each selected texture compared to all queued pairs. Resolved actual Particles, actual blend/Wrap/pivot/all TextureN records captured; full texture native renderer UV proof. Existing accepted/selected/anyraw/prior held excluded;597/1089 never regenerated.',
    'records':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable effects9 ready',READY,'pending material',PENDING)

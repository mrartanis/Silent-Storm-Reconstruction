import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[1084,1085,2513,2514,2515,2516,2517,2518,1248,1251]
READY=[1084,1085,2513,2514,2515,1248,1251]
PENDING=[2516,2517,2518]
SOURCE_ONLY=[1086,3095]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-eighth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-eighth.json').read_text(encoding='utf-8'))}
notes={
1084:'Original orange Fire01 central mass, upper-left and mid/lower black holes/channels, source forked tips, right wisps and lower-left tongue retained. Plain source fire paint, no new decorative curls identified.',
1085:'Original dim brown diagonal split flame branches, upper/right loop and separate faint left patch retained at source coordinates. Global brightness reduced in private scalar preview, not regenerated. Material remains dark brown and softly painted.',
2513:'Tiny original pale white/lavender connected flash core, exact top/left lobe and muted beige/brown left/bottom patch retained; original very broad black margins and tiny source UV scale preserved.',
2514:'Small original pale white/lavender core, left notch, top and lower-left lobes, original darker lavender right and beige contour paint retained. Exact original broad black margins and soft glow footprint.',
2515:'Original tilted lower-left to upper-right connected pale white/lavender flash, source darker beige/lavender right lobes, top lobe and faint periphery retained. Original broad white/dark internal patch arrangement is preserved, including source bright contour curves already in original.',
2516:'Original connected pale white/lavender core and source darker lavender internal patches/bright source curves retain approximate exact UV pose, but faint peripheral marks are overexpressed.',
2517:'Original core/contour and broad lavender patch layout retained; outer faint brown/gray marks become more pronounced/dense small spots and some original contour wisps become stronger.',
2518:'Original lavender core, muted brown perimeter and its pre-existing outward spikes/marks retain broad UV placement, but some faint spikes/particles become thicker/brighter in native preview.',
1248:'Original compact yellow/orange muzzle flash with exact pale-yellow irregular bright center, source angular tipped outer contour and surrounding soft brown paint retained. No extra separate shape or symmetrical star replacement identified.',
1251:'Original smaller/dimmer yellow/orange muzzle flash with exact source bright core shape, existing tipped contour and broad diffuse brown shading retained. Original faint source peripheral patches exist at those locations; no new independent component identified.'
}
concerns={
2515:'One isolated weak original point warrants coordinator review: source3x3max at logical(58,32)=4/2/2; normalized candidate3x3max12/3/1 after private scalar QA. This is an existing source point, not an invented component. Main core/UV and material remain source-matched; conservative optional hold is available.',
2516:'Held for native material review. Existing weak peripheral source marks intensify and become browner. Examples logical(61,27): source3x3max3/1/1 versus candidate19/8/6; (70,20):3/3/3 versus16/8/7; (65,102):3/2/2 versus12/6/5. No blanket claim of invented point; source marks exist. Scalar calibration already applied privately, so relative peripheral material still differs.',
2517:'Held for native material review. Existing very weak peripheral points are much brighter/browner; examples logical(30,55): source3x3max4/3/3 versus candidate28/16/12; (98,67):4/2/2 versus19/6/4; (69,21):6/2/2 versus26/10/5. Some contour wisps strengthen. Original core remains placed correctly, but faithful soft peripheral material not established.',
2518:'Held for native material review. Original peripheral spikes/points intensify and thicken; examples logical(72,17): source3x3max4/1/3 versus candidate17/4/2; (76,22):18/4/4 versus48/18/7; (65,5):6/5/5 versus16/7/4. Broad lavender core/UV stays close; no new rays accepted and no artistic source RGB insertion.'
}
jobs=[];records=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-effects-eighth-call.json';call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt';exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-effects-eighth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    alpha=original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS)
    assert alpha.tobytes()==native.getchannel('A').tobytes() and alpha.getextrema()==(0,0)
    assert Image.open(raw).convert('RGBA').getchannel('A').getextrema()==(255,255)
    call['reference_sha256']={x:hashlib.sha256((ROOT/x).read_bytes()).hexdigest() for x in call['reference_pngs']}
    call['source_transform_recipe']=proof[id_]['helper_recipe']
    call['prompt_serialization']='Exact actual tool argument, preserving source CRLF and matching UTF-8 prompt file bytes without trailing newline.'
    call_path.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    if id_ in READY:jobs.append({'id':id_,'generated':call['generated'],'prompt':exact})
    note=notes[id_]+' Full original/native-RGB support viewed before call; full raw material and private source/native4x storedRGB reviewed. No autoBBox crop/composite, reposition, invented artistic source insertion, generated alpha replacement or repeated brightness regeneration.'
    concern=concerns.get(id_,'Coordinator final native material review remains necessary; HD source edges/fine paint slightly more distinct than original interpolation. High UV correlation does not establish artistic acceptance by itself.')
    entry={'id':id_,'status':'ready-for-coordinator-art-review' if id_ in READY else 'pending-material-review','call_count':1,
        'review_note':note,'review_concern':concern,'source_check':'assets/terrain-hd/expanded/source-check-effects-eighth.json',
        'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-eighth-call.json','generated':call['generated'],'prompt_file':call['prompt_file'],
        'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],
        'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':proof[id_]['helper_recipe'],
        'metrics':metrics[id_],'source_native4x_rgb_preview':f'assets/terrain-hd/expanded/private-effects-eighth/native4x/{id_}-source-rgb4x-private.png',
        'alpha_pipeline':'All10 generated textures actual TransparentAdd with useful nativeRGB and uniformly0 source alpha. Root preserves original alpha0; opaque helper/raw alpha never substitutes it. Private nativealpha4x exactly matches source alpha4x, pure root pad/match preview only, no premultiplication for this texture type.',
        'uv_registration':'None. Exact complete source canvas4x without crop/recenter/rotation/fitting/repacking/source artistic RGB insertion. Native particle renderer fulltexture binding proof in source checks.',
        'actual_typed_use_count':len(proof[id_]['fresh_typed_usage']),
        'missing_sibling_frames':[u['missing_sibling_texture_slots'] for u in proof[id_]['fresh_typed_usage'] if u['missing_sibling_texture_slots']]}
    if id_ in PENDING:
        folder=BASE/'rejected/effects-eighth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-native-material-review','resource_status':'pending-material-review','reason':concern,'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-eighth/{id_}-attempt1-raw.png'
    records.append(entry)
source_only=[{'id':1086,'status':'source-only-deferred','call_count':0,'generated':None,'reason':'Native frame is a collection of very soft smooth orange light globs with coarse near-analytic falloffs and no resolvable natural painted texture. Exact light-component placement/intensity field is structural; preserve source-only without generator.','source_check':'assets/terrain-hd/expanded/source-check-effects-eighth.json'},
 {'id':3095,'status':'source-only-deferred','call_count':0,'generated':None,'reason':'Smooth white filled vertical rounded/elliptical opacity/light gradient with exact soft outer falloff, no organic paint detail. Actual Transparent variable native alpha retained; structural gradient source-only without generator.','source_check':'assets/terrain-hd/expanded/source-check-effects-eighth.json'}]
records.extend(source_only)
(BASE/'source-only-effects-eighth.json').write_text(json.dumps(source_only,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'jobs-effects-eighth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-eighth.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':GENERATED+SOURCE_ONLY,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':SOURCE_ONLY,
   'tool':'Built-in imagegen10 separate original-RGB fullcanvas edits, one call per ID. Two smooth structural fields source-only without calls.',
   'ownership':'Only independent expanded package artifacts/private PNGs changed. Shared metadata/import/calibration/native build/Git untouched.',
   'source_parity':'Fresh actual historical MMP == actual release resource == original PNG RGBA bytes/dimensions/hash. All actual typed ParticleInstances bindings, resolved Particles definitions, blend/Wrap/pivot/all TextureN frame records captured, full native renderer texture UV proof. Existing accepted/selected/anyraw/prior held excluded;1089 not regenerated.',
   'records':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable ready',READY,'pending material',PENDING,'source-only',SOURCE_ONLY)

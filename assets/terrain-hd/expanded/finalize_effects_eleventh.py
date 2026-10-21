import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[1566,1568,1569,1570,1571,1572,1573,1574,1575,1576]
READY=[1566,1568,1569,1571,1572]
PENDING=[id_ for id_ in GENERATED if id_ not in READY]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-eleventh.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-eleventh.json').read_text(encoding='utf-8'))}
detail={x['id']:x for x in json.loads((BASE/'detail-metrics-effects-eleventh.json').read_text(encoding='utf-8'))}
notes={1566: 'Original one connected torn paper sheet and its source hole/gaps, fullcanvas footprint, left pale fold and right elevated fold retained. Fine plain-gray painted surface and existing fold valleys become more distinct; same original alpha restored. No lettering, new independent island or new broad crease identified. Coordinator final same-scale review of fold contrast and original small source holes required.', 1568: 'Original single broad paper sheet, long pointed left arm, middle downward lobe and higher right fold retained. Source two principal fold valleys stay at their source UV positions. Fine paint/shadow contrast is more distinct but remains plain gray folded paper without new detached pieces. Final native fold/material review required.', 1569: 'Original one compact folded paper sheet in central/upper-right region retained, with source high right pale fold, low-left lobe and middle diagonal fold. Native contour follows original sourceA. Fine paint is stronger and existing fold transition sharper; no new broad rib or separate fragment identified. Final coordinator material review required.', 1570: 'Original one connected thin pointed sheet retained in native sourceA, but raw introduces sharply defined transverse diagonal fold/rib bands along the thin left extension where source had only weak gradual shading. Hold fold/material review: no accepted invention of hard creases or repeated call.', 1571: 'Original one connected thin-left-extended sheet, lower central lobe and high right pale fold retained. Main source diagonal valley and original long taper stay in place; finer neutral-gray paper paint remains plain. Existing fold is more distinct and requires final native material review, with no additional broad fold identified.', 1572: 'Original one broad torn paper sheet retained, with source left edge, downward middle ragged lobes and original high right fold/dark diagonal valley. Fine painted surface and valley contrast strengthen but broad fold layout/source alpha contour stay faithful. Final coordinator native review of edges/material required.', 1573: 'Original source has four alpha-connected components: left upper sliver, lower-left shape, upper-right long sheet and separate dim lower-right part. Raw preserves those broad locations/count, but adds more defined diagonal folded stripes within the faint left pieces and a conspicuous central fold line within the upper-right piece. Hold fold/material review rather than accepting stronger/new creases.', 1574: 'Original three source alpha components and original lower-left hole/right gap retained. Raw/plain-gray paper becomes more sharply folded and introduces or strengthens angular light/dark bands inside the upper-left and lower-left pieces; existing right fold is stronger too. Hold faithful internal fold topology/material review, despite matching source contour and aggregate correlation.', 1575: 'Original three source alpha components and black source gaps retained at their locations. Raw adds conspicuous dark/light fold bands within upper-left sheet and strengthens the lower-left crease. Source original is softly shaded and these sharply articulated small folded lobes require hold for internal material review.', 1576: 'Original three alpha components and original black gaps retained. Raw fine paper has stronger rib-like diagonal shaded bands in the upper-left faint sheet and more sharply divided light/dark lower-left fold. Hold original subtle relative fold material, no artistic RGB patch or retry.'}
jobs=[];records=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-effects-eleventh-call.json'
    call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-effects-eleventh/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(n>0 and n&(n-1)==0 for n in native.size)
    alpha=original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS)
    assert alpha.tobytes()==native.getchannel('A').tobytes()
    raw_image=Image.open(raw).convert('RGBA')
    assert raw_image.getchannel('A').getextrema()==(255,255)
    assert call['reference_pngs']==[f'assets/terrain-hd/expanded/generated/{id_}-effects-eleventh-source-rgb-support.png']
    helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB')
    assert helper.size==(original.width*8,original.height*8)
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    refs={x:hashlib.sha256((ROOT/x).read_bytes()).hexdigest() for x in call['reference_pngs']}
    assert refs==call['reference_sha256']
    native_type=proof[id_]['material_evidence']['source_native_type']
    call.update({'source_transform_recipe':proof[id_]['helper_recipe'],'original_source_size':list(original.size),'raw_size':list(raw_image.size),
        'prompt_serialization':'Exact built-in prompt argument preserves CRLF and equals UTF-8 prompt file bytes without trailing newline.',
        'reference_argument_serialization':'Actual reference tool arguments used absolute repository paths. Portable metadata names exact reference PNG identities with unchanged byte SHA256.',
        'reproducible_tool_arguments':{'prompt':exact,'referenced_image_paths':call['reference_pngs'],'transparent_background':False},
        'native_texture_type':native_type,'native_alpha_extrema':proof[id_]['alpha_extrema']})
    call_path.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    if id_ in READY:jobs.append({'id':id_,'generated':call['generated'],'prompt':exact,'source_transform_recipe':proof[id_]['helper_recipe']})
    entry={'id':id_,'status':'ready-for-coordinator-art-review' if id_ in READY else 'pending-material-review','call_count':1,
        'review_note':notes[id_]+' Original native RGBA and complete opaque storedRGB source-only guide privately viewed before call; fullraw and source/native4x storedRGB and nativealpha previews privately reviewed. No artistic source-RGB patch or repeated call.',
        'review_concern':notes[id_], 'source_check':'assets/terrain-hd/expanded/source-check-effects-eleventh.json',
        'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-eleventh-call.json',
        'generated':call['generated'],'prompt_file':call['prompt_file'],'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
        'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],
        'source_transform_recipe':proof[id_]['helper_recipe'],'metrics':metrics[id_],'detail_metrics':detail[id_],
        'source_native4x_rgb_preview':f'assets/terrain-hd/expanded/private-effects-eleventh/native4x/{id_}-source-rgb-native4x-private.png',
        'source_native4x_rgba_preview':f'assets/terrain-hd/expanded/private-effects-eleventh/native4x/{id_}-source-rgba-native4x-private.png',
        'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
        'native_alpha_preview':f'assets/terrain-hd/expanded/private-effects-eleventh/native4x/{id_}-calibrated-native-alpha-private.png',
        'alpha_pipeline':f'Actual source Type {native_type}, alpha extrema {proof[id_]["alpha_extrema"]}. Native sourceA4x restored byte-exact. Opaque storedRGB helper/raw never replaces native source A. Source RGB not unpremultiplied; native premultiplication only for Type Transparent. Private pad/match pure scalar calibration; root final native alpha/calibration/pack responsibility.',
        'uv_registration':'None. ENTIRE raw LANCZOS to exact original4x. No crop/padding/recenter/rotation/fitting/island composite/artistic RGB insertion. Complete native texture renderer and actual all TextureN bindings proved in source checks.',
        'actual_typed_use_count':len(proof[id_]['fresh_typed_usage']),'material_evidence':proof[id_]['material_evidence']}
    if id_ in PENDING:
        folder=BASE/'rejected/effects-eleventh';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-native-material-review','resource_status':'pending-material-review','permanent_refusal':False,'reason':notes[id_],'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-eleventh/{id_}-attempt1-raw.png'
    records.append(entry)
(BASE/'jobs-effects-eleventh.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-eleventh.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':GENERATED,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':[],
    'tool':'Built-in imagegen10 separate wholecanvas original storedRGB-only edits, exactly one call per ID; all raw saved immediately. No retries or analytical gradient calls.',
    'ownership':'Only independent expanded package artifacts/private ordinary PNGs changed. Shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh actual historical MMP == release resource == original PNG RGBA bytes/dimensions/hash. Complete actual ParticleInstances TextureN referencing each selected texture compared to queued pairs. All selected actual Particles resolved, actual blend/Wrap/pivot/all TextureN and sibling texture records captured; full native renderer UV proof. Accepted/selected/anyraw/prior held excluded;597/1089 never regenerated.',
    'ready_semantics':'Ready subset means candidate for coordinator final native material review, never an accepted/packed texture. Other candidates explicitly pending material review, not permanent refusal.',
    'records':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable effects11 ready',READY,'pending material',PENDING)

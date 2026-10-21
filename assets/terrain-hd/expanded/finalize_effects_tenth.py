import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[710,711,712,713,714,716,899,900,2530,5735]
READY=[714,2530]
PENDING=[id_ for id_ in GENERATED if id_ not in READY]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-tenth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-tenth.json').read_text(encoding='utf-8'))}
detail={x['id']:x for x in json.loads((BASE/'detail-metrics-effects-tenth.json').read_text(encoding='utf-8'))}
notes={
710:'Broad original multiple grey separated puff/fragment positions and black gaps retained, but original soft dim gray cores develop brighter, more defined white knots and fine cauliflower-like cloudy microcontrast. Hold native storedRGB material: native mean scalar preserves aggregate brightness but concentrates brightness inside the source alpha islands.',
711:'Original dispersed gray source cluster footprint retained, but the separate puff centers become brighter and cloudier/granular rather than faint soft fragmented paint. Hold native relative core contrast and material; no blanket claim of added components where weak source RGB already exists.',
712:'Original gray fragments and weak smoke spots stay close in UV. Native premultiplied scalar QA produces much brighter compact cores and stronger granular/cloudy lobes inside original alpha islands. Hold native soft fragment material, not a brightness-only retry.',
713:'Original late faint gray dispersed fragment footprint retained. Native scalar/premultiplication yields more distinct bright knots in existing islands and stronger local contrast than muted source. Hold native material.',
714:'Original connected diffuse yellow/orange flame head, upper orange tip, original left/right pale hot regions, dark internal markings and narrow downward orange tail retained in fullcanvas. Fine paint remains soft in fullraw; no new separate solid rock, ray network or decorative curl identified. Existing upper orange tip, selected upper edge and lower tail are locally brighter/sharper after scalar correction. Coordinator final same-scale material review required: source logical30,5 RGB126/66/31 becomes220/111/37; relative hotspot intensity is a review concern, not proof of an invented point.',
716:'Original three stronger brown source fragment/puff locations and surrounding faint spots broadly retained. Existing puff centers become richer golden highlights and finer-cloudy textured lobes compared with low-contrast brown original. Hold native material, preserving uncertain candidate and provenance.',
899:'Original numerous very faint brown fragments remain in their broad source UV positions. Native source-alpha/scalar preview strongly brightens compact golden cores compared with muted soft source fragments; original faint outlines develop more distinct cloudy/lacey detail. Hold native material and relative intensity.',
900:'Original weak brown dispersed fragment layout broadly retained, but existing small centers become brighter golden flecks and cloudier/granular knots. Hold native material; original soft brown components should remain subdued after correction.',
2530:'Original connected asymmetric orange/brown/pink diffuse cloud, bright upper-left source regions, smaller upper-right orange patch, central pink area and darker channels/low extensions retained. Fullraw remains diffuse painted smoke without new detached solid object, new ray network or hard carved curl identified. Fine paint adds cloudy microcontrast and some existing core/perimeter spots become brighter; source logical26,19 RGB165/89/51 becomes225/110/64. Coordinator final same-scale native material review required, especially source-relative brown/pink intensity.',
5735:'Original compact central red-brown irregular fragment/ember silhouette and major internal gaps retained. Fullraw adds rougher lace-like granular material and more conspicuous pink/olive highlights within original regions. Hold native relative material despite good aggregate spatial correlation.'}
jobs=[];records=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-effects-tenth-call.json'
    call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-effects-tenth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(n>0 and n&(n-1)==0 for n in native.size)
    alpha=original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS)
    assert alpha.tobytes()==native.getchannel('A').tobytes()
    raw_image=Image.open(raw).convert('RGBA')
    assert raw_image.getchannel('A').getextrema()==(255,255)
    assert call['reference_pngs']==[f'assets/terrain-hd/expanded/generated/{id_}-effects-tenth-source-rgb-support.png']
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
        'review_concern':notes[id_], 'source_check':'assets/terrain-hd/expanded/source-check-effects-tenth.json',
        'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-tenth-call.json',
        'generated':call['generated'],'prompt_file':call['prompt_file'],'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
        'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],
        'source_transform_recipe':proof[id_]['helper_recipe'],'metrics':metrics[id_],'detail_metrics':detail[id_],
        'source_native4x_rgb_preview':f'assets/terrain-hd/expanded/private-effects-tenth/native4x/{id_}-source-rgb-native4x-private.png',
        'source_native4x_rgba_preview':f'assets/terrain-hd/expanded/private-effects-tenth/native4x/{id_}-source-rgba-native4x-private.png',
        'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
        'native_alpha_preview':f'assets/terrain-hd/expanded/private-effects-tenth/native4x/{id_}-calibrated-native-alpha-private.png',
        'alpha_pipeline':f'Actual source Type {native_type}, alpha extrema {proof[id_]["alpha_extrema"]}. Native sourceA4x restored byte-exact. Opaque storedRGB helper/raw never replaces native source A. Source RGB not unpremultiplied; native premultiplication only for Type Transparent. Private pad/match pure scalar calibration; root final native alpha/calibration/pack responsibility.',
        'uv_registration':'None. ENTIRE raw LANCZOS to exact original4x. No crop/padding/recenter/rotation/fitting/island composite/artistic RGB insertion. Complete native texture renderer and actual all TextureN bindings proved in source checks.',
        'actual_typed_use_count':len(proof[id_]['fresh_typed_usage']),'material_evidence':proof[id_]['material_evidence']}
    if id_ in PENDING:
        folder=BASE/'rejected/effects-tenth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-native-material-review','resource_status':'pending-material-review','permanent_refusal':False,'reason':notes[id_],'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-tenth/{id_}-attempt1-raw.png'
    records.append(entry)
(BASE/'jobs-effects-tenth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-tenth.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':GENERATED,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':[],
    'tool':'Built-in imagegen10 separate wholecanvas original storedRGB-only edits, exactly one call per ID; all raw saved immediately. No retries or analytical gradient calls.',
    'ownership':'Only independent expanded package artifacts/private ordinary PNGs changed. Shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh actual historical MMP == release resource == original PNG RGBA bytes/dimensions/hash. Complete actual ParticleInstances TextureN referencing each selected texture compared to queued pairs. All selected actual Particles resolved, actual blend/Wrap/pivot/all TextureN and sibling texture records captured; full native renderer UV proof. Accepted/selected/anyraw/prior held excluded;597/1089 never regenerated.',
    'ready_semantics':'Ready subset means candidate for coordinator final native material review, never an accepted/packed texture. Other candidates explicitly pending material review, not permanent refusal.',
    'records':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable effects10 ready',READY,'pending material',PENDING)

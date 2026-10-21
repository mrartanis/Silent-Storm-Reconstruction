import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[1617,1618,1619,2120,2121]
READY=[1619,2120,2121]
PENDING=[id_ for id_ in GENERATED if id_ not in READY]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-twelfth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-twelfth.json').read_text(encoding='utf-8'))}
detail={x['id']:x for x in json.loads((BASE/'detail-metrics-effects-twelfth.json').read_text(encoding='utf-8'))}
notes={1617: 'Original glass fragment retains broad footprint but fullraw copies a coarse reference pixel/mosaic grid, with strongly brightened areas in dim source perimeter and lower section. Source logical16,27 RGB1/1/1 becomes98/98/98 after scalar native calibration. Hold source pattern-scale/material/peripheral contrast; no second call or raw patch.', 1618: 'Original triangular gray glass footprint and dark central region remain approximate, but fullraw copies a coarse reference pixel/mosaic grid and strengthens original dim top/rim pixels; source18,6 RGB23/23/24 becomes111/110/109. Hold grid/material/relative rim contrast, no new-generation retry.', 1619: 'Original single diagonally slanted muted gray painted glass fragment retained, with weaker upper-left taper, source pale lower-right area, dark elongated inner shade and surrounding black margins. Fine soft diffuse paint replaces reference blockiness; no new shard, crack, prism, sharp glint/ray or hard rim identified. Original lower-right/inner paint becomes more textured and some existing regions brighter (source19,17 RGB76/76/76 becomes108/107/107). Coordinator final native same-scale gray glass material review required.', 2120: 'Original irregular pale cool-gray/blue feather diffuse group retained, with broad upper-right and middle-left lobes, original dark internal channels and narrow lower-left extensions. Native sourceA restores exact source silhouette and original tiny A-only marks. Fullraw adds finer directional softly brushed/fuzzy paint inside original silhouettes without a new shaft/quill/spine or new detached feather identified. Existing pale cores strengthen during native scalar/premultiply QA: source10,11 RGB165/166/182 becomes225/229/252; fine directional paint and core contrast require coordinator final same-scale material review.', 2121: 'Original softly painted gray-blue central feather group, bright middle-left region, pale diagonal middle/right area, original lower dim extension and existing faint peripheral remnants retained. Fullraw is soft diffuse fine paint with no new shaft/quill/barbed fan or independent feather identified; native originalA restored exactly. Existing core becomes whiter/stronger in private scalar/native correction (source10,17 RGB167/178/193 becomes238/239/238). Coordinator final same-scale native color/core contrast review required; source weakmax<60 localgain>12 diagnostic count0, not sole acceptance proof.'}
jobs=[];records=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-effects-twelfth-call.json'
    call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-effects-twelfth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(n>0 and n&(n-1)==0 for n in native.size)
    alpha=original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS)
    assert alpha.tobytes()==native.getchannel('A').tobytes()
    raw_image=Image.open(raw).convert('RGBA')
    assert raw_image.getchannel('A').getextrema()==(255,255)
    assert call['reference_pngs']==[f'assets/terrain-hd/expanded/generated/{id_}-effects-twelfth-source-rgb-support.png']
    helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB')
    assert helper.size==tuple(proof[id_]['helper_recipe']['helper_size'])
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
        'review_concern':notes[id_], 'source_check':'assets/terrain-hd/expanded/source-check-effects-twelfth.json',
        'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-twelfth-call.json',
        'generated':call['generated'],'prompt_file':call['prompt_file'],'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
        'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],
        'source_transform_recipe':proof[id_]['helper_recipe'],'metrics':metrics[id_],'detail_metrics':detail[id_],
        'source_native4x_rgb_preview':f'assets/terrain-hd/expanded/private-effects-twelfth/native4x/{id_}-source-rgb-native4x-private.png',
        'source_native4x_rgba_preview':f'assets/terrain-hd/expanded/private-effects-twelfth/native4x/{id_}-source-rgba-native4x-private.png',
        'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
        'native_alpha_preview':f'assets/terrain-hd/expanded/private-effects-twelfth/native4x/{id_}-calibrated-native-alpha-private.png',
        'alpha_pipeline':f'Actual source Type {native_type}, alpha extrema {proof[id_]["alpha_extrema"]}. Native sourceA4x restored byte-exact. Opaque storedRGB helper/raw never replaces native source A. Source RGB not unpremultiplied; native premultiplication only for Type Transparent. Private pad/match pure scalar calibration; root final native alpha/calibration/pack responsibility.',
        'uv_registration':'None. ENTIRE raw LANCZOS to exact original4x. No crop/padding/recenter/rotation/fitting/island composite/artistic RGB insertion. Complete native texture renderer and actual all TextureN bindings proved in source checks.',
        'actual_typed_use_count':len(proof[id_]['fresh_typed_usage']),'material_evidence':proof[id_]['material_evidence']}
    if id_ in PENDING:
        folder=BASE/'rejected/effects-twelfth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-native-material-review','resource_status':'pending-material-review','permanent_refusal':False,'reason':notes[id_],'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-twelfth/{id_}-attempt1-raw.png'
    records.append(entry)
(BASE/'jobs-effects-twelfth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-twelfth.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':[1578,1617,1618,1619,1620,2118,2119,2120,2121,5397],'generated_ids':GENERATED,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':[1578,1620,2118,2119,5397],'source_only_records':json.loads((BASE/'source-only-effects-twelfth.json').read_text(encoding='utf-8')),
    'tool':'Built-in imagegen5 separate wholecanvas original storedRGB-only edits, exactly one call per ID; all raw saved immediately. No retries. Five source-only original candidates intentionally have0calls; no analytic/featureless/sparse/near-held-source generation.',
    'ownership':'Only independent expanded package artifacts/private ordinary PNGs changed. Shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh actual historical MMP == release resource == original PNG RGBA bytes/dimensions/hash. Complete actual ParticleInstances TextureN referencing each selected texture compared to queued pairs. All selected actual Particles resolved, actual blend/Wrap/pivot/all TextureN and sibling texture records captured; full native renderer UV proof. Accepted/selected/anyraw/prior held excluded;597/1089 never regenerated.',
    'ready_semantics':'Ready subset means candidate for coordinator final native material review, never an accepted/packed texture. Other candidates explicitly pending material review, not permanent refusal.',
    'records':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable effects12 ready',READY,'pending material',PENDING)

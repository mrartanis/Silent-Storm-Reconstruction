import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[1995,1996];READY=[1995];PENDING=[1996]
SOURCE_ONLY=[2000,2001,2002,2003,2158,3018,3019,3020,4144,4146]
metrics={x['id']:x for x in json.loads((BASE/'metrics-ui-sixth.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-ui-sixth.json').read_text(encoding='utf-8'))}
jobs=[];reviews=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-ui-sixth-call.json';call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt';exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    src=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-ui-sixth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(428,156)==(src.width*4,src.height*4)
    assert native.getchannel('A').tobytes()==src.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert Image.open(raw).convert('RGBA').getchannel('A').getextrema()==(255,255)
    call['reference_sha256']={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']}
    call['source_transform_recipe']=proof[id_]['helper_recipe']
    call['prompt_serialization']='Exact actual built-in argument, including original CRLF, equals UTF-8 prompt file bytes without trailing newline.'
    call_path.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    if id_ in READY:jobs.append({'id':id_,'generated':call['generated'],'prompt':exact})
    note='Full source107x39 and opaque source-RGB nearest8x support viewed privately before call. Full raw2076x757 and source/native4x428x156 privately viewed. Full UV canvas, exact long rectangular footprint, narrow border/bevel segments/corners and principal original dark wear patch positions retained. No text/icon/hardware or new independent artistic components identified. Original variable alpha restored in private pack-equivalent preview and byte-verified; final alpha/root native calibration remains coordinator responsibility.'
    concern='HD matte gray/brown worn source material has finer, more distinct speckle grain and edge contrast than original interpolation. Main wear patch layout follows source; coordinator final native material review required. No autoBBox registration/composite and no artistic source-RGB fixes.'
    if id_ in PENDING:
        concern='Macro wear/patch positions and rectangular UV retained, but bottom/left bevel becomes materially whiter/shinier after scalar/native alpha preview, and coarse mottled contrast changes to dense finer granules. Held for native material review rather than accepted. Numeric brightness alone may not preserve relative face/bevel appearance. No repeated generation for brightness; candidate raw retained unchanged.'
    entry={'id':id_,'status':'ready-for-coordinator-art-review' if id_ in READY else 'pending-material-review','call_count':1,
       'review_note':note,'review_concern':concern,'source_check':'assets/terrain-hd/expanded/source-check-ui-sixth.json',
       'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-ui-sixth-call.json','generated':call['generated'],'prompt_file':call['prompt_file'],
       'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],
       'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':proof[id_]['helper_recipe'],
       'metrics':metrics[id_],'uv_registration':'None. Raw complete native107:39 landscape canvas normalized4x; no fitting/crop/repacking/bbox composite.',
       'source_4x_rgb_preview':f'assets/terrain-hd/expanded/private-ui-sixth/native4x/{id_}-source-rgb-native4x-private.png',
       'alpha_pipeline':'Actual source Transparent variable alpha0..255; opaque original stored-RGB-only helper/raw alpha never replaces native alpha. Private nativealpha4x exactly source alpha Lanczos4x; pure root pad/match/premultiply preview only. No shared pipeline changes.',
       'typed_actual_chain':f"Actual UIControls{940 if id_==1995 else 939}.Texture0 -> UITextures{345 if id_==1995 else 346}.R_1024x768 -> Textures{id_}. Original control rect107x39/type2/string0 and complete actual resolution/other-state fields preserved in fresh source proof."}
    if id_ in PENDING:
        folder=BASE/'rejected/ui-sixth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':'held-for-native-material-review','resource_status':'pending-material-review','reason':concern,'raw_sha256':call['raw_sha256']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/ui-sixth/{id_}-attempt1-raw.png'
    reviews.append(entry)
reasons={
2000:'Only a small neutral rectangular outline/bevel and flat interior; no painted art to reconstruct. Preserve structural border geometry/variable alpha source-only without generator.',
2001:'Only a long thin neutral rectangular outline/bevel and flat interior, reused by twelve actual UIControls. Text is separate UI StringID, absent from this original. Structural source-only without generator.',
2002:'Only a small flat outline/bevel with rounded corners, reused up/down controls. Preserve exact structural border geometry/alpha source-only without generator.',
2003:'Only a long neutral rectangular outline/bevel and flat interior, labels live separately in actual UIControls StringID. Pure frame geometry source-only without generator.',
2158:'A precise orange selection outline around a smooth brown gradient/flat fill; mathematical selected-state geometry/light field, not painted material. Source-only without generator.',
3018:'ComboBox first-item light cream flat/soft gradient background with exact straight top/side border. Structural strip source-only without generator.',
3019:'ComboBox middle-item light cream flat/soft gradient background with exact side borders and end alpha. Structural strip source-only without generator.',
3020:'ComboBox last-item light cream flat/soft gradient background with exact top/side border. Structural strip source-only without generator.',
4144:'Small folded-corner frame with one precise black left-arrow silhouette and smooth facet gradients, without organic painted material. Geometric icon/frame fields source-only without generator.',
4146:'Small mirrored folded-corner frame with one precise black right-arrow silhouette and smooth facet gradients, without organic painted material. Geometric icon/frame fields source-only without generator.'}
source_only=[{'id':i,'status':'source-only-deferred','call_count':0,'reason':reasons[i],'source_check':'assets/terrain-hd/expanded/source-check-ui-sixth.json','generated':None} for i in SOURCE_ONLY]
reviews.extend(source_only)
(BASE/'source-only-ui-sixth.json').write_text(json.dumps(source_only,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'jobs-ui-sixth.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-ui-sixth.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':GENERATED+SOURCE_ONLY,'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':SOURCE_ONLY,
  'tool':'Built-in imagegen2 independent originalRGB fullcanvas edits,1call each. Ten pure geometric/flat/bevel/gradient fields source-only with no calls.',
  'ownership':'Independent expanded package artifacts/private PNGs only; shared metadata/import/calibration/native build/Git untouched.',
  'source_parity':'Fresh actual historical/release/original RGBA decoded byte/dimension parity and complete actual read-only UIControls->UITextures resolution->Textures records/bindings. No inferred atlases. Existing accepted/selected/all raw and4997/4998/7199/7206 excluded before selection.',
  'records':reviews},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable ready',READY,'pending material',PENDING,'source-only',SOURCE_ONLY)

import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[6658]
READY=[]
PENDING=[6658]
SOURCE_ONLY=[]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(name,data):(BASE/name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
metrics={x['id']:x for x in read('metrics-ui-ninth.json')}
proof={x['id']:x for x in read('source-check-ui-ninth.json')}
selected=read('selected-ui-ninth.json')
types={x['id']:x['texture']['Type'] for x in selected}
notes={6658: 'Full square source canvas/large black top, three main fields, eight left rows (5+3), six right rows (2+4), paired original dark ends, middle one gray field and original paper folds retained. Quiet paper refinement remains faithful. However original central gray ROI[380,430,645,815] is EXACT flatRGB(58,63,61), std0, whereas entire inverse scalar-calibrated native storedRGB mean(54.67,59.16,58.79), std(1.02,1.07,1.13), ranges51..62. Subtle new mottling in a flat placeholder is a material change. Hold pending strict flat-gray review, no retry or artistic sourceRGB patch. detail-proof-ui-ninth-6658.json provides read-only statistics; all alpha bytes retained.'}
jobs=[];reviews=[]
for id_ in GENERATED:
    cp=BASE/f'generated/{id_}-ui-ninth-call.json';call=json.loads(cp.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt';exact=prompt.read_bytes().decode('utf-8')
    assert call['exact_prompt']==exact and sha(prompt)==call['prompt_sha256'] and sha(raw)==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    src=Image.open(BASE/f'original/{id_}.png').convert('RGBA');ri=Image.open(raw).convert('RGBA')
    native=Image.open(BASE/f'private-ui-ninth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(src.width*4,src.height*4)
    assert all(n>0 and n&(n-1)==0 for n in src.size+native.size)
    assert native.getchannel('A').tobytes()==src.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert ri.getchannel('A').getextrema()==(255,255)
    recipe=proof[id_]['helper_recipe'];ref=recipe['output'];helper=Image.open(ROOT/ref).convert('RGB')
    assert call['reference_pngs']==[ref]
    assert helper.size==tuple(recipe['helper_size'])
    assert helper.tobytes()==src.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    if recipe['fullcanvas_affine_prewarp']:assert helper.size==(1536,512)
    assert call['source_transform_recipe']==recipe
    assert call['reference_sha256']=={ref:sha(ROOT/ref)}
    assert call['source_rgba_sha256']==proof[id_]['source_rgba_sha256']
    call.update({'reproducible_tool_arguments':{'prompt':exact,'referenced_image_paths':[ref],'transparent_background':False},
       'reference_argument_serialization':'Actual arguments were absolute paths resolved against this repository root; these portable paths preserve exact reference image identities/byteSHA. Prompt including CRLF and transparent_background are exact unchanged actual arguments.',
       'original_source_size':list(src.size),'raw_size':list(ri.size),'native_texture_type':types[id_],
       'native_alpha_source':'Original source alpha LANCZOS4x, verified byte-identical in private native preview; root final pack responsibility.'})
    cp.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    status='ready-for-coordinator-art-review' if id_ in READY else 'pending-material-review'
    if id_ in READY:jobs.append({'id':id_,'generated':call['generated'],'prompt':exact,'source_transform_recipe':recipe})
    entry={'id':id_,'status':status,'call_count':1,'review_note':notes[id_],'review_concern':notes[id_],
       'generated':call['generated'],'prompt_file':call['prompt_file'],'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-ui-ninth-call.json',
       'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_pngs':[ref],'reference_sha256':call['reference_sha256'],
       'source_check':'assets/terrain-hd/expanded/source-check-ui-ninth.json','source_transform_recipe':recipe,'metrics':metrics[id_],
       'native_texture_type':types[id_],'source_alpha_extrema':proof[id_]['alpha_extrema'],
       'fullraw_QA':'Source original and exact helper viewed before individual built-in call; full raw and entire normalized native4x material/storedRGB privately viewed after save. Alpha independently byte-verified. Correlation diagnostic, not acceptance.',
       'typed_actual_chain':'Every actual UIControls TextureN binding to actual UITexture resolution/Texture record freshly compared to source queue; complete actual records and exact historical/release/original RGBA parity saved in source-check.',
       'native_alpha_preview':f'assets/terrain-hd/expanded/private-ui-ninth/native4x/{id_}-calibrated-native-alpha-private.png',
       'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
       'registration':'Entire raw uniformly normalized to exact original4x, inverse wholecanvas aspect only for source-affine guides. No crop/padding/autobbox/generated composite/UV fit/artistic source-RGB insertion.'}
    if id_ in PENDING:
        folder=BASE/'rejected/ui-ninth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':status,'resource_status':'pending','reason':notes[id_],'raw_sha256':call['raw_sha256'],'permanent_refusal':False},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/ui-ninth/{id_}-attempt1-raw.png'
    reviews.append(entry)
source_only=[]
reviews.extend(source_only)
write('source-only-ui-ninth.json',source_only)
write('jobs-ui-ninth.json',jobs)
write('review-ui-ninth.json',{'status':'stable-no-ready-candidate-pending-flat-gray-review','selected_ids':[x['id'] for x in selected],'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':SOURCE_ONLY,
    'tool':'Built-in imagegen1 separate sourceRGB edit for6658, exactly1 call. Other68 remainingPOT were audited read-only and not generated: excluded text/glyphs/flags/screenshots/complex maps, structural fields, exactheld duplicates. No quota padding.',
    'ownership':'Independent artifacts/private ordinary PNGs only; shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh read-only complete actual UIControls->UITextures resolution->Textures; historical/release/original decoded RGBA bytes/dimensions exact. Actual6658 source1024x1024 and native4x4096x4096 POT; remaining69POT fulltyped/sourceparity audit saved separately. Accepted/selected/raw/held/blocked glyphs/flags/photos excluded before calls.',
    'art_policy':'No source artistic RGB patches or generated composites/autobbox. This single originalRGB source-only nearest2x guide keeps square fullcanvas aspect, ENTIRE raw normalized to original4x without crop/padding. Original alpha root restored; scalar numeric brightness private only. Failed material candidates pending, no retries.',
    'records':reviews,
    'tail_scope_audit':'assets/terrain-hd/expanded/source-only-ui-ninth-tail.json',
    'tail_actual_typed_source_parity':'assets/terrain-hd/expanded/source-check-ui-ninth-tail.json',
    'scope_limit':'10-12 new simple painted POT candidates cannot be filled under current accepted/selected/raw/blocked exclusions. Only6658 remained clearly suitable; its first result is pending flatgray material review. Other sources were not regenerated to fill quota.',
    'detail_proof':'assets/terrain-hd/expanded/detail-proof-ui-ninth-6658.json'})
print('Stable UI9 ready',READY,'pending',PENDING,'source-only',SOURCE_ONLY)
for name in ['jobs-ui-ninth.json','review-ui-ninth.json','metrics-ui-ninth.json','source-check-ui-ninth.json','selected-ui-ninth.json']:
    print(name,sha(BASE/name))

import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[2940,3958,3959,4049,4438,4624,5207,5208,6310,7226,7710]
READY=[2940,3958,3959,4049,4438,7226]
PENDING=[4624,5207,5208,6310,7710]
SOURCE_ONLY=[6666]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(name,data):(BASE/name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
metrics={x['id']:x for x in read('metrics-ui-eighth.json')}
proof={x['id']:x for x in read('source-check-ui-eighth.json')}
selected=read('selected-ui-eighth.json')
types={x['id']:x['texture']['Type'] for x in selected}
notes={
2940:'The complete original narrow paper strip including black top and right blank region, one outer rectangle, upper two separators and one middle horizontal separator retained after wholecanvas affine3:1/inverse. No added row, glyph or fold. Paper fibre/weak stains clearer; coordinator final native material/relative border intensity review required.',
3958:'Full tall canvas and large black top preserved. Exactly six top gray placeholders; upper paired tables five rows each; lower paired tables three rows each; one lower long field and one lower-right gray placeholder retained. Original center fold, lower paper clip/stack and existing yellow close-X remain source forms at source UV. No invented text/photo. Paper grain/stains and edge shading sharper; coordinator final native material review required.',
3959:'Full tall canvas/large black top, six top gray placeholders, one large blank outline, original horizontal fold, paper clip/stack and yellow close-X retained. No added content in blank fields or new mark. Source faint rings/vertical stain resolve more clearly; coordinator final native paper and relative edge material review required.',
4049:'Full square canvas with black margins, one original blank brown outline, original center horizontal fold, source ring stains/vertical mark and irregular stacked paper edges retained. No added text/field/symbol. Paper grain/creases more explicit; coordinator same-scale native material review required.',
4438:'Large black top and bottom regions retained; low wide paper form has exact11 original middle gray squares (6+3+2), original three thin gray fields, left large gray field, right paired five-row tables, three long lower table rows, two rounded gray fields and three black end fields. No added row/font/photo. Very weak extra fine grain may appear in originally plain gray placeholders; coordinator inspect relative field flatness and paper edge intensity before final acceptance.',
4624:'Fullcanvas affine3:1/inverse retains upper black band, one left gray painted/reflection field, right eight small slots plus original narrow three-window stack, upper-right large blank and lower metal strip. No new object/photo was requested. Native result retains broad source gray strokes but resolves them into more individually contrasty painterly bands; metal has coarser pitting and relatively brighter bevels after scalar calibration. Hold material, not accepted.',
5207:'All31 original rectangular black holes retained (4 top-clipped plus27 full), diagnostic source/inverse raw count confirmed; maximum black-interior bbox boundary delta2 logical pixels. Original arrow directions, short/long strokes and lower-left closed field retained in private fullcanvas review. Source quiet grain becomes markedly gritty pitting and bright relative arrow/window bevels even after scalar correction. Hold material. Diagnostic rectangles never used to crop/fit/composite art; details in detail-proof-ui-eighth-5207.json.',
5208:'Fullcanvas affine3:1/inverse preserves upper black region and four original bottom-clipped holes at source UV, no new opening. Source quiet mostly vertical worn grain becomes more contrasty granular/diagonal-looking roughness and stronger relative hole/upper edge highlights. Hold pattern/material scale; numeric brightness alone insufficient. Exact source RGBA duplicates6302/6304/6306/6308 were not separately selected or regenerated.',
6310:'Fullcanvas affine3:1/inverse preserves upper black region and four source bottom-clipped holes. Native source is similar geometry to5208 but different source RGBA; source parity individually proven. Subtle vertical source grain becomes contrasty granular/diagonal roughness with stronger relative edge intensity. Hold pattern/material rather than accepting scalar gain.',
7226:'Full open-book canvas retained: two outlined blank pages, two existing brown rounded-square fields near fold, short top-center source notch and black exterior. No new page/text/icon/fold. Original weak mottled paper is finer with faint fibre visible; coordinator final native pattern/edge review required.',
7710:'Fullcanvas affine3:1/inverse retains upper black band, broad left metal field, upper-right blank, eight right small slots plus three-window stack and long lower strip. Broad source stains/pits remain situated but rough grain and individual pits/bevels become more contrasty/coarse after scalar brightness. Hold material, not accepted.'}
jobs=[];reviews=[]
for id_ in GENERATED:
    cp=BASE/f'generated/{id_}-ui-eighth-call.json';call=json.loads(cp.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt';exact=prompt.read_bytes().decode('utf-8')
    assert call['exact_prompt']==exact and sha(prompt)==call['prompt_sha256'] and sha(raw)==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    src=Image.open(BASE/f'original/{id_}.png').convert('RGBA');ri=Image.open(raw).convert('RGBA')
    native=Image.open(BASE/f'private-ui-eighth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
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
       'generated':call['generated'],'prompt_file':call['prompt_file'],'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-ui-eighth-call.json',
       'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_pngs':[ref],'reference_sha256':call['reference_sha256'],
       'source_check':'assets/terrain-hd/expanded/source-check-ui-eighth.json','source_transform_recipe':recipe,'metrics':metrics[id_],
       'native_texture_type':types[id_],'source_alpha_extrema':proof[id_]['alpha_extrema'],
       'fullraw_QA':'Source original and exact helper viewed before individual built-in call; full raw and entire normalized native4x material/storedRGB privately viewed after save. Alpha independently byte-verified. Correlation diagnostic, not acceptance.',
       'typed_actual_chain':'Every actual UIControls TextureN binding to actual UITexture resolution/Texture record freshly compared to source queue; complete actual records and exact historical/release/original RGBA parity saved in source-check.',
       'native_alpha_preview':f'assets/terrain-hd/expanded/private-ui-eighth/native4x/{id_}-calibrated-native-alpha-private.png',
       'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
       'registration':'Entire raw uniformly normalized to exact original4x, inverse wholecanvas aspect only for source-affine guides. No crop/padding/autobbox/generated composite/UV fit/artistic source-RGB insertion.'}
    if id_ in PENDING:
        folder=BASE/'rejected/ui-eighth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':status,'resource_status':'pending','reason':notes[id_],'raw_sha256':call['raw_sha256'],'permanent_refusal':False},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/ui-eighth/{id_}-attempt1-raw.png'
    reviews.append(entry)
source_only=[{'id':6666,'status':'source-only-deferred','call_count':0,'generated':None,'source_check':'assets/terrain-hd/expanded/source-check-ui-eighth.json','reason':'Only an analytical flat dark rectangle with thin geometric outline on black canvas; no painted material. Exact typed UI and sourceRGBA parity proven, native512x128 POT. Preserve structural source field without invented texture;0 imagegen calls.'}]
reviews.extend(source_only)
write('source-only-ui-eighth.json',source_only)
write('jobs-ui-eighth.json',jobs)
write('review-ui-eighth.json',{'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':[x['id'] for x in selected],'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':SOURCE_ONLY,
    'tool':'Built-in imagegen11 separate originalRGB edits, exactly1 call each. Pure flat structural source6666 deferred with0calls.',
    'ownership':'Independent artifacts/private ordinary PNGs only; shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh read-only complete actual UIControls->UITextures resolution->Textures; historical/release/original decoded RGBA bytes/dimensions exact. All12 native source dimensions and all11 native4x are POT. Accepted/selected/raw/held/blocked glyphs/flags/photos excluded before calls.',
    'art_policy':'No source artistic RGB patches or generated composites/autobbox. Five source-only wholecanvas1536x512 guides invert ENTIRE raw to original4x, no crop/padding. Original alpha root restored; scalar numeric brightness private only. Failed material candidates pending, no retries.',
    'records':reviews})
print('Stable UI8 ready',READY,'pending',PENDING,'source-only',SOURCE_ONLY)
for name in ['jobs-ui-eighth.json','review-ui-eighth.json','metrics-ui-eighth.json','source-check-ui-eighth.json','selected-ui-eighth.json']:
    print(name,sha(BASE/name))

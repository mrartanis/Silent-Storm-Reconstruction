import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
GENERATED=[1994,2018,2031,4882,5111,5743,6673,7115,7116]
READY=[6673,7115,7116]
PENDING=[1994,2018,2031,4882,5111,5743]
SOURCE_ONLY=[1998,4080,6339]
AFFINE=[4882,5743,7115,7116]
metrics={x['id']:x for x in json.loads((BASE/'metrics-ui-seventh.json').read_text(encoding='utf-8'))}
proof_list=json.loads((BASE/'source-check-ui-seventh.json').read_text(encoding='utf-8'))
proof={x['id']:x for x in proof_list}
selected=json.loads((BASE/'selected-ui-seventh.json').read_text(encoding='utf-8'))
types={x['id']:x['texture']['Type'] for x in selected}
transforms={x['id']:x for x in json.loads((BASE/'input-transforms-ui-seventh.json').read_text(encoding='utf-8'))}
notes={
1994:'Complete raw has new white upper/lower letterbox strips: native original4:1 canvas became tool3:1. Direct wholecanvas normalization moves all panel boundaries and slots. Hold UV; raw preserved without crop, bbox fitting or retry. Source panel components are present but that does not repair normalized UV.',
2018:'Complete raw has new white upper/lower letterbox strips: native original8:1 canvas became tool3:1. Direct wholecanvas normalization vertically compresses/repositions the strip and three holes. Hold UV; raw unchanged, no crop/bbox/retry.',
2031:'Complete raw has new white upper/lower letterbox strips: native original8:1 canvas became tool3:1. Direct wholecanvas normalization vertically compresses/repositions the strip and three holes clustered at right. Hold UV; raw unchanged, no crop/bbox/retry.',
4882:'Fullcanvas affine3:1 helper then entire raw inverse original4x retains two original clipped holes and the full black upper source region without new whitespace. However the source fine quiet matte grain becomes more contrasty/coarse pitting and the relative upper/slot-edge highlights materially brighter after scalar calibration. Hold material rather than accepting brightness correction as sufficient.',
5111:'Source38x46 -> native4x152x184 is NPOT, and root reports native packer assertion requires physical power-of-two. Hold runtime dimensions; do not change native UV/size or patch pipeline here. The original two arrowheads and narrow connecting stem are source shapes, not an invented joined arrow: original-space rows22..25 source orange x16..21 match inverse raw. Fine grain and sharper frame/bevel contrast still need coordinator material review. Exact native-sized artistic evidence in detail-proof-ui-seventh-5111.json.',
5743:'Fullcanvas affine3:1 helper then entire raw inverse original4x retains the four original bottom-clipped holes and full original black upper region. However the source quiet matte grain becomes more contrasty/coarse pitting, particularly in the left uninterrupted field, and the relative upper/slot-edge highlights materially brighter after scalar calibration. Hold material; no brightness retry.',
6673:'Complete square source canvas retained, including the existing openings cropped by the TOP canvas edge, the large lower-left black window, plain central mottled metal field and narrow right black window. Existing counts/clipping and boundaries followed; no extra row/cell/hardware or independent marks identified. Fine source speckle/wear is sharper in HD and edge glints are more defined. Coordinator must confirm final native relative face/edge material after numeric calibration.',
7115:'Source-only wholecanvas affine3:1 -> entire raw inverse2048x256 preserved the one existing brown outline, its blank paper field and source black rightmost region. No added frame, text, glyph, fold or decorative curl identified. HD paper fibre/grain is more explicit and tiny source stains may gain contrast; coordinator final same-scale native paper/material review remains required. No crop/BBox/UV fit.',
7116:'Source-only wholecanvas affine3:1 -> entire raw inverse2048x512 preserved the outer brown rectangle plus the ONE existing shallow lower rectangle, blank paper field and source black rightmost region. No extra rectangle, text, glyph, fold or curl identified. Fine paper fibre/grain and faint mottling become clearer; small source stair steps in border shading are retained. Coordinator final native material/border review remains required. No crop/BBox/UV fit.'}
jobs=[];reviews=[]
for id_ in GENERATED:
    call_path=BASE/f'generated/{id_}-ui-seventh-call.json'
    call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    exact=prompt.read_bytes().decode('utf-8')
    assert exact==call['exact_prompt']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(exact.encode('utf-8')).hexdigest()==call['prompt_sha256']
    src=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-ui-seventh/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(src.width*4,src.height*4)
    assert native.getchannel('A').tobytes()==src.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    raw_image=Image.open(raw).convert('RGBA')
    assert raw_image.getchannel('A').getextrema()==(255,255)
    expected=f'assets/terrain-hd/expanded/generated/{id_}-ui-seventh-'+('fullcanvas-affine3to1-support.png' if id_ in AFFINE else 'original-rgb-support.png')
    assert call['reference_pngs']==[expected]
    recipe=transforms[id_] if id_ in AFFINE else proof[id_]['helper_recipe']
    helper=Image.open(ROOT/expected).convert('RGB')
    if id_ in AFFINE:
        assert helper.size==(1536,512)
        assert helper.tobytes()==src.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    call.update({'reproducible_tool_arguments':{'prompt':exact,'referenced_image_paths':call['reference_pngs'],'transparent_background':False},
        'reference_argument_serialization':'Reference arguments were absolute paths resolved against the current repository root; JSON preserves their exact image identities as portable repo-relative paths with byte SHA256. Exact prompt and transparent_background are unchanged actual arguments.',
        'reference_sha256':{r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']},
        'source_transform_recipe':recipe,'source_rgba_sha256':proof[id_]['source_rgba_sha256'],
        'original_source_size':list(src.size),'raw_size':list(raw_image.size),
        'prompt_serialization':'Exact actual built-in argument including original CRLF equals UTF-8 prompt file bytes without trailing newline. Reference paths portable equivalents of actual absolute repo paths.',
        'native_texture_type':types[id_],'native_alpha_source':'Original source alpha LANCZOS4x; checked byte-identical in private native preview. Root final pack responsibility.'})
    call_path.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    if id_ in AFFINE:
        proof[id_]['helper_recipe']=recipe
    if id_ in READY:
        assert all(n>0 and n&(n-1)==0 for n in native.size)
        jobs.append({'id':id_,'generated':call['generated'],'prompt':exact})
    status='ready-for-coordinator-art-review' if id_ in READY else ('pending-runtime-POT' if id_==5111 else 'pending-UV-review' if id_ in [1994,2018,2031] else 'pending-material-review')
    entry={'id':id_,'status':status,'call_count':1,'review_note':notes[id_],
        'review_concern':notes[id_],'generated':call['generated'],'prompt_file':call['prompt_file'],
        'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-ui-seventh-call.json',
        'source_check':'assets/terrain-hd/expanded/source-check-ui-seventh.json',
        'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],
        'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],
        'source_transform_recipe':recipe,'metrics':metrics[id_],
        'native_texture_type':types[id_],'source_alpha_extrema':proof[id_]['alpha_extrema'],
        'fullraw_QA':'Original and helper privately viewed before individual built-in call. Full raw plus fullcanvas normalized4x material/storedRGB privately viewed after save. Alpha restored and byte-verified independently. Correlation is diagnostic, not acceptance.',
        'typed_actual_chain':'All actual UIControls TextureN bindings to this UITexture resolution row were compared against source queue and complete control records saved in fresh proof; source historical/release/original decoded RGBA byte/dimension parity exact.',
        'native_alpha_preview':f'assets/terrain-hd/expanded/private-ui-seventh/native4x/{id_}-calibrated-native-alpha-private.png',
        'source_4x_rgb_preview':f'assets/terrain-hd/expanded/private-ui-seventh/native4x/{id_}-source-rgb-native4x-private.png',
        'stored_native_rgb_preview':metrics[id_]['stored_native_rgb_preview'],
        'registration':'Entire raw directly resized to exact original4x, no crop, rotation, padding, bbox composite, fitting or source artistic RGB insertion. Prewarped cases invert only fullcanvas aspect.'}
    if id_ in PENDING:
        folder=BASE/'rejected/ui-seventh';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(call_path,'call.json')]:
            target=folder/f'{id_}-attempt1-{suffix}'
            assert not target.exists() or target.read_bytes()==f.read_bytes()
            if not target.exists():shutil.copyfile(f,target)
        (folder/f'{id_}-attempt1-reason.json').write_text(json.dumps({'id':id_,'candidate_status':status,'resource_status':'pending','reason':notes[id_],'raw_sha256':call['raw_sha256'],'permanent_refusal':False},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/ui-seventh/{id_}-attempt1-raw.png'
    reviews.append(entry)
reasons={1998:'Only an analytical black fade/variable-alpha mask0..128, exact source UI fade field. No painted art, source-only without imagegen. Original253x127 is also NPOT; preserve source pending root runtime policy.',
4080:'Only flat dark rectangular field, thin geometric outline and one small square outline at left, without painted material. Source-only without imagegen; no invented metal/grain.',
6339:'Only flat dark rectangular field and a thin neutral rectangular outline, without painted material. Source-only without imagegen; exact structural/alpha source field preserved.'}
source_only=[{'id':i,'status':'source-only-deferred','call_count':0,'reason':reasons[i],'source_check':'assets/terrain-hd/expanded/source-check-ui-seventh.json','generated':None} for i in SOURCE_ONLY]
reviews.extend(source_only)
(BASE/'source-check-ui-seventh.json').write_text(json.dumps(proof_list,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'source-only-ui-seventh.json').write_text(json.dumps(source_only,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'jobs-ui-seventh.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-ui-seventh.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-native-review','selected_ids':[x['id'] for x in selected],
    'ready_ids':READY,'pending_ids':PENDING,'source_only_ids':SOURCE_ONLY,
    'tool':'Built-in imagegen9 separate original-source RGB edits, exactly1call each; three mask/flat geometric fields deferred with0calls.',
    'ownership':'Independent package artifacts/private ordinary PNGs only; shared metadata/import/calibration/native build/Git untouched.',
    'source_parity':'Fresh actual historical/release/original decoded RGBA exact bytes/dimensions and read-only full UIControls->UITextures resolution->Textures chain. All accepted/selected/raw and blocked4997/4998/7199/7206 excluded before selection.',
    'art_policy':'No artistic source-RGB patches, generated art composites, autofit or autoBBox. Four remaining extreme-aspect helpers use root-authorized source-only affine wholecanvas3:1 and inverse wholecanvas resize. Initial letterboxed3raw remain pending, not regenerated.',
    'records':reviews},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable UI7 ready',READY,'pending',PENDING,'source-only',SOURCE_ONLY)

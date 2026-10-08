import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[655]
SELECTED=[655,3034,3045]
READY=[]
PENDING=[i for i in IDS if i not in READY]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
proof={r['id']:r for r in read('source-check-equipment-fourteenth.json')}
metrics={r['id']:r for r in read('metrics-equipment-fourteenth.json')}
details={r['id']:r for r in read('detail-metrics-equipment-fourteenth.json')}
notes={655:'Complete original UV canvas preserves large silver blade, olive case field, top-left metal cap and wrapped grip. Original5 grip sections/four internal separators at native rows21/26/31/36 retain exact count in private same-scale detailed view. However fullraw adds coarser tarnished metallic mottling and clearly circular screw-head detail on each side bracket, while source bracket fields contain dark rectangular recesses and light crossbars/corner marks. Original two recess fields/perimeter shading are not sufficiently retained, possible reinterpretation of source fastener/depth detail. Hold exact bracket pattern/material/topology, not accepted. Actual Materials441.TextureID655/Template340/Alphaopaque/AddressModeClamp proven; no actual Models consume Template340, queued consumer list alsoempty. Further runtime dispatch/meshUV unknown, not fabricated. Raw1774x887 keeps exact2:1 canvas, entire inverse512x256 only; sourceA255 byte-exact. Stored native luma spatial correlation0.9741; global scalar gain0.8563. Source114,36 RGB8/8/0→50/47/44, local max+42/weak-source gain count345. No crop/BBox/artRGBpatch or retry.'}

jobs=[];reviews=[]
for i in IDS:
    p=proof[i];raw=BASE/f'generated/{i}-raw.png';prompt=BASE/f'generated/{i}-prompt.txt';cp=BASE/f'generated/{i}-equipment-fourteenth-call.json';call=read(str(cp.relative_to(BASE)))
    exact=prompt.read_bytes().decode('utf-8');assert call['exact_prompt']==exact and not exact.endswith('\n')
    assert '\r\n' in exact and hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    refs={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']};assert refs==call['reference_sha256']
    original=Image.open(BASE/f'original/{i}.png').convert('RGBA');helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB');rawimage=Image.open(raw).convert('RGBA')
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    assert rawimage.getchannel('A').getextrema()==(255,255)
    native=Image.open(BASE/f'private-equipment-fourteenth/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4) and all(n>0 and not n&(n-1) for n in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert not p['material_evidence']['unresolved_particle_definitions'] and not p['material_evidence']['missing_sibling_Texture_rows']
    call.update(original_source_size=list(original.size),raw_size=list(rawimage.size),native_texture_type=p['material_evidence']['actual_source_native_type'],native_alpha_extrema=p['material_evidence']['source_alpha_extrema'],reference_argument_serialization='Actual built-in references absolute current repo paths; these portable identities reproduce the same exact byte SHA256 inputs.')
    save(str(cp.relative_to(BASE)),call)
    p.update(full_original_private_view_before_call=True,full_helper_private_view_before_call=True,full_raw_private_view=True,full_native4x_original_RGBA_RGB_and_candidate_nativeA_storedRGB_private_view=True,imagegen_call_count=1,source_visual_review=notes[i],generated_artifacts=[call['generated']])
    p['source_only_reason']=None
    if i in READY:jobs.append({'id':i,'generated':call['generated'],'prompt':exact,'source_transform_recipe':p['helper_recipe']})
    entry={'id':i,'status':'ready-for-coordinator-art-review' if i in READY else 'pending-native-material-review','call_count':1,'review_note':notes[i],'review_concern':notes[i],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':f'assets/terrain-hd/expanded/generated/{i}-equipment-fourteenth-call.json','reference_pngs':call['reference_pngs'],'reference_sha256':refs,'source_transform_recipe':p['helper_recipe'],'source_check':'assets/terrain-hd/expanded/source-check-equipment-fourteenth.json','metrics':metrics[i],'detail_metrics':details[i],'source_native4x_RGB':f'assets/terrain-hd/expanded/private-equipment-fourteenth/native4x/{i}-source-rgb-native4x-private.png','source_native4x_RGBA':f'assets/terrain-hd/expanded/private-equipment-fourteenth/native4x/{i}-source-rgba-native4x-private.png','candidate_native_alpha':f'assets/terrain-hd/expanded/private-equipment-fourteenth/native4x/{i}-calibrated-native-alpha-private.png','candidate_native_storedRGB':metrics[i]['stored_native_rgb_preview'],'material_evidence':p['material_evidence'],'actual_particle_binding_count':len(p['fresh_typed_usage']),'actual_nonparticle_binding_count':len(p['fresh_nonparticle_usage']),'UV_registration':'None. ENTIREraw→exact original4x wholecanvas LANCZOS. Original sourceA separate. No crop/padding/autobbox/fitting/recenter/rotation/regioncomposite/artRGBpatch. Model mesh UV/overlay dispatch unknown, not inferred; all full resource material roles remain unchanged.'}
    if i in PENDING:
        folder=BASE/'rejected/equipment-fourteenth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            dst=folder/f'{i}-attempt1-{suffix}';assert not dst.exists() or dst.read_bytes()==f.read_bytes()
            if not dst.exists():shutil.copyfile(f,dst)
        save(str((folder/f'{i}-attempt1-reason.json').relative_to(BASE)),{'id':i,'status':'pending-native-material-review','permanent_refusal':False,'reason':notes[i],'raw_sha256':call['raw_sha256'],'call_count':1})
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/equipment-fourteenth/{i}-attempt1-raw.png'
    reviews.append(entry)
save('source-check-equipment-fourteenth.json',[proof[i] for i in SELECTED])
save('jobs-equipment-fourteenth.json',jobs)
tail=read('audit-equipment-fourteenth-readonly-tail.json')
sourceonly=[r for r in tail if r['id'] not in IDS and (r['source_RGB_private_view'] or r['unresolved_particle_ids'])]
save('source-only-equipment-fourteenth.json',sourceonly)
save('review-equipment-fourteenth.json',{'status':'stable-no-ready-pending-bracket-material-review','selected_ids':SELECTED,'generated_ids':IDS,'ready_ids':READY,'pending_ids':PENDING,'call_count':len(IDS),'records':reviews,'audit_correction':'Materials/FinalElements/ContainerModels not mistaken for missing Particle rows. Fresh3candidate audit verifies each actual typed table category independently. Selected equipment has no Particle consumers/definition requirement; category alone never prohibits art generation.','full_remaining_source_audit_count':len(tail),'source_only_ids':[i for i in SELECTED if i not in IDS],'source_only_records':sourceonly,'unreviewed_source_ids':[r['id'] for r in tail if r['id'] not in IDS and not r['source_RGB_private_view'] and not r['unresolved_particle_ids']],'source_parity':'Fresh read-only all actual TextureN ParticleInstance slots and all actual Materials.TextureID/FinalElements.LightFlareTexture/ContainerModels.PLightFlareTexture bindings exact queued sets. Actual all Material model consumers/templates/geometries checked. Historical MMP/release/existing original native RGBA bytes/dims/hash identical. Actual material Alpha/AddressMode enums retained, missing runtime dispatch/meshUV unknown not fabricated.','tool':'Built-in imagegen1 wholecanvas edit, exactly one/ID; source native original and opaqueRGB4xnearest guide privately inspected before every call, immediate raw/exactCRLFprompt/refSHA/callmetadata saves. No retries.','QA':'Complete raw and original4xRGBA/storedRGB/native sourceA-restored scalar/native material privately inspected; quantitative gains/local deltas/count bboxes are diagnostics only, not artistic edits/registration. SourceA4x byte-exact; target512x256 physicalPOT.','ready_semantics':'Candidates awaiting coordinator final native material review; no accepted/packed claims. Held candidates explicitly pending, not permanent refusal.','ownership':'Independent expanded artifacts only. Shared metadata/calibration/import/build/Git and stable effects13/14/15 untouched.'})
files=[p for p in BASE.glob('*equipment-fourteenth*.json')]+[p for p in (BASE/'generated').glob('*equipment-fourteenth-call.json')]
save('sha256-equipment-fourteenth.json',{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if not p.name.startswith('sha256-')})
print('Stable1call equipment package; ready',READY,'pending',PENDING)

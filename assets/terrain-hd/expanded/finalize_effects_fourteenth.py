import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[5276,5277,5278,5279]
SELECTED=[5276,5277,5278,5279,2485,5298,5299,5699,4915,7682]
READY=[]
PENDING=[i for i in IDS if i not in READY]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
proof={r['id']:r for r in read('source-check-effects-fourteenth.json')}
metrics={r['id']:r for r in read('metrics-effects-fourteenth.json')}
details={r['id']:r for r in read('detail-metrics-effects-fourteenth.json')}
notes={
5276:'Fullraw changed original dim irregular flat splatter into coarsely mottled red shapes and rounded spots, with materially different spatial tone/relative component positions; not a faithful fullcanvas source edit. Raw luma spatial correlation0.6916 and calibrated stored native correlation0.8563. Native sourceA components34 retained byte-exact by sourceA restore, but that does not repair altered raw art. Existing source55,56 RGB17/4/2 becomes60/15/13, local max increase43; weak-source<60 localgain>12 count243. Hold source UV/material/count/tonal correspondence, no second call or art RGB patch.',
5277:'Fullraw broad source asymmetric body/upward tails/lower-right lobe positions remain, but weak original peripheral marks become rounded dense painted dots and the original subtle soft red/brown mottling becomes larger coarse mosaic patches with local bright centers. Original flat subdued diffuse pattern scale not sufficiently preserved. Calibrated stored correlation0.9733, original sourceA components35 byte-exact. Existing upper spot60,36 RGB50/20/18 becomes99/35/32 and76,38 RGB20/5/3 becomes61/10/10; weak-source gain count111, max increase49. Hold material/peripheral patch intensity, no retry.',
5278:'Broad source diagonal body/lobes and dark original openings retained, original sourceA components27 byte-exact. Raw introduces larger faceted/mosaic red paint patches and harder rounded peripheral dot boundaries than source subtle fuzzy planar mottling. Current native correction retains only moderate local increases (max18, weak-source gain count13, stored correlation0.9755), but raw source material/pattern scale remains uncertain. Existing source54,76 RGB27/6/4 becomes45/13/11. Explicit pending native-material review, not accepted or permanent refusal; coordinator may independently judge existing raw, no imagegen retry or art patch.',
5279:'Broad source central body/upper-left tail and upper-right/lower-right source patches remain, original sourceA components32 byte-exact. Raw turns muted existing spots and subtle source material into firmer rounded red shapes with larger uniform mosaic mottling; existing weak source patches locally strengthen despite global gain<1. Stored correlation0.9772, source84,38 RGB19/5/3 becomes44/10/8, max increase25, weak-source gain count72. Hold source pattern scale/softness/peripheral tone, no retry or artistic RGB insertion.'}

jobs=[];reviews=[]
for i in IDS:
    p=proof[i];raw=BASE/f'generated/{i}-raw.png';prompt=BASE/f'generated/{i}-prompt.txt';cp=BASE/f'generated/{i}-effects-fourteenth-call.json';call=read(str(cp.relative_to(BASE)))
    exact=prompt.read_bytes().decode('utf-8');assert call['exact_prompt']==exact and not exact.endswith('\n')
    assert '\r\n' in exact and hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    refs={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']};assert refs==call['reference_sha256']
    original=Image.open(BASE/f'original/{i}.png').convert('RGBA');helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB');rawimage=Image.open(raw).convert('RGBA')
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    assert rawimage.getchannel('A').getextrema()==(255,255)
    native=Image.open(BASE/f'private-effects-fourteenth/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4) and all(n>0 and not n&(n-1) for n in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert not p['material_evidence']['unresolved_particle_definitions'] and not p['material_evidence']['missing_sibling_Texture_rows']
    call.update(original_source_size=list(original.size),raw_size=list(rawimage.size),native_texture_type=p['material_evidence']['actual_source_native_type'],native_alpha_extrema=p['material_evidence']['source_alpha_extrema'],reference_argument_serialization='Actual built-in references absolute current repo paths; these portable identities reproduce the same exact byte SHA256 inputs.')
    save(str(cp.relative_to(BASE)),call)
    p.update(full_original_private_view_before_call=True,full_helper_private_view_before_call=True,full_raw_private_view=True,full_native4x_original_RGBA_RGB_and_candidate_nativeA_storedRGB_private_view=True,imagegen_call_count=1,source_visual_review=notes[i],generated_artifacts=[call['generated']])
    p['source_only_reason']=None
    if i in READY:jobs.append({'id':i,'generated':call['generated'],'prompt':exact,'source_transform_recipe':p['helper_recipe']})
    entry={'id':i,'status':'ready-for-coordinator-art-review' if i in READY else 'pending-native-material-review','call_count':1,'review_note':notes[i],'review_concern':notes[i],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':f'assets/terrain-hd/expanded/generated/{i}-effects-fourteenth-call.json','reference_pngs':call['reference_pngs'],'reference_sha256':refs,'source_transform_recipe':p['helper_recipe'],'source_check':'assets/terrain-hd/expanded/source-check-effects-fourteenth.json','metrics':metrics[i],'detail_metrics':details[i],'source_native4x_RGB':f'assets/terrain-hd/expanded/private-effects-fourteenth/native4x/{i}-source-rgb-native4x-private.png','source_native4x_RGBA':f'assets/terrain-hd/expanded/private-effects-fourteenth/native4x/{i}-source-rgba-native4x-private.png','candidate_native_alpha':f'assets/terrain-hd/expanded/private-effects-fourteenth/native4x/{i}-calibrated-native-alpha-private.png','candidate_native_storedRGB':metrics[i]['stored_native_rgb_preview'],'material_evidence':p['material_evidence'],'actual_particle_binding_count':len(p['fresh_typed_usage']),'actual_nonparticle_binding_count':len(p['fresh_nonparticle_usage']),'UV_registration':'None. ENTIREraw→exact original4x wholecanvas LANCZOS. Original sourceA separate. No crop/padding/autobbox/fitting/recenter/rotation/regioncomposite/artRGBpatch. Model mesh UV/overlay dispatch unknown, not inferred; all full resource material roles remain unchanged.'}
    if i in PENDING:
        folder=BASE/'rejected/effects-fourteenth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            dst=folder/f'{i}-attempt1-{suffix}';assert not dst.exists() or dst.read_bytes()==f.read_bytes()
            if not dst.exists():shutil.copyfile(f,dst)
        save(str((folder/f'{i}-attempt1-reason.json').relative_to(BASE)),{'id':i,'status':'pending-native-material-review','permanent_refusal':False,'reason':notes[i],'raw_sha256':call['raw_sha256'],'call_count':1})
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/effects-fourteenth/{i}-attempt1-raw.png'
    reviews.append(entry)
save('source-check-effects-fourteenth.json',[proof[i] for i in SELECTED])
save('jobs-effects-fourteenth.json',jobs)
tail=read('audit-effects-fourteenth-readonly-tail.json')
sourceonly=[r for r in tail if r['id'] not in IDS and (r['source_RGB_private_view'] or r['unresolved_particle_ids'])]
save('source-only-effects-fourteenth.json',sourceonly)
save('review-effects-fourteenth.json',{'status':'stable-no-ready-candidates-pending-native-material-review','selected_ids':SELECTED,'generated_ids':IDS,'ready_ids':READY,'pending_ids':PENDING,'call_count':len(IDS),'records':reviews,'audit_correction':'Materials/FinalElements/ContainerModels not mistaken for missing Particle rows. Fresh81candidate audit verifies each actual typed table category independently. Only28IDs have actual unresolved Particles definitions; category alone never prohibits art generation.','full_remaining_source_audit_count':len(tail),'source_only_ids':[i for i in SELECTED if i not in IDS],'source_only_records':sourceonly,'unreviewed_source_ids':[r['id'] for r in tail if r['id'] not in IDS and not r['source_RGB_private_view'] and not r['unresolved_particle_ids']],'source_parity':'Fresh read-only all actual TextureN ParticleInstance slots and all actual Materials.TextureID/FinalElements.LightFlareTexture/ContainerModels.PLightFlareTexture bindings exact queued sets. Actual all Material model consumers/templates/geometries checked. Historical MMP/release/existing original native RGBA bytes/dims/hash identical. Actual material Alpha/AddressMode enums retained, missing runtime dispatch/meshUV unknown not fabricated.','tool':'Built-in imagegen4 separate wholecanvas edits, exactly one/ID; source native original and opaqueRGB8xnearest guide privately inspected before every call, immediate raw/exactCRLFprompt/refSHA/callmetadata saves. No retries.','QA':'Complete raw and original4xRGBA/storedRGB/native sourceA-restored scalar/native material privately inspected; quantitative gains/local deltas/count bboxes are diagnostics only, not artistic edits/registration. SourceA4x byte-exact; target512square physicalPOT.','ready_semantics':'Candidates awaiting coordinator final native material review; no accepted/packed claims. Held candidates explicitly pending, not permanent refusal.','ownership':'Independent expanded artifacts only. Shared metadata/calibration/import/build/Git and stable13 untouched.'})
files=[p for p in BASE.glob('*effects-fourteenth*.json')]+[p for p in (BASE/'generated').glob('*effects-fourteenth-call.json')]
save('sha256-effects-fourteenth.json',{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if not p.name.startswith('sha256-')})
print('Stable4call package; ready',READY,'pending',PENDING)

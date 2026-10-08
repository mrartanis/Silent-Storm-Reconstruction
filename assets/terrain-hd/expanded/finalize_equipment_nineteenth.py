import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[5330,5332,5381]
SELECTED=[5330,5332,5381]
READY=[]
PENDING=[i for i in IDS if i not in READY]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
proof={r['id']:r for r in read('source-check-equipment-nineteenth.json')}
metrics={r['id']:r for r in read('metrics-equipment-nineteenth.json')}
details={r['id']:r for r in read('detail-metrics-equipment-nineteenth.json')}
notes={5330: 'Complete source128square/sourceRGB NEAREST4x512square/fullraw1254square/native512square originalRGBA/storedRGB+sourceA privately viewed. Major wood fields, one black rectangular window, one ring opening and one vertical metal field preserve normalized fullcanvas locations and broad component count. However irregular mottled painted-metal ring became regular concentric raised rim; fine woodgrain became more regular elongated horizontal striations. Source ring black-core threshold5 area192/bbox[44,25,60,41] versus candidate area176/bbox[44,25,60,40] at native inverse128: this diagnostic suggests one-native-pixel edge change, not registration proof. Vertical metal area gains up to36; weak-source gain531, stored native correlation .969630. Hold native material/pattern/contour review, no accepted claim. SourceA255 byte-exact, physicalPOT512square, wholeinverse only/no source-artRGB/composite/retry.', 5332: 'Complete source128square/sourceRGB NEAREST4x512square/fullraw1254square/native512square originalRGBA/storedRGB+sourceA privately viewed. Wood planes, one ring opening, existing lowerleft bar localized pale left tip and dim upper patch retain major counts and wholecanvas normalized positions. However irregular soft metal ring becomes a regular concentric raised rim; original mottled wood paint becomes uniform horizontal streaks and weak wood regions strengthen (weak-source gain2156). Native inverse ring threshold5 black-core area670/bbox[85,49,114,79] versus area660/bbox[86,49,114,79]: one-native-pixel edge change diagnostic. Stored native correlation .951271/maxlocal+27. Hold native material/pattern review despite broad UV/count retained, no accepted claim. SourceA255 byte-exact/POT512square, fullinverse/no artistic insert/retry.', 5381: 'Complete source128x64/sourceRGB NEAREST4x512x256/fullraw1774x887/native512x256 sourceRGBA/storedRGB+sourceA privately viewed. Dim gray metal palette and broad upper pincer fields/middle and bottom horizontal rectangles remain, without new bright plates/glyph/hardware. Yet upper stepped irregular contour becomes a smoother oval opening and several original small dark internal patches/steps disappear in the rectangular fields. This loses source material silhouette/shadow detail despite stored native correlation .922290/maxlocal+25/weak-source gain17. Hold native contour/detail-material review, no accepted claim. SourceA255 byte-exact/POT512x256, wholeinverse/no artistRGB/BBox/retry.'}

jobs=[];reviews=[]
for i in IDS:
    p=proof[i];raw=BASE/f'generated/{i}-raw.png';prompt=BASE/f'generated/{i}-prompt.txt';cp=BASE/f'generated/{i}-equipment-nineteenth-call.json';call=read(str(cp.relative_to(BASE)))
    exact=prompt.read_bytes().decode('utf-8');assert call['exact_prompt']==exact and not exact.endswith('\n')
    assert '\r\n' in exact and hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    refs={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']};assert refs==call['reference_sha256']
    original=Image.open(BASE/f'original/{i}.png').convert('RGBA');helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB');rawimage=Image.open(raw).convert('RGBA')
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    assert rawimage.getchannel('A').getextrema()==(255,255)
    native=Image.open(BASE/f'private-equipment-nineteenth/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4) and all(n>0 and not n&(n-1) for n in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert not p['material_evidence']['unresolved_particle_definitions'] and not p['material_evidence']['missing_sibling_Texture_rows']
    call.update(original_source_size=list(original.size),raw_size=list(rawimage.size),native_texture_type=p['material_evidence']['actual_source_native_type'],native_alpha_extrema=p['material_evidence']['source_alpha_extrema'],reference_argument_serialization='Actual built-in references absolute current repo paths; these portable identities reproduce the same exact byte SHA256 inputs.')
    save(str(cp.relative_to(BASE)),call)
    p.update(full_original_private_view_before_call=True,full_helper_private_view_before_call=True,full_raw_private_view=True,full_native4x_original_RGBA_RGB_and_candidate_nativeA_storedRGB_private_view=True,imagegen_call_count=1,source_visual_review=notes[i],generated_artifacts=[call['generated']])
    p['source_only_reason']=None
    p['UV_proof'].pop('particle_implementation',None);p['UV_proof'].pop('renderer_sha256',None)
    p['pattern_constraints']=next(x for x in read('pattern-constraints-equipment-nineteenth.json') if x['id']==i)
    if i in READY:jobs.append({'id':i,'generated':call['generated'],'prompt':exact,'source_transform_recipe':p['helper_recipe']})
    entry={'id':i,'status':'ready-for-coordinator-art-review' if i in READY else 'pending-native-material-review','call_count':1,'review_note':notes[i],'review_concern':notes[i],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':f'assets/terrain-hd/expanded/generated/{i}-equipment-nineteenth-call.json','reference_pngs':call['reference_pngs'],'reference_sha256':refs,'source_transform_recipe':p['helper_recipe'],'source_check':'assets/terrain-hd/expanded/source-check-equipment-nineteenth.json','metrics':metrics[i],'detail_metrics':details[i],'source_native4x_RGB':f'assets/terrain-hd/expanded/private-equipment-nineteenth/native4x/{i}-source-rgb-native4x-private.png','source_native4x_RGBA':f'assets/terrain-hd/expanded/private-equipment-nineteenth/native4x/{i}-source-rgba-native4x-private.png','candidate_native_alpha':f'assets/terrain-hd/expanded/private-equipment-nineteenth/native4x/{i}-calibrated-native-alpha-private.png','candidate_native_storedRGB':metrics[i]['stored_native_rgb_preview'],'material_evidence':p['material_evidence'],'actual_particle_binding_count':len(p['fresh_typed_usage']),'actual_nonparticle_binding_count':len(p['fresh_nonparticle_usage']),'UV_registration':'None. ENTIREraw→exact original4x wholecanvas LANCZOS. Original sourceA separate. No crop/padding/autobbox/fitting/recenter/rotation/regioncomposite/artRGBpatch. Model mesh UV/overlay dispatch unknown, not inferred; all full resource material roles remain unchanged.'}
    entry['pattern_constraints']=p['pattern_constraints']
    entry['pattern_metrics']=next(x for x in read('pattern-metrics-equipment-nineteenth.json') if x['id']==i)
    if i in PENDING:
        folder=BASE/'rejected/equipment-nineteenth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            dst=folder/f'{i}-attempt1-{suffix}';assert not dst.exists() or dst.read_bytes()==f.read_bytes()
            if not dst.exists():shutil.copyfile(f,dst)
        save(str((folder/f'{i}-attempt1-reason.json').relative_to(BASE)),{'id':i,'status':'pending-native-material-review','permanent_refusal':False,'reason':notes[i],'raw_sha256':call['raw_sha256'],'call_count':1})
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/equipment-nineteenth/{i}-attempt1-raw.png'
    reviews.append(entry)
save('source-check-equipment-nineteenth.json',[proof[i] for i in SELECTED])
save('jobs-equipment-nineteenth.json',jobs)
tail=read('audit-equipment-nineteenth-readonly-tail.json')
sourceonly=[r for r in tail if r['id'] not in IDS and (r['source_RGB_private_view'] or r['unresolved_particle_ids'])]
save('source-only-equipment-nineteenth.json',sourceonly)
save('review-equipment-nineteenth.json',{'status':'stable-no-ready-subset-pending-native-material-and-contours','selected_ids':SELECTED,'generated_ids':IDS,'ready_ids':READY,'pending_ids':PENDING,'call_count':len(IDS),'records':reviews,'audit_correction':'Materials/FinalElements/ContainerModels not mistaken for missing Particle rows. Fresh3candidate audit verifies each actual typed table category independently. Selected equipment has no Particle consumers/definition requirement; category alone never prohibits art generation.','full_remaining_source_audit_count':len(tail),'source_only_ids':[i for i in SELECTED if i not in IDS],'source_only_records':sourceonly,'unreviewed_source_ids':[r['id'] for r in tail if r['id'] not in IDS and not r['source_RGB_private_view'] and not r['unresolved_particle_ids']],'source_parity':'Fresh read-only all actual TextureN ParticleInstance slots and all actual Materials.TextureID/FinalElements.LightFlareTexture/ContainerModels.PLightFlareTexture bindings exact queued sets. Actual all Material model consumers/templates/geometries checked. Historical MMP/release/existing original native RGBA bytes/dims/hash identical. Actual material Alpha/AddressMode enums retained, missing runtime dispatch/meshUV unknown not fabricated.','tool':'Built-in imagegen3 wholecanvas edits, exactly one/ID; source native original and opaqueRGB wholeNEAREST4x-or8x source guide privately inspected before every call, immediate raw/exactCRLFprompt/refSHA/callmetadata saves. No retries.','QA':'Complete raw and original4xRGBA/storedRGB/native sourceA-restored scalar/native material privately inspected; quantitative gains/local deltas/count bboxes are diagnostics only, not artistic edits/registration. SourceA4x byte-exact; targets512x256 or512square per original4x, physicalPOT.','ready_semantics':'Candidates awaiting coordinator final native material review; no accepted/packed claims. Held candidates explicitly pending, not permanent refusal.','ownership':'Independent expanded artifacts only. Shared metadata/calibration/import/build/Git and every preceding stable package untouched.'})
allproof=read('audit-equipment-nineteenth-all-source-check.json')
for r in allproof:
    if not r.get('fresh_typed_usage'):
        r.get('UV_proof',{}).pop('particle_implementation',None);r.get('UV_proof',{}).pop('renderer_sha256',None)
save('audit-equipment-nineteenth-all-source-check.json',allproof)
files=[p for p in BASE.glob('*equipment-nineteenth*.json')]+[p for p in (BASE/'generated').glob('*equipment-nineteenth-call.json')]
save('sha256-equipment-nineteenth.json',{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if not p.name.startswith('sha256-')})
print('Stable3call equipment package; ready',READY,'pending',PENDING)

import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[609,2377,4267]
SELECTED=[609,2377,4267,5374]
READY=[609]
PENDING=[i for i in IDS if i not in READY]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
proof={r['id']:r for r in read('source-check-equipment-eighteenth.json')}
metrics={r['id']:r for r in read('metrics-equipment-eighteenth.json')}
details={r['id']:r for r in read('detail-metrics-equipment-eighteenth.json')}
notes={609: 'Complete original128x64/source fullRGB NEAREST4x512x256/fullraw1774x887/native512x256 sourceRGBA and sourceA-restored storedRGB privately reviewed. Muted green/gray painted shell fields/top wear contour/right dark oval remain same fullcanvasUV; EXACTLY TWO source faint isolated specks preserved at fixed sourcebboxes[30,29,31,31] and[38,34,40,36]. Source and native maxima coincide at30,29 and39,34; source107/130/115→113/133/119 and140/162/148→136/163/142, original relative mark strength retained, no extra points/holes/glints. Fine soft painted surface and original irregular paintwear footprint retained, no new photograin/hardware. Native luma spatial corr.99536, maxlocal+35 beside existingsecond speck at39,33 source24/56/33→66/91/70; weak-source gain7. Flag soft halo/interpolation next to secondmark and slightly stronger sourcewear edge for root finalnative artreview, not accepted claim. Wholeinverse4x512x256/sourceA255 byte-exact/POT, no crop/BBox/composite/artistRGBfix/retry. Actual directMaterials430.Template329 Alphaopaque/Clamp, no staticModels consume it; dispatch/meshUV unknown honestly.', 2377: 'Complete original64square/source wholeNEAREST8x512square/fullraw1254square/native256square sourceRGBA/candidate sourceA+storedRGB reviewed. Original olive/charcoal fullcanvas fields and rounded parts retained, no new glyph/buttons; sourcealpha255 byteexact. However originally smooth flat painted strips/rims now have stronger raised/beveled border interpretation and fine new surfacegrain; thin olive midrim increased source82/81/41→107/104/57 at34,30 (+25). Geometry/material strength not faithful despite wholeUV/counts broadlyretained/corr.9786/weak-sourcegain11. Hold native border/material, no accepted claim. Onecall, fullinverse4x256square only, no retry/artistpatch.', 4267: 'Complete original128x64/fullNEAREST4x512x256 helper/fullraw1774x887/native512x256 sourceRGBA and candidate sourceA/storedRGB privately reviewed; same-scale private diagnostic source/candidate fastener fields also inspected without editing. Brown softpaint broadleftbody/two rightfoldedstrips keep sourceUV/palette/folds, corr.98483/maxlocal+27/weaksourcegain27. PRE-CALL SOURCE ANNOTATION ERROR: prompt and constraints said THREE narrowmotifs, incorrect; actual source has TWO central long painted straps plus TWO side looplike motifs = FOUR. Exact originalprompt/precallconstraints/raw remain untouched; honest correction in audit-correction/pattern-metrics. Candidate keeps those four major motifs but tiny central dark strapspots become explicitly rounded hole-like detail; source internal fastener geometry/spot meaning not secure enough to accept. Hold small-detail interpretation, not permanent refusal. SourceA255 byteexact, wholeinverse512x256/POT, no crop/BBox/source-artRGB insert/retry.'}

jobs=[];reviews=[]
for i in IDS:
    p=proof[i];raw=BASE/f'generated/{i}-raw.png';prompt=BASE/f'generated/{i}-prompt.txt';cp=BASE/f'generated/{i}-equipment-eighteenth-call.json';call=read(str(cp.relative_to(BASE)))
    exact=prompt.read_bytes().decode('utf-8');assert call['exact_prompt']==exact and not exact.endswith('\n')
    assert '\r\n' in exact and hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    refs={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']};assert refs==call['reference_sha256']
    original=Image.open(BASE/f'original/{i}.png').convert('RGBA');helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB');rawimage=Image.open(raw).convert('RGBA')
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    assert rawimage.getchannel('A').getextrema()==(255,255)
    native=Image.open(BASE/f'private-equipment-eighteenth/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4) and all(n>0 and not n&(n-1) for n in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert not p['material_evidence']['unresolved_particle_definitions'] and not p['material_evidence']['missing_sibling_Texture_rows']
    call.update(original_source_size=list(original.size),raw_size=list(rawimage.size),native_texture_type=p['material_evidence']['actual_source_native_type'],native_alpha_extrema=p['material_evidence']['source_alpha_extrema'],reference_argument_serialization='Actual built-in references absolute current repo paths; these portable identities reproduce the same exact byte SHA256 inputs.')
    save(str(cp.relative_to(BASE)),call)
    p.update(full_original_private_view_before_call=True,full_helper_private_view_before_call=True,full_raw_private_view=True,full_native4x_original_RGBA_RGB_and_candidate_nativeA_storedRGB_private_view=True,imagegen_call_count=1,source_visual_review=notes[i],generated_artifacts=[call['generated']])
    p['source_only_reason']=None
    p['UV_proof'].pop('particle_implementation',None);p['UV_proof'].pop('renderer_sha256',None)
    p['pattern_constraints']=next(x for x in read('pattern-constraints-equipment-eighteenth.json') if x['id']==i)
    if i==4267:p['precall_annotation_correction']=read('audit-correction-equipment-eighteenth.json')['precall_art_annotation_correction']
    if i in READY:jobs.append({'id':i,'generated':call['generated'],'prompt':exact,'source_transform_recipe':p['helper_recipe']})
    entry={'id':i,'status':'ready-for-coordinator-art-review' if i in READY else 'pending-native-material-review','call_count':1,'review_note':notes[i],'review_concern':notes[i],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':f'assets/terrain-hd/expanded/generated/{i}-equipment-eighteenth-call.json','reference_pngs':call['reference_pngs'],'reference_sha256':refs,'source_transform_recipe':p['helper_recipe'],'source_check':'assets/terrain-hd/expanded/source-check-equipment-eighteenth.json','metrics':metrics[i],'detail_metrics':details[i],'source_native4x_RGB':f'assets/terrain-hd/expanded/private-equipment-eighteenth/native4x/{i}-source-rgb-native4x-private.png','source_native4x_RGBA':f'assets/terrain-hd/expanded/private-equipment-eighteenth/native4x/{i}-source-rgba-native4x-private.png','candidate_native_alpha':f'assets/terrain-hd/expanded/private-equipment-eighteenth/native4x/{i}-calibrated-native-alpha-private.png','candidate_native_storedRGB':metrics[i]['stored_native_rgb_preview'],'material_evidence':p['material_evidence'],'actual_particle_binding_count':len(p['fresh_typed_usage']),'actual_nonparticle_binding_count':len(p['fresh_nonparticle_usage']),'UV_registration':'None. ENTIREraw→exact original4x wholecanvas LANCZOS. Original sourceA separate. No crop/padding/autobbox/fitting/recenter/rotation/regioncomposite/artRGBpatch. Model mesh UV/overlay dispatch unknown, not inferred; all full resource material roles remain unchanged.'}
    entry['pattern_constraints']=p['pattern_constraints']
    entry['pattern_metrics']=next(x for x in read('pattern-metrics-equipment-eighteenth.json') if x['id']==i)
    if i in PENDING:
        folder=BASE/'rejected/equipment-eighteenth';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            dst=folder/f'{i}-attempt1-{suffix}';assert not dst.exists() or dst.read_bytes()==f.read_bytes()
            if not dst.exists():shutil.copyfile(f,dst)
        save(str((folder/f'{i}-attempt1-reason.json').relative_to(BASE)),{'id':i,'status':'pending-native-material-review','permanent_refusal':False,'reason':notes[i],'raw_sha256':call['raw_sha256'],'call_count':1})
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/equipment-eighteenth/{i}-attempt1-raw.png'
    reviews.append(entry)
save('source-check-equipment-eighteenth.json',[proof[i] for i in SELECTED])
save('jobs-equipment-eighteenth.json',jobs)
tail=read('audit-equipment-eighteenth-readonly-tail.json')
sourceonly=[r for r in tail if r['id'] not in IDS and (r['source_RGB_private_view'] or r['unresolved_particle_ids'])]
save('source-only-equipment-eighteenth.json',sourceonly)
save('review-equipment-eighteenth.json',{'status':'stable-ready-subset-with-pending-border-and-small-detail-material','selected_ids':SELECTED,'generated_ids':IDS,'ready_ids':READY,'pending_ids':PENDING,'call_count':len(IDS),'records':reviews,'audit_correction':'Materials/FinalElements/ContainerModels not mistaken for missing Particle rows. Fresh4candidate audit verifies each actual typed table category independently. Selected equipment has no Particle consumers/definition requirement; category alone never prohibits art generation.','full_remaining_source_audit_count':len(tail),'source_only_ids':[i for i in SELECTED if i not in IDS],'source_only_records':sourceonly,'unreviewed_source_ids':[r['id'] for r in tail if r['id'] not in IDS and not r['source_RGB_private_view'] and not r['unresolved_particle_ids']],'source_parity':'Fresh read-only all actual TextureN ParticleInstance slots and all actual Materials.TextureID/FinalElements.LightFlareTexture/ContainerModels.PLightFlareTexture bindings exact queued sets. Actual all Material model consumers/templates/geometries checked. Historical MMP/release/existing original native RGBA bytes/dims/hash identical. Actual material Alpha/AddressMode enums retained, missing runtime dispatch/meshUV unknown not fabricated.','tool':'Built-in imagegen3 wholecanvas edits, exactly one/ID; source native original and opaqueRGB wholeNEAREST4x-or8x source guide privately inspected before every call, immediate raw/exactCRLFprompt/refSHA/callmetadata saves. No retries.','QA':'Complete raw and original4xRGBA/storedRGB/native sourceA-restored scalar/native material privately inspected; quantitative gains/local deltas/count bboxes are diagnostics only, not artistic edits/registration. SourceA4x byte-exact; targets512x256 or256square per original4x, physicalPOT.','ready_semantics':'Candidates awaiting coordinator final native material review; no accepted/packed claims. Held candidates explicitly pending, not permanent refusal.','ownership':'Independent expanded artifacts only. Shared metadata/calibration/import/build/Git and every preceding stable package untouched.'})
allproof=read('audit-equipment-eighteenth-all-source-check.json')
for r in allproof:
    if not r.get('fresh_typed_usage'):
        r.get('UV_proof',{}).pop('particle_implementation',None);r.get('UV_proof',{}).pop('renderer_sha256',None)
save('audit-equipment-eighteenth-all-source-check.json',allproof)
files=[p for p in BASE.glob('*equipment-eighteenth*.json')]+[p for p in (BASE/'generated').glob('*equipment-eighteenth-call.json')]
save('sha256-equipment-eighteenth.json',{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if not p.name.startswith('sha256-')})
print('Stable3call equipment package; ready',READY,'pending',PENDING)

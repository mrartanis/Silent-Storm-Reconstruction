import hashlib,json,shutil
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[1746,2447]
SELECTED=[1746,2447]
READY=[1746,2447]
PENDING=[i for i in IDS if i not in READY]
def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
proof={r['id']:r for r in read('source-check-equipment-twenty-second.json')}
metrics={r['id']:r for r in read('metrics-equipment-twenty-second.json')}
details={r['id']:r for r in read('detail-metrics-equipment-twenty-second.json')}
notes={1746: 'Full native128x64/sourceRGB NEAREST4x512x256/fullraw1774x887/native512x256 originalRGBA/sourceA/storedRGB privately reviewed. Muted graygreen soft finepaint cloth, original upper curve/small upperleft patch/TWO lowerleft bands with original angular blackgap/ONE separate right oval retain broad normalized positions and component count, no extra stitching/weave/buttons/glyph or photographic grain. Two existing faint palepatch maxima coincide at source native4,37 and5,48:165/170/165→153/165/155;189/203/198→155/167/158, not brighter new white spots. Review concerns: upper green sourceedge locally+41 and slightly smoother/more regular ovaledge/soft paintmottling than source, native correlation .978866/weaksourcegain91. Ready only for coordinator final native material/fieldedge/relativepaint review, no accepted claim. SourceA255 byte-exact/POT512x256, wholeinverse4x only/no crop/BBox/artistic sourceRGB fix/retry.', 2447: 'Full native64square/sourceRGB NEAREST8x512square/fullraw1254square/native256square originalRGBA/sourceA/storedRGB privately reviewed. Charcoalgray soft matte paintedcloth, large rounded leftfield/curved dark lowerfield/ONE long narrow rightstrip/weak angular lowerleft patches keep fullcanvas positions and broad sourcepatternscale. No added button/stitch/glyph, new weave or shiny photographic leather. Mild finerpaint and sharper curved lightedge are review concerns; narrow edge maxlocal+27 at46,7 source33/32/33→60/60/59, corr .981426/weaksourcegain40. Original broad quiet grayfield remains subdued. Ready only for coordinator final native material/rimstrength review, no accepted claim. SourceA255 byte-exact/POT256square/wholeinverse only, no crop/BBox/artRGB/retry.'}

jobs=[];reviews=[]
for i in IDS:
    p=proof[i];raw=BASE/f'generated/{i}-raw.png';prompt=BASE/f'generated/{i}-prompt.txt';cp=BASE/f'generated/{i}-equipment-twenty-second-call.json';call=read(str(cp.relative_to(BASE)))
    exact=prompt.read_bytes().decode('utf-8');assert call['exact_prompt']==exact and not exact.endswith('\n')
    assert '\r\n' in exact and hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    refs={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']};assert refs==call['reference_sha256']
    original=Image.open(BASE/f'original/{i}.png').convert('RGBA');helper=Image.open(ROOT/call['reference_pngs'][0]).convert('RGB');rawimage=Image.open(raw).convert('RGBA')
    assert helper.tobytes()==original.convert('RGB').resize(helper.size,Image.Resampling.NEAREST).tobytes()
    assert rawimage.getchannel('A').getextrema()==(255,255)
    native=Image.open(BASE/f'private-equipment-twenty-second/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4) and all(n>0 and not n&(n-1) for n in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    assert not p['material_evidence']['unresolved_particle_definitions'] and not p['material_evidence']['missing_sibling_Texture_rows']
    call.update(original_source_size=list(original.size),raw_size=list(rawimage.size),native_texture_type=p['material_evidence']['actual_source_native_type'],native_alpha_extrema=p['material_evidence']['source_alpha_extrema'],reference_argument_serialization='Actual built-in references absolute current repo paths; these portable identities reproduce the same exact byte SHA256 inputs.')
    save(str(cp.relative_to(BASE)),call)
    p.update(full_original_private_view_before_call=True,full_helper_private_view_before_call=True,full_raw_private_view=True,full_native4x_original_RGBA_RGB_and_candidate_nativeA_storedRGB_private_view=True,imagegen_call_count=1,source_visual_review=notes[i],generated_artifacts=[call['generated']])
    p['source_only_reason']=None
    p['UV_proof'].pop('particle_implementation',None);p['UV_proof'].pop('renderer_sha256',None)
    p['pattern_constraints']=next(x for x in read('pattern-constraints-equipment-twenty-second.json') if x['id']==i)
    if i in READY:jobs.append({'id':i,'generated':call['generated'],'prompt':exact,'source_transform_recipe':p['helper_recipe']})
    entry={'id':i,'status':'ready-for-coordinator-art-review' if i in READY else 'pending-native-material-review','call_count':1,'review_note':notes[i],'review_concern':notes[i],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':f'assets/terrain-hd/expanded/generated/{i}-equipment-twenty-second-call.json','reference_pngs':call['reference_pngs'],'reference_sha256':refs,'source_transform_recipe':p['helper_recipe'],'source_check':'assets/terrain-hd/expanded/source-check-equipment-twenty-second.json','metrics':metrics[i],'detail_metrics':details[i],'source_native4x_RGB':f'assets/terrain-hd/expanded/private-equipment-twenty-second/native4x/{i}-source-rgb-native4x-private.png','source_native4x_RGBA':f'assets/terrain-hd/expanded/private-equipment-twenty-second/native4x/{i}-source-rgba-native4x-private.png','candidate_native_alpha':f'assets/terrain-hd/expanded/private-equipment-twenty-second/native4x/{i}-calibrated-native-alpha-private.png','candidate_native_storedRGB':metrics[i]['stored_native_rgb_preview'],'material_evidence':p['material_evidence'],'actual_particle_binding_count':len(p['fresh_typed_usage']),'actual_nonparticle_binding_count':len(p['fresh_nonparticle_usage']),'UV_registration':'None. ENTIREraw→exact original4x wholecanvas LANCZOS. Original sourceA separate. No crop/padding/autobbox/fitting/recenter/rotation/regioncomposite/artRGBpatch. Model mesh UV/overlay dispatch unknown, not inferred; all full resource material roles remain unchanged.'}
    entry['pattern_constraints']=p['pattern_constraints']
    entry['pattern_metrics']=next(x for x in read('pattern-metrics-equipment-twenty-second.json') if x['id']==i)
    if i in PENDING:
        folder=BASE/'rejected/equipment-twenty-second';folder.mkdir(parents=True,exist_ok=True)
        for f,suffix in [(raw,'raw.png'),(prompt,'prompt.txt'),(cp,'call.json')]:
            dst=folder/f'{i}-attempt1-{suffix}';assert not dst.exists() or dst.read_bytes()==f.read_bytes()
            if not dst.exists():shutil.copyfile(f,dst)
        save(str((folder/f'{i}-attempt1-reason.json').relative_to(BASE)),{'id':i,'status':'pending-native-material-review','permanent_refusal':False,'reason':notes[i],'raw_sha256':call['raw_sha256'],'call_count':1})
        entry['held_raw']=f'assets/terrain-hd/expanded/rejected/equipment-twenty-second/{i}-attempt1-raw.png'
    reviews.append(entry)
save('source-check-equipment-twenty-second.json',[proof[i] for i in SELECTED])
save('jobs-equipment-twenty-second.json',jobs)
tail=read('audit-equipment-twenty-second-readonly-tail.json')
sourceonly=[r for r in tail if r['id'] not in IDS and (r['source_RGB_private_view'] or r['unresolved_particle_ids'])]
save('source-only-equipment-twenty-second.json',sourceonly)
save('review-equipment-twenty-second.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':SELECTED,'generated_ids':IDS,'ready_ids':READY,'pending_ids':PENDING,'call_count':len(IDS),'records':reviews,'audit_correction':'Materials/FinalElements/ContainerModels not mistaken for missing Particle rows. Fresh2candidate audit verifies each actual typed table category independently. Selected equipment has no Particle consumers/definition requirement; category alone never prohibits art generation.','full_remaining_source_audit_count':len(tail),'source_only_ids':[i for i in SELECTED if i not in IDS],'source_only_records':sourceonly,'unreviewed_source_ids':[r['id'] for r in tail if r['id'] not in IDS and not r['source_RGB_private_view'] and not r['unresolved_particle_ids']],'source_parity':'Fresh read-only all actual TextureN ParticleInstance slots and all actual Materials.TextureID/FinalElements.LightFlareTexture/ContainerModels.PLightFlareTexture bindings exact queued sets. Actual all Material model consumers/templates/geometries checked. Historical MMP/release/existing original native RGBA bytes/dims/hash identical. Actual material Alpha/AddressMode enums retained, missing runtime dispatch/meshUV unknown not fabricated.','tool':'Built-in imagegen2 wholecanvas edits, exactly one/ID; source native original and opaqueRGB wholeNEAREST4x-or8x source guide privately inspected before every call, immediate raw/exactCRLFprompt/refSHA/callmetadata saves. No retries.','QA':'Complete raw and original4xRGBA/storedRGB/native sourceA-restored scalar/native material privately inspected; quantitative gains/local deltas/count bboxes are diagnostics only, not artistic edits/registration. SourceA4x byte-exact; targets512x256 or256square per original4x, physicalPOT.','ready_semantics':'Candidates awaiting coordinator final native material review; no accepted/packed claims. Held candidates explicitly pending, not permanent refusal.','ownership':'Independent expanded artifacts only. Shared metadata/calibration/import/build/Git and every preceding stable package untouched.'})
allproof=read('audit-equipment-twenty-second-all-source-check.json')
for r in allproof:
    if not r.get('fresh_typed_usage'):
        r.get('UV_proof',{}).pop('particle_implementation',None);r.get('UV_proof',{}).pop('renderer_sha256',None)
save('audit-equipment-twenty-second-all-source-check.json',allproof)
files=[p for p in BASE.glob('*equipment-twenty-second*.json')]+[p for p in (BASE/'generated').glob('*equipment-twenty-second-call.json')]
save('sha256-equipment-twenty-second.json',{str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if not p.name.startswith('sha256-')})
print('Stable2call equipment package; ready',READY,'pending',PENDING)

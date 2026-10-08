from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import shutil
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[3048,5437,5436]
READY=[5437,5436]
def read(path):return json.loads(path.read_text(encoding='utf-8'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def portable(path):return path.relative_to(ROOT).as_posix()
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

metrics={r['id']:r for r in read(BASE/'metrics-equipment-twenty-eighth.json')}
details={r['id']:r for r in read(BASE/'detail-metrics-equipment-twenty-eighth.json')}
patterns={r['id']:r for r in read(BASE/'pattern-metrics-equipment-twenty-eighth.json')}
constraints={r['id']:r for r in read(BASE/'pattern-constraints-equipment-twenty-eighth.json')}
proof_path=BASE/'source-check-equipment-twenty-eighth.json'
source_before=BASE/'source-check-equipment-twenty-eighth-before-call.json'
assert not source_before.exists()
source_before.write_bytes(proof_path.read_bytes())
proof=read(proof_path)
selected={r['id']:r for r in read(BASE/'selected-equipment-twenty-eighth.json')}

domains={3048:[('left matte graypaint',[0,0,28,90]),('upper broad darkbrownpaint',[31,20,126,60]),('gray oval surroundingpaint',[54,86,82,106]),('existing two crossedstrokes',[4,112,12,120]),('lower pointed graypaint',[44,107,128,128])],5437:[('large left finebrownpaint',[10,15,42,49]),('upper right finebrownpaint',[75,5,120,28]),('lower taperedpaint',[52,44,71,60]),('first lower roundedpaint',[82,46,98,58]),('second lower roundedpaint',[108,45,120,57])],5436:[('upper pale speckledpaint',[42,2,120,30]),('lower pale speckledpaint',[100,43,114,57]),('left darkbrownpaint',[0,0,22,64]),('first lower warmbrownpaint',[34,46,49,61]),('second lower warmbrownpaint',[65,46,72,60])]}
materials=[]
for texture_id in IDS:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    source=np.array(original.convert('RGB'),dtype=float)
    candidate=np.array(Image.open(BASE/'private-equipment-twenty-eighth/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(original.size,Image.Resampling.LANCZOS),dtype=float)
    row={'id':texture_id,'scope':'Fixed native diagnostic domains on ENTIRE inverse-resized candidate; not ROI editing/registration/source-RGB artistic insertion.','source_alpha_extrema':list(original.getchannel('A').getextrema()),'domains':[]}
    for name,box in domains[texture_id]:
        x0,y0,x1,y1=box;s=source[y0:y1,x0:x1];g=candidate[y0:y1,x0:x1]
        row['domains'].append({'name':name,'native_bbox':box,'source_mean_RGB':s.mean((0,1)).tolist(),'candidate_mean_RGB':g.mean((0,1)).tolist(),'source_luma_std':float(s.mean(2).std()),'candidate_luma_std':float(g.mean(2).std())})
    materials.append(row)
save('material-metrics-equipment-twenty-eighth.json',materials)

notes={3048: 'Pending native material/edge/mark-strength review. Full original128square/sourceRGBA/storedRGB wholeNEAREST4x512square guide/fullraw1254square/native4x512square/sourceA/storedRGB privately viewed. Original eight bitmap regions broadly remain and existing crossed mark stays two crossed bright strokes, no new font/brand/symbol semantic inferred. But narrow original graypaint edges/oval surround become raised-looking stronger glossy rims, broad brownpaint/leftgray relative light changed; local+47 at26,81/weakgain560/corr .9708. Mark afterscalar support14→8pixels maxRGB>120/bbox[5,114,11,119]→[6,114,10,118], peak8,116 RGB231/231/231→7,115 RGB171/169/166, no sourceRGB glyph patch. Actual sourceTypeOrdinary with A0..255 and MaterialAlpha alpha_test, original sourceA4x byte-exact; not inferred A255 from Type. Geometry/content source mask retained by full originalA, material drift still real. Held raw/exactprompt/call/reason saved, onecall no retry/crop/BBox/RGB artist repair. Pending, not permanent refusal.', 5437: 'Ready only coordinator final native material review. Full original128x64/sourceRGBA/storedRGB wholeNEAREST4x512x256 guide/fullraw1774x887/native4x512x256/sourceA/storedRGB privately viewed. Exactly five sourcepaint bitmap regions retained in original normalized positions/sourceblack gaps: one left polygon/one upper-right broadbrownfield/one lower-middle taperedfield/two lower-right roundedfields. Original dark matte fine brownpaint/huehierarchy/grain kind retained, no new physical cap/rim/fragment/knurl. Left painted-tip actual source/candidate both end nativey59, y60..63black in both; no shorter tip despite initial thumbnail impression. Corr .981/maxlocal+30 near existing round soft edge/weakgain13; moderatepaint/soft-edge caveat, not auto-hold. Left field mean16.95→16.54/std1.81→1.82/highpass1.55→1.32; upperright mean13.22→15.77; smaller lower-right paint smoother/dimmer mean54.72→48.67/highpass3.86→1.93. Explicit caveat smoothing/grain strength there for coordinator fullraw/source material review. SourceA255 byte-exact/POT512x256, pure entire inverse/scalar/defaultpad, no crop/BBox/rotation/artRGB/retry. Not worker accepted/packed claim.', 5436: 'Ready only coordinator final native material review. Full original128x64/sourceRGBA/storedRGB wholeNEAREST4x512x256 guide/fullraw1774x887/native4x512x256/sourceA/storedRGB privately viewed. Same original large upper pale fine-speckled field/small lower-right pale polygon/two lower warmbrownfields/dark far-left brownfield in exact wholecanvas layout/shapes/relativepalette, no new cracks/physical stones/cauliflower/fragments. Source stochastic fine gray-brown painted speckles retain source spatial pattern/scale: native highpass correlations .815upper/.902lower; upper source/candidate means126.55→128.71/std20.64→16.44/highpass12.81→10.10; lower means146.42→146.44/std20.59→18.96/highpass17.18→14.98. Caveat smoother grain and local source darkgranule lift+44 at60,3 or94,39, weakgain40; source grain material remains fine matte, no added rim/relief. Whole corr .9896, moderatepaint/soft-edge caveats for final coordinator art decision, not worker acceptedclaim. SourceA255 byte-exact/POT512x256, originalA/pure scalar/ENTIRE inverse only, no artistRGB/BBox/crop/retry.'}

records=[];jobs=[]
for texture_id in IDS:
    call_path=BASE/'generated'/f'{texture_id}-equipment-twenty-eighth-call.json'
    call=read(call_path)
    assert sha(ROOT/call['generated'])==call['raw_sha256']
    assert sha(ROOT/call['prompt_file'])==call['prompt_sha256']
    assert (ROOT/call['prompt_file']).read_bytes().decode('utf-8')==call['exact_prompt']
    assert all(sha(ROOT/name)==digest for name,digest in call['reference_sha256'].items())
    assert sha(BASE/'pattern-constraints-equipment-twenty-eighth.json')==call['pattern_constraints_file_sha256']
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    native=Image.open(BASE/'private-equipment-twenty-eighth/native4x'/f'{texture_id}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(d&(d-1)==0 for d in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    status='ready-for-coordinator-art-review'if texture_id in READY else'pending-native-material-review'
    record={'id':texture_id,'status':status,'call_count':1,'review_note':notes[texture_id],'review_concern':notes[texture_id],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':portable(call_path),'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':call['source_transform_recipe'],'source_check':portable(proof_path),'source_before_call_snapshot':portable(source_before),'metrics':metrics[texture_id],'detail_metrics':details[texture_id],'pattern_constraints':constraints[texture_id],'pattern_metrics':patterns[texture_id]}
    if texture_id in READY:
        jobs.append({'id':texture_id,'generated':call['generated'],'prompt':call['exact_prompt'],'source_transform_recipe':call['source_transform_recipe']})
    else:
        rejected=BASE/'rejected'/'equipment-twenty-eighth';rejected.mkdir(parents=True,exist_ok=True)
        for suffix,source in [('raw.png',ROOT/call['generated']),('prompt.txt',ROOT/call['prompt_file']),('call.json',call_path)]:
            target=rejected/f'{texture_id}-attempt1-{suffix}';assert not target.exists();shutil.copyfile(source,target)
        reason=rejected/f'{texture_id}-attempt1-reason.json';reason.write_text(json.dumps({'id':texture_id,'status':status,'reason':notes[texture_id],'raw_sha256':call['raw_sha256'],'one_call_only':True},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        record['held_raw']=portable(rejected/f'{texture_id}-attempt1-raw.png')
    for row in proof:
        if row['id']==texture_id:
            row.update(imagegen_call_count=1,postcall_status=status,full_raw_native4x_source_alpha_storedRGB_privately_reviewed=True,review_note=notes[texture_id])
    records.append(record)
save('jobs-equipment-twenty-eighth.json',jobs)
save('source-check-equipment-twenty-eighth.json',proof)
save('audit-correction-equipment-twenty-eighth.json',{'scope':'Fresh actual Materials/templates/all consumers independently verified; no Particle-only missing-definition claim.','selected_ids':IDS,'actual_native_type':{str(i):selected[i]['texture']['Type']for i in IDS},'actual_source_alpha_extrema':{str(i):list(Image.open(BASE/'original'/f'{i}.png').convert('RGBA').getchannel('A').getextrema())for i in IDS},'note':'3048actual Ordinary sourceA0..255/MaterialAlpha alpha_test;5436/5437actual A255/opaque, never inferred from Type alone. Existing two crossed bright3048strokes treated as exactsourcepaint, no font/brand meaning inferred. A drafting-only markerRGB206/217/231 guess was corrected to measured231/231/231 BEFORE3048 built-in call; executed sourceconstraint originalactual value always correct/unchanged. Thumbnail impression of5437shorter lefttip disproved by fresh actualsource/candidate rows:bothend nativey59/y60..63black, no invented UVdrift claim. All correction diagnostics read-only; raw/exact actualcall/reference/source proof bytes preserved.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'generation_calls':3,'retries':0})
save('survey-equipment-twenty-eighth.json',{'scope':'Full original-only visual survey; no fresh typed/historyrelease parity claim for unselected IDs. No blanket remaining-group suitability/exhaustion claim.','viewed_unselected_ids':[3039,3384,5380,5386,5387,5431,5438,6901,6902],'call_count':0,'note':'Some maps have readable text/microglyphs or complex native strands/patterns, others genuine complex material not chosen in this small scope. Complexity/simple surfaces/repeating patterns alone never make a source technical. All unselected0calls.'})

exclusions=read(BASE/'exclusions-equipment-twenty-eighth.json')
assert not set(IDS).intersection(exclusions['ids'])
assert all(row['source_rgba_sha256']not in exclusions['blocked_hashes']for row in proof)
source_path=BASE.parent/'sources.json';queue_path=BASE/'queue.json';sbytes=source_path.read_bytes();qbytes=queue_path.read_bytes();sources=json.loads(sbytes.decode('utf-8'))['textures'];queue=json.loads(qbytes.decode('utf-8'));own=[{'id':r['id'],'status':r.get('status')}for r in sources+queue if r['id']in IDS]
assert not own,own
save('final-guards-equipment-twenty-eighth.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[exclusions['accepted_count'],exclusions['queue_count']],'postcall_readonly_counts':[len(sources),len(queue)],'selected_beforecall_allids_and_calledRGBA_absent':True,'own_postcall_sources_queue_overlap':own,'source_snapshot_sha256':hashlib.sha256(sbytes).hexdigest(),'queue_snapshot_sha256':hashlib.sha256(qbytes).hexdigest(),'shared_writes':False,'generated_ids':IDS,'call_count':3,'repeat_calls':0})
save('review-equipment-twenty-eighth.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':IDS,'generated_ids':IDS,'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'call_count':3,'records':records,'source_parity':'Fresh read-only actual Materials.TextureID/templates/all actual Model consumers/geometries/templates and all Particle TextureN/nonparticle bindings exact sourcequeue; historicalMMP/release/existing original native RGBA bytes/dims/hash identical. Missing runtime meshUV/dispatch unknown, no fake semantic proof.','QA':'Every full original/helper/raw/native4x storedRGB and originalA restored preview inspected privately. Original fullcanvas sourceA byte-exact, pure scalar calibrated entire inverse only; diagnostics do not alter art or UV. All targets physicalPOT.','ready_semantics':'Only coordinator review candidates, never accepted/packed worker claim. Held records remain pending, not permanent refusal.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'pattern_before_call_proof':portable(BASE/'pattern-constraints-equipment-twenty-eighth.json'),'material_metrics':portable(BASE/'material-metrics-equipment-twenty-eighth.json'),'audit_correction':portable(BASE/'audit-correction-equipment-twenty-eighth.json'),'final_guards':portable(BASE/'final-guards-equipment-twenty-eighth.json'),'unselected_original_only_survey':portable(BASE/'survey-equipment-twenty-eighth.json'),'ownership':'Independent Eq25 files only. Stable Eq24/prior packages/shared metadata/import/calibration/Git/Game/C++ untouched.'})
manifest_path=BASE/'sha256-equipment-twenty-eighth.json';files=sorted(BASE.glob('*equipment-twenty-eighth*.json'))+sorted((BASE/'generated').glob('*equipment-twenty-eighth-call.json'));manifest={portable(path):sha(path)for path in files if path!=manifest_path};save(manifest_path.name,manifest);assert all(sha(ROOT/name)==digest for name,digest in read(manifest_path).items())
print(json.dumps({'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'calls':3,'manifest_verified':len(manifest),'stable_SHA256':{name:sha(BASE/f'{name}-equipment-twenty-eighth.json')for name in ['jobs','review','source-check','selected','sha256']}},ensure_ascii=False))

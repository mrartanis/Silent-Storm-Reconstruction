from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import shutil
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[5405,5406,5388]
READY=[5406]
def read(path):return json.loads(path.read_text(encoding='utf-8'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def portable(path):return path.relative_to(ROOT).as_posix()
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

metrics={r['id']:r for r in read(BASE/'metrics-equipment-twenty-fifth.json')}
details={r['id']:r for r in read(BASE/'detail-metrics-equipment-twenty-fifth.json')}
patterns={r['id']:r for r in read(BASE/'pattern-metrics-equipment-twenty-fifth.json')}
constraints={r['id']:r for r in read(BASE/'pattern-constraints-equipment-twenty-fifth.json')}
proof_path=BASE/'source-check-equipment-twenty-fifth.json'
source_before=BASE/'source-check-equipment-twenty-fifth-before-call.json'
assert not source_before.exists()
source_before.write_bytes(proof_path.read_bytes())
proof=read(proof_path)
selected={r['id']:r for r in read(BASE/'selected-equipment-twenty-fifth.json')}

domains={5405:[('left gray sourcepaint',[0,0,21,64]),('three weak right gray strokes',[22,5,64,55])],5406:[('upper-left gray strips',[0,0,28,33]),('upper-right gray strips',[28,0,64,33]),('lower-right gray strips',[32,34,64,64])],5388:[('large brown field',[0,0,20,19]),('upper-right bands',[25,0,32,20]),('lower-gray source field',[5,25,15,32])]}
materials=[]
for texture_id in IDS:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    source=np.array(original.convert('RGB'),dtype=float)
    candidate=np.array(Image.open(BASE/'private-equipment-twenty-fifth/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(original.size,Image.Resampling.LANCZOS),dtype=float)
    row={'id':texture_id,'scope':'Fixed native diagnostic domains on ENTIRE inverse-resized candidate; not ROI editing/registration/source-RGB artistic insertion.','source_alpha_extrema':list(original.getchannel('A').getextrema()),'domains':[]}
    for name,box in domains[texture_id]:
        x0,y0,x1,y1=box;s=source[y0:y1,x0:x1];g=candidate[y0:y1,x0:x1]
        row['domains'].append({'name':name,'native_bbox':box,'source_mean_RGB':s.mean((0,1)).tolist(),'candidate_mean_RGB':g.mean((0,1)).tolist(),'source_luma_std':float(s.mean(2).std()),'candidate_luma_std':float(g.mean(2).std())})
    materials.append(row)
save('material-metrics-equipment-twenty-fifth.json',materials)

notes={
5405:'Pending material/paint position review. Full source/originalRGB wholeNEAREST8x guide/fullraw/native4x/sourceA/storedRGB privately viewed. Exactly two original dark left marks remain, but source minima11,13 and11,49 become11,14 and11,50 in whole-inverse diagnostics; source faint left contour becomes a stronger light raised-looking painted border (+47 at6,5 source57/56/66→96/100/113). Existing three broad right gray strokes remain wholeUV, yet stronger brush/bevel material requires coordinator judgment. Corr .9795/weakgain37. No retry or RGB art repair; A255 exact. Held, not accepted/permanent refusal.',
5406:'Ready only for coordinator final native art/material review. Full original64square storedRGB/sourceRGBA/wholeNEAREST8x512square guide/fullraw1254square/native4x256square/sourceA/storedRGB privately viewed. Eight original5x5 black fields retain all source normalized positions (two upper-right, three upper-left, three lower-right), plus one original large rounded dark lower-left field. SourceA actually0..255, all eight black fields A0; final sourceA restored byte-exact controls those original transparent footprints. Raw separately retains eight matching dark field interiors (17..21of25 native inverse pixels maxRGB<10), without new hardware/counts. Grey strips/parts/matte source palette retained; fine painterly strokes and original circle rim locally stronger (+32), flagged for coordinator fullraw review. Corr .9887/weakgain44. Wholecanvas only/POT/source scale, no BBox/ROI/artRGB/crop/padding/retry. This is not a worker accepted/packed claim.',
5388:'Pending source paint/detail-scale review. Full source32square/wholeNEAREST16x512square guide/fullraw/native4x128square/sourceA/storedRGB privately viewed. Original ten faint upper-right paintbands (row0 and2..18, pitch2) preserve exact native profile phase. Broad source brown/gray layout retained, but originally very weak brown field becomes more textured/coarsely brush-shaded (+21 at11,13 source46/30/5→67/43/9), and lower-gray small source marks appear enlarged/regularized into stronger rectangles. Pattern count alone is insufficient for material/detail-scale acceptance. Corr .9848/weakgain35. A255 byte-exact, no retry/artistRGB repair; held, not permanent refusal.'
}

records=[];jobs=[]
for texture_id in IDS:
    call_path=BASE/'generated'/f'{texture_id}-equipment-twenty-fifth-call.json'
    call=read(call_path)
    assert sha(ROOT/call['generated'])==call['raw_sha256']
    assert sha(ROOT/call['prompt_file'])==call['prompt_sha256']
    assert (ROOT/call['prompt_file']).read_bytes().decode('utf-8')==call['exact_prompt']
    assert all(sha(ROOT/name)==digest for name,digest in call['reference_sha256'].items())
    assert sha(BASE/'pattern-constraints-equipment-twenty-fifth.json')==call['pattern_constraints_file_sha256']
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    native=Image.open(BASE/'private-equipment-twenty-fifth/native4x'/f'{texture_id}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(d&(d-1)==0 for d in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    status='ready-for-coordinator-art-review'if texture_id in READY else'pending-native-material-review'
    record={'id':texture_id,'status':status,'call_count':1,'review_note':notes[texture_id],'review_concern':notes[texture_id],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':portable(call_path),'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':call['source_transform_recipe'],'source_check':portable(proof_path),'source_before_call_snapshot':portable(source_before),'metrics':metrics[texture_id],'detail_metrics':details[texture_id],'pattern_constraints':constraints[texture_id],'pattern_metrics':patterns[texture_id]}
    if texture_id in READY:
        jobs.append({'id':texture_id,'generated':call['generated'],'prompt':call['exact_prompt'],'source_transform_recipe':call['source_transform_recipe']})
    else:
        rejected=BASE/'rejected'/'equipment-twenty-fifth';rejected.mkdir(parents=True,exist_ok=True)
        for suffix,source in [('raw.png',ROOT/call['generated']),('prompt.txt',ROOT/call['prompt_file']),('call.json',call_path)]:
            target=rejected/f'{texture_id}-attempt1-{suffix}';assert not target.exists();shutil.copyfile(source,target)
        reason=rejected/f'{texture_id}-attempt1-reason.json';reason.write_text(json.dumps({'id':texture_id,'status':status,'reason':notes[texture_id],'raw_sha256':call['raw_sha256'],'one_call_only':True},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        record['held_raw']=portable(rejected/f'{texture_id}-attempt1-raw.png')
    for row in proof:
        if row['id']==texture_id:
            row.update(imagegen_call_count=1,postcall_status=status,full_raw_native4x_source_alpha_storedRGB_privately_reviewed=True,review_note=notes[texture_id])
    records.append(record)
save('jobs-equipment-twenty-fifth.json',jobs)
save('source-check-equipment-twenty-fifth.json',proof)
save('audit-correction-equipment-twenty-fifth.json',{'scope':'Fresh actual typed Materials/consumers independently verified; no missing Particle claim for Material-only sources.','selected_ids':IDS,'actual_native_type':{str(i):selected[i]['texture']['Type']for i in IDS},'correction':'Worker preliminary message incorrectly assumed sourceA255 for all three. Actual fresh source proof always recorded5406alpha0..255, and original alpha restored exactly in private production4x.5405/5388alpha255. Eight5406black square fields have sourceA0, not opaque shadow-only evidence. No false staticModel/Races/physicalhardware/meshUV inference. Actual argument/template/source bytes unchanged.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'generation_calls':3,'retries':0})
save('survey-equipment-twenty-fifth.json',{'scope':'Original-only visual survey, no fresh typed/historical/release parity claim for unselected IDs. No blanket remaining-group suitability or exhaustion claim.','viewed_unselected_ids':[2213,2378,2448,3000,5652,6213,5402,5404,5407,5408,5433,5436,5390],'call_count':0,'note':'These full originals were inspected privately; source patterns/mini hardware/marks not selected in this small scope. Counts/phase can be proved in another fresh independent scope, repeating pattern alone is not prohibited.'})

exclusions=read(BASE/'exclusions-equipment-twenty-fifth.json')
assert not set(IDS).intersection(exclusions['ids'])
assert all(row['source_rgba_sha256']not in exclusions['blocked_hashes']for row in proof)
source_path=BASE.parent/'sources.json';queue_path=BASE/'queue.json';sbytes=source_path.read_bytes();qbytes=queue_path.read_bytes();sources=json.loads(sbytes.decode('utf-8'))['textures'];queue=json.loads(qbytes.decode('utf-8'));own=[{'id':r['id'],'status':r.get('status')}for r in sources+queue if r['id']in IDS]
assert not own,own
save('final-guards-equipment-twenty-fifth.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[exclusions['accepted_count'],exclusions['queue_count']],'postcall_readonly_counts':[len(sources),len(queue)],'selected_beforecall_allids_and_calledRGBA_absent':True,'own_postcall_sources_queue_overlap':own,'source_snapshot_sha256':hashlib.sha256(sbytes).hexdigest(),'queue_snapshot_sha256':hashlib.sha256(qbytes).hexdigest(),'shared_writes':False,'generated_ids':IDS,'call_count':3,'repeat_calls':0})
save('review-equipment-twenty-fifth.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':IDS,'generated_ids':IDS,'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'call_count':3,'records':records,'source_parity':'Fresh read-only actual Materials.TextureID/templates/all actual Model consumers/geometries/templates and all Particle TextureN/nonparticle bindings exact sourcequeue; historicalMMP/release/existing original native RGBA bytes/dims/hash identical. Missing runtime meshUV/dispatch unknown, no fake semantic proof.','QA':'Every full original/helper/raw/native4x storedRGB and originalA restored preview inspected privately. Original fullcanvas sourceA byte-exact, pure scalar calibrated entire inverse only; diagnostics do not alter art or UV. All targets physicalPOT.','ready_semantics':'Only coordinator review candidates, never accepted/packed worker claim. Held records remain pending, not permanent refusal.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'pattern_before_call_proof':portable(BASE/'pattern-constraints-equipment-twenty-fifth.json'),'material_metrics':portable(BASE/'material-metrics-equipment-twenty-fifth.json'),'audit_correction':portable(BASE/'audit-correction-equipment-twenty-fifth.json'),'final_guards':portable(BASE/'final-guards-equipment-twenty-fifth.json'),'unselected_original_only_survey':portable(BASE/'survey-equipment-twenty-fifth.json'),'ownership':'Independent Eq25 files only. Stable Eq24/prior packages/shared metadata/import/calibration/Git/Game/C++ untouched.'})
manifest_path=BASE/'sha256-equipment-twenty-fifth.json';files=sorted(BASE.glob('*equipment-twenty-fifth*.json'))+sorted((BASE/'generated').glob('*equipment-twenty-fifth-call.json'));manifest={portable(path):sha(path)for path in files if path!=manifest_path};save(manifest_path.name,manifest);assert all(sha(ROOT/name)==digest for name,digest in read(manifest_path).items())
print(json.dumps({'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'calls':3,'manifest_verified':len(manifest),'stable_SHA256':{name:sha(BASE/f'{name}-equipment-twenty-fifth.json')for name in ['jobs','review','source-check','selected','sha256']}},ensure_ascii=False))

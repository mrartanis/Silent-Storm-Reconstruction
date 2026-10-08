from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import shutil
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[6403,7659,4803]
READY=[6403,7659]
def read(path):return json.loads(path.read_text(encoding='utf-8'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def portable(path):return path.relative_to(ROOT).as_posix()
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

metrics={r['id']:r for r in read(BASE/'metrics-equipment-twenty-seventh.json')}
details={r['id']:r for r in read(BASE/'detail-metrics-equipment-twenty-seventh.json')}
patterns={r['id']:r for r in read(BASE/'pattern-metrics-equipment-twenty-seventh.json')}
constraints={r['id']:r for r in read(BASE/'pattern-constraints-equipment-twenty-seventh.json')}
proof_path=BASE/'source-check-equipment-twenty-seventh.json'
source_before=BASE/'source-check-equipment-twenty-seventh-before-call.json'
assert not source_before.exists()
source_before.write_bytes(proof_path.read_bytes())
proof=read(proof_path)
selected={r['id']:r for r in read(BASE/'selected-equipment-twenty-seventh.json')}

domains={6403:[('left brown diagonalpaint',[0,0,29,29]),('right muted brownpaint',[30,0,64,30]),('single weak warmmark',[33,15,41,23])],7659:[('upper grayblue painted field',[20,0,95,18]),('left brown triangular field',[20,20,40,50]),('right brown triangular field',[45,20,75,55]),('source narrowgraylight',[20,2,40,15]),('original darkright painted field',[97,7,128,54])],4803:[('upperleft curvedpaint',[0,12,14,33]),('middleleft curvedpaint',[0,34,14,55]),('clippedlower curvepaint',[12,50,32,64]),('right narrow painted fields',[43,0,64,43]),('dark central roundedpaint',[17,25,38,58])]}
materials=[]
for texture_id in IDS:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    source=np.array(original.convert('RGB'),dtype=float)
    candidate=np.array(Image.open(BASE/'private-equipment-twenty-seventh/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(original.size,Image.Resampling.LANCZOS),dtype=float)
    row={'id':texture_id,'scope':'Fixed native diagnostic domains on ENTIRE inverse-resized candidate; not ROI editing/registration/source-RGB artistic insertion.','source_alpha_extrema':list(original.getchannel('A').getextrema()),'domains':[]}
    for name,box in domains[texture_id]:
        x0,y0,x1,y1=box;s=source[y0:y1,x0:x1];g=candidate[y0:y1,x0:x1]
        row['domains'].append({'name':name,'native_bbox':box,'source_mean_RGB':s.mean((0,1)).tolist(),'candidate_mean_RGB':g.mean((0,1)).tolist(),'source_luma_std':float(s.mean(2).std()),'candidate_luma_std':float(g.mean(2).std())})
    materials.append(row)
save('material-metrics-equipment-twenty-seventh.json',materials)

notes={6403: 'Ready only coordinator final native material review. Full original64x32/sourceRGBA/storedRGB wholeNEAREST8x512x256 guide/fullraw1774x887/native4x256x128/sourceA/storedRGB privately viewed. Original subdued matte brown panels/diagonal shadow/central boundary/dark right small pointed painted fields retain original wholeUV footprint and source scale. ONE tiny warm sourcepeak37,21 matches exactnative position, sourceRGB123/105/82→105/81/53 (weaker, not white new hardware). Partial gray source mark at far-left remains partial and subdued, no added component or letters claimed. Corr .9886/maxlocal+20 at38,21 adjacent sourceweak mark/weakgain3. Flag fine brushpaint and local adjacent mark softness for coordinator fullraw review; no bright raised seam/weave/photograin. Actual sourceA255 byte-exact/POT256x128, full inverse/scalar/defaultpad only, no BBox/crop/artRGB/retry. Not worker accepted/packed claim.', 7659: 'Ready only coordinator final native material review. Full original128x64/sourceRGBA/storedRGB wholeNEAREST4x512x256 guide/fullraw1774x887/native4x512x256/sourceA/storedRGB privately viewed. Exact two original central brown triangular painted fields, surrounding matte grayblue painted field and original darkrounded right shadow footprint retained wholeUV/count/scale. Original uppergray sourcepeak30,9 stays exactnative position RGB173/182/181→182/186/183 (max+9R), original tinybrown paintpeak30,29 exact RGB107/105/90→102/91/73 weaker. No new stitch/button/pocket/strap/lens/rim inferred. Corr .9944/maxlocal+26 at31,8 neighboring existinggray highlight/weakgain5. Flag uppergray highlight locally broader/finepaint patch stronger, coordinator final fullraw material judgment needed; faint sourcepaint retained without new weave/hardware/raised edge. Actual A255 byte-exact/POT512x256, entire inverse/scalar only, no artRGB repair/crop/BBox/retry.', 4803: 'Pending native material/paint phase review. Full original64square/sourceRGBA/storedRGB wholeNEAREST8x512square guide/fullraw1254square/native4x256square/sourceA/storedRGB privately viewed. Three source curved-band portions and dark rounded fields broadly retain sourcecanvas positions/count, but original soft dim olive-gray paint becomes coarse oily/brushed curved rims, right bands more geometric, fine grain/relative intensity scale changed. Upper native sourcepeak7,30→7,31 and middle5,52→3,53 shifted1–2pixels; lower26,60 remains. Right irregular measured tonal maxima46/50/52/56/58/62→50/52/56/58/62 (6→5), not a claim of six physical ribs. Corr .9654/maxlocal+38 at3,23 weakcurve shadow/weakgain31. Scalar darkening alone cannot restore original material/phase, no brightness regeneration or RGB patch. SourceA255 byte-exact/POT256square, held raw/exactprompt/call/reason saved. Pending, not permanent refusal or blanket rejection of sourcepaint complexity.'}

records=[];jobs=[]
for texture_id in IDS:
    call_path=BASE/'generated'/f'{texture_id}-equipment-twenty-seventh-call.json'
    call=read(call_path)
    assert sha(ROOT/call['generated'])==call['raw_sha256']
    assert sha(ROOT/call['prompt_file'])==call['prompt_sha256']
    assert (ROOT/call['prompt_file']).read_bytes().decode('utf-8')==call['exact_prompt']
    assert all(sha(ROOT/name)==digest for name,digest in call['reference_sha256'].items())
    assert sha(BASE/'pattern-constraints-equipment-twenty-seventh.json')==call['pattern_constraints_file_sha256']
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    native=Image.open(BASE/'private-equipment-twenty-seventh/native4x'/f'{texture_id}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(d&(d-1)==0 for d in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    status='ready-for-coordinator-art-review'if texture_id in READY else'pending-native-material-review'
    record={'id':texture_id,'status':status,'call_count':1,'review_note':notes[texture_id],'review_concern':notes[texture_id],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':portable(call_path),'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':call['source_transform_recipe'],'source_check':portable(proof_path),'source_before_call_snapshot':portable(source_before),'metrics':metrics[texture_id],'detail_metrics':details[texture_id],'pattern_constraints':constraints[texture_id],'pattern_metrics':patterns[texture_id]}
    if texture_id in READY:
        jobs.append({'id':texture_id,'generated':call['generated'],'prompt':call['exact_prompt'],'source_transform_recipe':call['source_transform_recipe']})
    else:
        rejected=BASE/'rejected'/'equipment-twenty-seventh';rejected.mkdir(parents=True,exist_ok=True)
        for suffix,source in [('raw.png',ROOT/call['generated']),('prompt.txt',ROOT/call['prompt_file']),('call.json',call_path)]:
            target=rejected/f'{texture_id}-attempt1-{suffix}';assert not target.exists();shutil.copyfile(source,target)
        reason=rejected/f'{texture_id}-attempt1-reason.json';reason.write_text(json.dumps({'id':texture_id,'status':status,'reason':notes[texture_id],'raw_sha256':call['raw_sha256'],'one_call_only':True},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        record['held_raw']=portable(rejected/f'{texture_id}-attempt1-raw.png')
    for row in proof:
        if row['id']==texture_id:
            row.update(imagegen_call_count=1,postcall_status=status,full_raw_native4x_source_alpha_storedRGB_privately_reviewed=True,review_note=notes[texture_id])
    records.append(record)
save('jobs-equipment-twenty-seventh.json',jobs)
save('source-check-equipment-twenty-seventh.json',proof)
save('audit-correction-equipment-twenty-seventh.json',{'scope':'Fresh actual Materials/templates/all static consumers independently verified; no missing Particle claim for Material-only sources.','selected_ids':IDS,'actual_native_type':{str(i):selected[i]['texture']['Type']for i in IDS},'actual_source_alpha_extrema':{str(i):list(Image.open(BASE/'original'/f'{i}.png').convert('RGBA').getchannel('A').getextrema())for i in IDS},'note':'Actual native sourceA255 verified per original, not inferred from Ordinary Type. Local light positions/strength asserted before prompts. Curved-field and right-profile evidence4803 does not invent six physical ribs or a global ideal grid. Source2378 source-only based on exact constant wholeRGBA/historyrelease/actualtyped proof, not blanket surface simplicity. No false hardware/glyph/staticModel/Races/meshUV inference.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'source_only2378_proof':portable(BASE/'source-only2378-equipment-twenty-seventh.json'),'generation_calls':3,'retries':0})
save('survey-equipment-twenty-seventh.json',{'scope':'Original-only full visual survey; no fresh typed/historical/release parity claim for other unselected IDs except separate actual constant2378 proof. No blanket remaining-group suitability/exhaustion claim.','viewed_unselected_ids':[5235,5236,5237,5241,5242,5243,5244,5246,5248,5249,5250,5251,5252,2378,2968,2984,3384,3784,4257,4867,4948,5676,5677,6901,6902],'call_count':0,'source_only2378_proof':portable(BASE/'source-only2378-equipment-twenty-seventh.json'),'note':'Some full sources show explicit text/glyphs/badges, others are genuine complex artwork not selected in this small scope. Repeating pattern/simple surface itself never prohibits a fresh faithful edit.2378 exactRGBA constant evidence independently verified, kept original-only with0calls.'})

exclusions=read(BASE/'exclusions-equipment-twenty-seventh.json')
assert not set(IDS).intersection(exclusions['ids'])
assert all(row['source_rgba_sha256']not in exclusions['blocked_hashes']for row in proof)
source_path=BASE.parent/'sources.json';queue_path=BASE/'queue.json';sbytes=source_path.read_bytes();qbytes=queue_path.read_bytes();sources=json.loads(sbytes.decode('utf-8'))['textures'];queue=json.loads(qbytes.decode('utf-8'));own=[{'id':r['id'],'status':r.get('status')}for r in sources+queue if r['id']in IDS]
assert not own,own
save('final-guards-equipment-twenty-seventh.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[exclusions['accepted_count'],exclusions['queue_count']],'postcall_readonly_counts':[len(sources),len(queue)],'selected_beforecall_allids_and_calledRGBA_absent':True,'own_postcall_sources_queue_overlap':own,'source_snapshot_sha256':hashlib.sha256(sbytes).hexdigest(),'queue_snapshot_sha256':hashlib.sha256(qbytes).hexdigest(),'shared_writes':False,'generated_ids':IDS,'call_count':3,'repeat_calls':0})
save('review-equipment-twenty-seventh.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':IDS,'generated_ids':IDS,'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'call_count':3,'source_only_ids':[2378],'source_only_records':[{'id':2378,'status':'source-only-exact-constant-color-field','call_count':0,'proof':portable(BASE/'source-only2378-equipment-twenty-seventh.json')}],'records':records,'source_parity':'Fresh read-only actual Materials.TextureID/templates/all actual Model consumers/geometries/templates and all Particle TextureN/nonparticle bindings exact sourcequeue; historicalMMP/release/existing original native RGBA bytes/dims/hash identical. Missing runtime meshUV/dispatch unknown, no fake semantic proof.','QA':'Every full original/helper/raw/native4x storedRGB and originalA restored preview inspected privately. Original fullcanvas sourceA byte-exact, pure scalar calibrated entire inverse only; diagnostics do not alter art or UV. All targets physicalPOT.','ready_semantics':'Only coordinator review candidates, never accepted/packed worker claim. Held records remain pending, not permanent refusal.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'pattern_before_call_proof':portable(BASE/'pattern-constraints-equipment-twenty-seventh.json'),'material_metrics':portable(BASE/'material-metrics-equipment-twenty-seventh.json'),'audit_correction':portable(BASE/'audit-correction-equipment-twenty-seventh.json'),'final_guards':portable(BASE/'final-guards-equipment-twenty-seventh.json'),'unselected_original_only_survey':portable(BASE/'survey-equipment-twenty-seventh.json'),'ownership':'Independent Eq25 files only. Stable Eq24/prior packages/shared metadata/import/calibration/Git/Game/C++ untouched.'})
manifest_path=BASE/'sha256-equipment-twenty-seventh.json';files=sorted(BASE.glob('*equipment-twenty-seventh*.json'))+sorted((BASE/'generated').glob('*equipment-twenty-seventh-call.json'));manifest={portable(path):sha(path)for path in files if path!=manifest_path};save(manifest_path.name,manifest);assert all(sha(ROOT/name)==digest for name,digest in read(manifest_path).items())
print(json.dumps({'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'calls':3,'manifest_verified':len(manifest),'stable_SHA256':{name:sha(BASE/f'{name}-equipment-twenty-seventh.json')for name in ['jobs','review','source-check','selected','sha256']}},ensure_ascii=False))

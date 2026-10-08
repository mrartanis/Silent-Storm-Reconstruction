from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import shutil
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
IDS=[2448,3051,3041]
READY=[2448,3051]
def read(path):return json.loads(path.read_text(encoding='utf-8'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def portable(path):return path.relative_to(ROOT).as_posix()
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

metrics={r['id']:r for r in read(BASE/'metrics-equipment-twenty-sixth.json')}
details={r['id']:r for r in read(BASE/'detail-metrics-equipment-twenty-sixth.json')}
patterns={r['id']:r for r in read(BASE/'pattern-metrics-equipment-twenty-sixth.json')}
constraints={r['id']:r for r in read(BASE/'pattern-constraints-equipment-twenty-sixth.json')}
proof_path=BASE/'source-check-equipment-twenty-sixth.json'
source_before=BASE/'source-check-equipment-twenty-sixth-before-call.json'
assert not source_before.exists()
source_before.write_bytes(proof_path.read_bytes())
proof=read(proof_path)
selected={r['id']:r for r in read(BASE/'selected-equipment-twenty-sixth.json')}

domains={2448:[('upper dark painted fields',[0,0,64,22]),('three weak highlights/middle brown field',[0,22,64,35]),('lower broad brown field',[0,35,64,64])],3051:[('whole muted field',[0,0,32,32]),('lower shaded edge',[0,28,32,32]),('upper-center dark patch',[8,0,22,14])],3041:[('eleven fine upper painted strokes',[13,8,58,45]),('upper broad golden paint',[75,3,255,49]),('irregular lower crossed pattern',[0,72,75,115]),('soft lower redbrown shape',[80,62,227,117])]}
materials=[]
for texture_id in IDS:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    source=np.array(original.convert('RGB'),dtype=float)
    candidate=np.array(Image.open(BASE/'private-equipment-twenty-sixth/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(original.size,Image.Resampling.LANCZOS),dtype=float)
    row={'id':texture_id,'scope':'Fixed native diagnostic domains on ENTIRE inverse-resized candidate; not ROI editing/registration/source-RGB artistic insertion.','source_alpha_extrema':list(original.getchannel('A').getextrema()),'domains':[]}
    for name,box in domains[texture_id]:
        x0,y0,x1,y1=box;s=source[y0:y1,x0:x1];g=candidate[y0:y1,x0:x1]
        row['domains'].append({'name':name,'native_bbox':box,'source_mean_RGB':s.mean((0,1)).tolist(),'candidate_mean_RGB':g.mean((0,1)).tolist(),'source_luma_std':float(s.mean(2).std()),'candidate_luma_std':float(g.mean(2).std())})
    materials.append(row)
save('material-metrics-equipment-twenty-sixth.json',materials)

notes={2448: 'Ready only coordinator final native material review. Full original64square/sourceRGBA/sourceRGB wholeNEAREST8x512square guide/fullraw1254square/native4x256square/sourceA/storedRGB privately viewed. Original muted brown upper dark fields/middle long weak horizontal boundary/lower broad brown polygon preserved in exact wholeUV/materialscale. Three tiny unequal warm highlights preserve EXACTnative peaks19,24/22,24/29,23: source181/142/99→181/135/89,134/90/57→115/69/39,189/142/74→176/127/69 (same or weaker originalpeakstrength, no added white fastener). Fine painted surface and short gray-black upper-left mark retain source positions. Corr .9872/maxlocal+29 at20,23 adjacent to existingmark/weakgain32; flag modest stronger local adjacent shading and fine brushgrain, final coordinator art judgment. SourceA255 byte-exact/POT256square/wholecanvas scalar only, no BBox/crop/artRGB/retry. Not worker accepted/packed claim.', 3051: 'Ready only coordinator final native material review. Full original32square/sourceRGBA/sourceRGB wholeNEAREST16x512square guide/fullraw1254square/native4x128square/sourceA/storedRGB privately viewed. Whole muted gray-beige/brown irregular broadpaint field/source lowcontrast tonal footprint retained, no new seams/stripes/hardware/raised rim. No invented repeated pattern or count. Native luma std11.99→10.63, source/candidate meanRGB124.8/114.9/94.5→127.7/114.5/89.84, gentle warmer subdued surface. Corr .9754/maxlocal+17 on existingbottomedge/weakgain0. Flag bottomedge locallylighter and source darkest min62.67→75, modest fine paint texture visible in fullraw; coordinator final fullraw/material review needed. Exact sourceA255/POT128square, full inverse/scalar only, no artistic RGB repair/retry. Native32square source is not sameRGBA/dimensions as670; no donor/alias claim.', 3041: 'Pending native material/pattern review despite genuine painted source. Full original256x128/sourceRGBA/sourceRGB wholeNEAREST2x512x256 guide/fullraw1774x887/native4x1024x512/sourceA/storedRGB privately viewed. Eleven upper stroke native maxima16,20,...56 retain exactcount/pitch4/phase, but faint sourcepaint interpreted into bright capped raised/glinty rods; original lower crossed pattern becomes coarser/darker geometric diamondpaint. Actual irregular core row94 source maxima6,13,20,27,33,41,47,53,60,67→candidate7,9,14,21,28,34,42,47,53,60,67:10→11 measured extrema/partial1nativephase shift; this diagnostic is not an invented global physical cell count. Soft redbrown shape/fullcanvas remains, but material/phase requires hold. Corr .9905/maxlocal+53/weakgain594. SourceA255 byte-exact/POT1024x512, no retry/BBox/artRGB/crop/wholepattern regularization fix. Pending, not permanent refusal or blanket rejection of patterns.'}

records=[];jobs=[]
for texture_id in IDS:
    call_path=BASE/'generated'/f'{texture_id}-equipment-twenty-sixth-call.json'
    call=read(call_path)
    assert sha(ROOT/call['generated'])==call['raw_sha256']
    assert sha(ROOT/call['prompt_file'])==call['prompt_sha256']
    assert (ROOT/call['prompt_file']).read_bytes().decode('utf-8')==call['exact_prompt']
    assert all(sha(ROOT/name)==digest for name,digest in call['reference_sha256'].items())
    assert sha(BASE/'pattern-constraints-equipment-twenty-sixth.json')==call['pattern_constraints_file_sha256']
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    native=Image.open(BASE/'private-equipment-twenty-sixth/native4x'/f'{texture_id}-calibrated-native-alpha-private.png').convert('RGBA')
    assert native.size==(original.width*4,original.height*4)
    assert all(d&(d-1)==0 for d in native.size)
    assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
    status='ready-for-coordinator-art-review'if texture_id in READY else'pending-native-material-review'
    record={'id':texture_id,'status':status,'call_count':1,'review_note':notes[texture_id],'review_concern':notes[texture_id],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':portable(call_path),'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':call['source_transform_recipe'],'source_check':portable(proof_path),'source_before_call_snapshot':portable(source_before),'metrics':metrics[texture_id],'detail_metrics':details[texture_id],'pattern_constraints':constraints[texture_id],'pattern_metrics':patterns[texture_id]}
    if texture_id in READY:
        jobs.append({'id':texture_id,'generated':call['generated'],'prompt':call['exact_prompt'],'source_transform_recipe':call['source_transform_recipe']})
    else:
        rejected=BASE/'rejected'/'equipment-twenty-sixth';rejected.mkdir(parents=True,exist_ok=True)
        for suffix,source in [('raw.png',ROOT/call['generated']),('prompt.txt',ROOT/call['prompt_file']),('call.json',call_path)]:
            target=rejected/f'{texture_id}-attempt1-{suffix}';assert not target.exists();shutil.copyfile(source,target)
        reason=rejected/f'{texture_id}-attempt1-reason.json';reason.write_text(json.dumps({'id':texture_id,'status':status,'reason':notes[texture_id],'raw_sha256':call['raw_sha256'],'one_call_only':True},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        record['held_raw']=portable(rejected/f'{texture_id}-attempt1-raw.png')
    for row in proof:
        if row['id']==texture_id:
            row.update(imagegen_call_count=1,postcall_status=status,full_raw_native4x_source_alpha_storedRGB_privately_reviewed=True,review_note=notes[texture_id])
    records.append(record)
save('jobs-equipment-twenty-sixth.json',jobs)
save('source-check-equipment-twenty-sixth.json',proof)
save('audit-correction-equipment-twenty-sixth.json',{'scope':'Fresh actual Materials/consumers independently verified; no missing Particle claim for Material-only sources.','selected_ids':IDS,'actual_native_type':{str(i):selected[i]['texture']['Type']for i in IDS},'actual_source_alpha_extrema':{str(i):list(Image.open(BASE/'original'/f'{i}.png').convert('RGBA').getchannel('A').getextrema())for i in IDS},'note':'All three actual sourceA255 verified from native originals, not inferred from Ordinary Type. Source3051 is32square nativeRGBA distinct from67064square; no false alias/donor/staticModel/Races/meshUV inference. Upper eleven strokes3041 exactpitch beforeprompt, irregular lower core maxima are measured native paint positions rather than fictitious global physical grid count. Actual source parity/typed evidence preserved separately.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'generation_calls':3,'retries':0})
save('survey-equipment-twenty-sixth.json',{'scope':'Original-only visual survey, no fresh typed/historical/release parity claim for unselected IDs. No blanket remaining-group suitability or exhaustion claim.','viewed_unselected_ids':[675,676,3783,6668,6669,7005,7653,3914,3047,1915,1916,1918,1920,1921,1976,1977,5379,5389],'call_count':0,'note':'These full originals inspected privately. Some show tiny label/badge/symbols, others are genuine complex art not selected in this small scope. No claim that complexity itself makes the whole remaining group unsuitable. All unselected survey IDs zero imagegen calls.'})

exclusions=read(BASE/'exclusions-equipment-twenty-sixth.json')
assert not set(IDS).intersection(exclusions['ids'])
assert all(row['source_rgba_sha256']not in exclusions['blocked_hashes']for row in proof)
source_path=BASE.parent/'sources.json';queue_path=BASE/'queue.json';sbytes=source_path.read_bytes();qbytes=queue_path.read_bytes();sources=json.loads(sbytes.decode('utf-8'))['textures'];queue=json.loads(qbytes.decode('utf-8'));own=[{'id':r['id'],'status':r.get('status')}for r in sources+queue if r['id']in IDS]
assert not own,own
save('final-guards-equipment-twenty-sixth.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[exclusions['accepted_count'],exclusions['queue_count']],'postcall_readonly_counts':[len(sources),len(queue)],'selected_beforecall_allids_and_calledRGBA_absent':True,'own_postcall_sources_queue_overlap':own,'source_snapshot_sha256':hashlib.sha256(sbytes).hexdigest(),'queue_snapshot_sha256':hashlib.sha256(qbytes).hexdigest(),'shared_writes':False,'generated_ids':IDS,'call_count':3,'repeat_calls':0})
save('review-equipment-twenty-sixth.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':IDS,'generated_ids':IDS,'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'call_count':3,'records':records,'source_parity':'Fresh read-only actual Materials.TextureID/templates/all actual Model consumers/geometries/templates and all Particle TextureN/nonparticle bindings exact sourcequeue; historicalMMP/release/existing original native RGBA bytes/dims/hash identical. Missing runtime meshUV/dispatch unknown, no fake semantic proof.','QA':'Every full original/helper/raw/native4x storedRGB and originalA restored preview inspected privately. Original fullcanvas sourceA byte-exact, pure scalar calibrated entire inverse only; diagnostics do not alter art or UV. All targets physicalPOT.','ready_semantics':'Only coordinator review candidates, never accepted/packed worker claim. Held records remain pending, not permanent refusal.','source_before_call_snapshot':portable(source_before),'source_before_call_sha256':sha(source_before),'pattern_before_call_proof':portable(BASE/'pattern-constraints-equipment-twenty-sixth.json'),'material_metrics':portable(BASE/'material-metrics-equipment-twenty-sixth.json'),'audit_correction':portable(BASE/'audit-correction-equipment-twenty-sixth.json'),'final_guards':portable(BASE/'final-guards-equipment-twenty-sixth.json'),'unselected_original_only_survey':portable(BASE/'survey-equipment-twenty-sixth.json'),'ownership':'Independent Eq25 files only. Stable Eq24/prior packages/shared metadata/import/calibration/Git/Game/C++ untouched.'})
manifest_path=BASE/'sha256-equipment-twenty-sixth.json';files=sorted(BASE.glob('*equipment-twenty-sixth*.json'))+sorted((BASE/'generated').glob('*equipment-twenty-sixth-call.json'));manifest={portable(path):sha(path)for path in files if path!=manifest_path};save(manifest_path.name,manifest);assert all(sha(ROOT/name)==digest for name,digest in read(manifest_path).items())
print(json.dumps({'ready_ids':READY,'pending_ids':[i for i in IDS if i not in READY],'calls':3,'manifest_verified':len(manifest),'stable_SHA256':{name:sha(BASE/f'{name}-equipment-twenty-sixth.json')for name in ['jobs','review','source-check','selected','sha256']}},ensure_ascii=False))

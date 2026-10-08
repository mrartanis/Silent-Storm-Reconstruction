from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
STEM='equipment-thirty-second'
GENERATED=[5378,5389,5404]
READY=[5389,5404]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def portable(p):return p.relative_to(ROOT).as_posix()
def save(name,data):(BASE/name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

proof_path=BASE/f'source-check-{STEM}.json'
before=BASE/f'source-check-{STEM}-before-call.json'
proof=read(proof_path)
assert proof==read(before)
scope=read(BASE/f'scope-{STEM}.json')
selected=read(BASE/f'selected-{STEM}.json')
assert [r['id']for r in selected]==scope['ids'] and len(selected)==12
metrics={r['id']:r for r in read(BASE/f'metrics-{STEM}.json')}
details={r['id']:r for r in read(BASE/f'detail-metrics-{STEM}.json')}
materials={r['id']:r for r in read(BASE/f'material-pattern-metrics-{STEM}.json')}
tiny={r['id']:r for r in read(BASE/f'tiny-marks-review-{STEM}.json')}
notes={5378: 'Pending coordinator native material review, not permanent refusal. Full native64square/sourceRGB NN8x512/raw1254/native4x256/sourceA/storedRGB privately inspected. Original seven source bitmap paintfields/count/layout retained; gray/red/brown/pale/warm fields and sourceblack gaps preserved, no new text semantics claimed. Actual sourceOrdinary A26..255, TWO actual Materials with Alpha transparent and opaque/Clamp (no inferred A255 from Type); fulloriginalA4x byte-exact. But source large gray soft wash appears as more pronounced etched/scratched/coarse brushed paint in fullraw, and originally smooth red soft field gains unrelated finer mottle. Native gray domain mean67.95→63.20/HPstd4.79→6.42/HPcorr.797; redmean30.78→33.43/HPstd1.34→2.52/HPcorr.509. Lowerbrown paint remains source-linked(.944), existing marks stronger/softer thresholds diagnostic only. Whole corr.9829/maxlocal+32/weakgain38. Hold concerns source surface/wash material, not brightness/threshold1px alone; root may independently review. One solecall/raw/exactprompt/call copied rejected, no repeat/artistRGB/crop/BBox/optout; defaultscalar/sourceA retained.', 5389: 'Ready only coordinator native art/material review. Full native64x32/sourceRGB NN8x512x256/raw1774x887/native4x256x128/sourceA/storedRGB privately inspected. Exactly four original visible paintfield groups retained: large left irregular grainygray field, lower-middle softbrown field, lower-right small grainygray field, upper-right wide brownfield. Upper TWO existing light tonal bands retain native peaksy1/y4, no new bands/rings. Fine original pigment grain preserves source phase/scale, no invented rocks/chunks/pores or circular relief. Leftgray HPcorr.929/mean51.36→53.81/std13.60→14.58; brownHPcorr.919/mean58.38→55.92/std6.09→8.08; rightgrayHPcorr.951/mean100.71→102.50/std16.82→14.87. Wholecorr.9842/maxlocal+20/weakgain20. Explicit caveat finer/sharper original grain and softbrown contrast slightly stronger, moderate source-linked paint not automatic hold; root final decision. ActualA255/POT256x128/fulloriginalA4x byte-exact, whole scalar/defaultpad only, no packed/accepted worker claim.', 5404: 'Ready only coordinator native art/material review. Full native32x64/sourceRGB NN8x256x512/raw887x1774/native4x128x256/sourceA/storedRGB privately inspected. Original DIM grayblue wash/diagonal top shading/narrow soft contours/long lower rectangular outline/sourceblack same whole canvas, no invented raised threads/ribs/bevel/gloss/hardware. Source7 soft tonal maxima y10/12/14/16/18/20/22 become6 after inverse: weak firsty10(luma99 vs preceding97) smooths into top transition, remaining6 maxima12/14/16/18/20/22 retain exact phase. These are tonal ripples, not7physicalpart counts; weak threshold/1px smoothing is not sole hold. Actual source bottom feature native x17/18/19,y57 is ORIGINAL RGBA[0,0,0,0], fullsourceA already carries this opening: not originally positive-alpha dim paint converted to new hole. Sourcebefore wording paintmark described visible bitmap, no physical tool/slot semantics inferred; root alpha unchanged. Inverse storedRGB can mix padding under A0 and remains invisible; no artRGB patch. Actual Ordinary A0..255/alpha_test Clamp/fulloriginalA4x byte-exact/POT128x256. Lowerwash HPcorr.983/mean80.76→82.43, topwashHPcorr.967/mean85.84→85.84; wholecorr.9937/maxlocal+11/weakgain0. Sourcewash softness/material retained, slight tonal smoothing caveat, root final art decision.'}
jobs=[];records=[]
for i in GENERATED:
 call_path=BASE/f'generated/{i}-{STEM}-call.json';call=read(call_path)
 assert sha(ROOT/call['generated'])==call['raw_sha256']
 assert (ROOT/call['prompt_file']).read_bytes()==call['exact_prompt'].encode('utf-8')
 assert sha(ROOT/call['prompt_file'])==call['prompt_sha256']
 assert all(sha(ROOT/p)==h for p,h in call['reference_sha256'].items())
 assert sha(BASE/f'pattern-constraints-{STEM}.json')==call['pattern_constraints_file_sha256']
 original=Image.open(BASE/f'original/{i}.png').convert('RGBA')
 native=Image.open(BASE/f'private-{STEM}/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
 assert native.size==(original.width*4,original.height*4) and all(x&(x-1)==0 for x in native.size)
 assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
 status='ready-for-coordinator-art-review'if i in READY else'pending-native-material-review'
 rec={'id':i,'status':status,'call_count':1,'review_note':notes[i],'review_concern':notes[i],'generated':call['generated'],'raw_sha256':call['raw_sha256'],'prompt_file':call['prompt_file'],'prompt_sha256':call['prompt_sha256'],'call_metadata':portable(call_path),'reference_pngs':call['reference_pngs'],'reference_sha256':call['reference_sha256'],'source_transform_recipe':call['source_transform_recipe'],'metrics':metrics[i],'detail_metrics':details[i],'material_metrics':materials[i],'supplemental_tiny_marks_review':tiny.get(i),'source_before_call':portable(before)}
 if i in READY:jobs.append({'id':i,'generated':call['generated'],'prompt':call['exact_prompt'],'source_transform_recipe':call['source_transform_recipe']})
 else:
  rejected=BASE/'rejected'/STEM;rejected.mkdir(parents=True,exist_ok=True)
  for suffix,p in [('raw.png',ROOT/call['generated']),('prompt.txt',ROOT/call['prompt_file']),('call.json',call_path)]:
   dest=rejected/f'{i}-attempt1-{suffix}';assert not dest.exists();shutil.copyfile(p,dest)
  save(f'rejected/{STEM}/{i}-attempt1-reason.json',{'id':i,'status':status,'reason':notes[i],'raw_sha256':call['raw_sha256'],'call_count':1,'not_permanent_refusal':True})
  rec['held_raw']=portable(rejected/f'{i}-attempt1-raw.png')
 for r in proof:
  if r['id']==i:r.update(imagegen_call_count=1,postcall_status=status,review_note=notes[i],full_raw_native4x_source_alpha_storedRGB_privately_reviewed=True)
 records.append(rec)
for r in proof:
 if r['id']not in GENERATED:records.append({'id':r['id'],'status':'genuine-art-pending-source-detail-proof','call_count':0,'reason':r['source_only_reason'],'source_check':portable(before)})
save(f'jobs-{STEM}.json',jobs)
save(f'source-check-{STEM}.json',proof)
ex=read(BASE/f'exclusions-{STEM}.json')
assert not set(scope['ids']).intersection(ex['ids'])
assert all(r['source_rgba_sha256']not in ex['blocked_hashes']for r in proof)
sp=BASE.parent/'sources.json';qp=BASE/'queue.json';sb=sp.read_bytes();qb=qp.read_bytes();sources=json.loads(sb.decode('utf-8'))['textures'];q=json.loads(qb.decode('utf-8'))
own=[{'id':r['id'],'status':r.get('status')}for r in sources+q if r['id']in scope['ids']]
save(f'final-guards-{STEM}.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[ex['accepted_count'],ex['queue_count']],'postcall_source_entry_count':len(sources),'postcall_queue_entry_count':len(q),'postcall_queue_status_counts':{status:sum(r.get('status')==status for r in q)for status in sorted({str(r.get('status'))for r in q})},'validated_queue_count':sum(r.get('status')=='validated'for r in q),'source_count_semantics':'Source entries include generated-pending-validation; only queue validated status is counted as validated, root native validation remains authoritative.','own_postcall_overlap':own,'precall_all_selected_ID_and_identical_called_RGBA_absent':True,'all_jobs_raw_prompts_calls_selected_and_shared_q_guarded':True,'sources_sha256':hashlib.sha256(sb).hexdigest(),'queue_sha256':hashlib.sha256(qb).hexdigest(),'shared_writes':False,'calls':3,'retries':0})
save(f'review-{STEM}.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':scope['ids'],'generated_ids':GENERATED,'ready_ids':READY,'pending_called_ids':[5378],'pending_uncalled_ids':[i for i in scope['ids']if i not in GENERATED],'call_count':3,'records':records,'source_parity':'All twelve fresh actual Materials.TextureID/templates/all actual Model consumers and Particle/nonparticle bindings checked against typed sourcequeue. HistoricalMMP/release/existing original fullRGBA dimensions/hash exact; actual source Type/alpha recorded. Mesh UV/dynamic dispatch unknown where not directly proven, no fake semantics.','QA':'Every generated fullraw/source/helper/native4x sourceA/storedRGB privately inspected. ENTIRE raw inverse4x, pure scalar/default source-mask padding/fulloriginalA only; no artRGB, crop, BBox, UVfit, retry or optout. OriginalA4x byte exact and all generated native targets POT. Exact original native sevenfield layout/51faint stain pixel coordinates,2warmbands and7soft tonal maxima executed and persisted before prompts/calls. Supplemental candidate/source phase/alpha/material diagnostics saved postcall and honestly labelled; first-ever readonly view never claimed. Current sourceentries may include root generated-pending-validation, not all accepted.','source_before_call':portable(before),'source_before_call_sha256':sha(before),'pattern_before_call':portable(BASE/f'pattern-constraints-{STEM}.json'),'supplemental_tiny_marks':portable(BASE/f'tiny-marks-review-{STEM}.json'),'final_guards':portable(BASE/f'final-guards-{STEM}.json'),'ownership':'Own new Eq32 independent files only. All old stable packages/shared metadata/build/Game/C++/Git untouched. Ready means review candidate; no acceptance/pack claim.'})
mp=BASE/f'sha256-{STEM}.json';files=sorted(BASE.glob(f'*{STEM}*.json'))+sorted((BASE/'generated').glob(f'*{STEM}-call.json'));manifest={portable(p):sha(p)for p in files if p!=mp};save(mp.name,manifest);assert all(sha(ROOT/p)==h for p,h in read(mp).items())
print(json.dumps({'ready':READY,'held_called':[5378],'uncalled_pending':9,'calls':3,'manifest_entries':len(manifest),'sha256':{n:sha(BASE/f'{n}-{STEM}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

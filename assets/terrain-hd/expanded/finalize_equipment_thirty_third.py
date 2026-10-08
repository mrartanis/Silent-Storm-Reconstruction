from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
STEM='equipment-thirty-third'
GENERATED=[5387,5408,5438]
READY=[]
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
notes={5387: 'Pending coordinator material review, not permanent refusal. Full native64x32/sourceRGB NN8x512x256/raw1774x887/native4x256x128/sourceA/storedRGB privately inspected. Exactly EIGHT source pale square fields TWO rows FOUR columns plus ONE lower-right round field and original mottled gray/brown background count/layout retained; no new holes/parts. Actual Ordinary/sourceA255/opaque Clamp; fullA4x byte exact/POT. But raw adds visibly unrelated fine mottled paint/paper-like grain onto originally smooth square fields, contrary to source softness and flatness. Their native4x HPstd source0.605..1.093 becomes3.725..4.688, HPcorr .080.. .221; this is a diagnostic supporting actual fullraw material concern, no invented universal acceptance threshold. Source squares remain source positions, some local grayscale relation changes: field1mean93.70→103.96, field5mean94.21→106.38, overall inversecorr.9896/localmax+21/weakgain1. Hold is new surface grain/flat-field material, not moderate brightness/one-pixel smoothing alone. Root may independently review. Wholecanvas pure defaultpad/scalar/fulloriginalA only; one call, no retry or artRGB/crop/BBox/optout.', 5408: 'Pending coordinator material review, not permanent refusal. Full native32x64/sourceRGB NN8x256x512/raw887x1774/native4x128x256/sourceA/storedRGB privately inspected. Original narrow left stroke, irregular upper notched field, lower soft rounded field/weak center all same whole UV. Actual Ordinary/sourceA0..255/alpha_test Clamp; source A0 contours and original center retained byte-exact4x. Center source A0 is already present; candidate does not introduce new native alpha hole. But fullraw turns original muted soft brown pale-edge paint into brighter harder raised-looking rims and strengthens the top left stroke. Nativeinverse source pixel3,1 RGB161/139/110→217/194/158(+56), top-notch15,14 +45; actual material/relative-edge strength concern. Domains mean near source but HPcorr .419/.458/.383 and finer textured paint/new harder edges visible independently of scalar; inversewholecorr.9802/weakgain20. Hold is material/rim strength, not isolated smoothing or detail count. Exact originalA4x/POT; defaultpad/fullscalar only; one call, no retry/artRGB/crop/BBox/optout.', 5438: 'Pending coordinator material review, not permanent refusal. Full native128x32 and original opaqueRGB/helper1536x512 affine3:1 privately viewed before call; raw2172x724 and entire inverse native4x512x128/sourceA/storedRGB inspected. Fullcanvas affine/inverse only, no crop/padding/BBox fitting. Actual Ordinary/sourceA255/opaque Clamp; exact sourceA4x/POT. Original narrow left gray strip, broad middle one-native-pixel checker field and TWO right dark markings retained. Exact core checker maxima per native row preserved25/28; remaining3 weak maxima interruptions differ, diagnostic only, not automatic count-drift hold. Fullraw adds harder pale scratch-like treatment to the existing weak left checker paint patch, stronger mottled/scratched left gray wash and a stronger textured raised-looking surround of the original lower-right round mark. Concern is original plain matte material/detail strength, not source repeat prohibition or new alpha hole. Middlemean96.67→95.53, left49.31→50.29/right49.14→52.63, but native4x HPstd left.881→3.613/right1.571→4.881, HPcorr .266/.336; inversewholecorr.9655/maxlocal+39/weakgain32. The underlying patch/mark positions originate from source; not claiming all stronger flecks are new geometric components. Root may independently review. One call/fullcanvas defaultscalar/fullsourceA, no retry/artRGB/optout.'}
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
save(f'review-{STEM}.json',{'status':'stable-empty-ready-subset-awaiting-coordinator-native-material-review','selected_ids':scope['ids'],'generated_ids':GENERATED,'ready_ids':READY,'pending_called_ids':[5387,5408,5438],'pending_uncalled_ids':[i for i in scope['ids']if i not in GENERATED],'call_count':3,'records':records,'source_parity':'All twelve fresh actual Materials.TextureID/templates/all actual Model consumers and Particle/nonparticle bindings checked against typed sourcequeue. HistoricalMMP/release/existing original fullRGBA dimensions/hash exact; actual source Type/alpha recorded. Mesh UV/dynamic dispatch unknown where not directly proven, no fake semantics.','QA':'Every generated fullraw/source/helper/native4x sourceA/storedRGB privately inspected. ENTIRE raw inverse4x, pure scalar/default source-mask padding/fulloriginalA only; no artRGB, crop, BBox, UVfit, retry or optout. OriginalA4x byte exact and all generated native targets POT. Exact original native8square+1round fields, soft notched/rounded/left-stroke bitmap fields, whole sourceA and EVERY core fine-checker native row phase were executed and persisted before prompts/calls. Supplemental candidate/source phase/alpha/material diagnostics saved postcall and honestly labelled; first-ever readonly view never claimed. Current sourceentries may include root generated-pending-validation, not all accepted.','source_before_call':portable(before),'source_before_call_sha256':sha(before),'pattern_before_call':portable(BASE/f'pattern-constraints-{STEM}.json'),'supplemental_tiny_marks':portable(BASE/f'tiny-marks-review-{STEM}.json'),'final_guards':portable(BASE/f'final-guards-{STEM}.json'),'ownership':'Own new Eq33 independent files only. All old stable packages/shared metadata/build/Game/C++/Git untouched. Ready means review candidate; no acceptance/pack claim.'})
mp=BASE/f'sha256-{STEM}.json';files=sorted(BASE.glob(f'*{STEM}*.json'))+sorted((BASE/'generated').glob(f'*{STEM}-call.json'));manifest={portable(p):sha(p)for p in files if p!=mp};save(mp.name,manifest);assert all(sha(ROOT/p)==h for p,h in read(mp).items())
print(json.dumps({'ready':READY,'held_called':[5387,5408,5438],'uncalled_pending':9,'calls':3,'manifest_entries':len(manifest),'sha256':{n:sha(BASE/f'{n}-{STEM}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

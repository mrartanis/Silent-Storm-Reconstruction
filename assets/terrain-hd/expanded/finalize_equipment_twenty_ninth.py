from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
STEM='equipment-twenty-ninth'
GENERATED=[3047,2984,3000]
READY=[2984,3000]
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
notes={
3047:'Pending native material/relative-strength review. Original left blade/brown handle/right sheath whole layout remains, but hilt gold highlight becomes a brighter raised-looking edge; source165/146/123 at native36,83 becomes232/201/156 (+67). Source dim right sheath crossed markings become more explicit sharp light lacing; fine source matte shading rendered with more coherent mottled paint and stronger material relief. Whole storedRGB correlation .9899, weak-source gain pixels228. This is real material/light-hierarchy concern, not an automatic 1px smoothing hold. No exact crossed-stroke count is asserted from ambiguous source black fragments. All sourceA255 and UV regions retained; no retries or sourceRGB repair.',
2984:'Ready only for coordinator native material/art review. Full128square original/sourceRGB guide512/raw1254/native512 reviewed. Three original vertical strap-like fields retained; each has three bright row bands at exact native row support: left44/50/53, first upper-right30/37/40, second upper-right30/37/40. Broad olive cloth/orange lower-left field/weak bottom gray curve and black regions remain at source UV/scale. Actual Ordinary sourceA0..255 (not inferred A255), full originalA4x byte exact/default RGB padding. Fine painted cloth shading follows source; grain highpass correlations .926/.907 across two broad domains, means39.59→38.48 and94.38→92.33. Whole storedRGB corr .9903; maximum local+46 and83 weak pixels lifted mostly existing edges, caveat for coordinator. Some tiny pale bars weaker; no added buckle/strap/button/crease/photoweave. Not worker accepted/packed claim.',
3000:'Ready only for coordinator native material/art review. Full128square original/sourceRGB guide512/raw1254/native512 reviewed. Four original vertical strap-like fields retained: each upper field has two bright row bands at25/28; left lower has53/[56,58)/59; right lower62/[65,67)/68. Native contiguous bright row support exact after inverse pure scalar. Existing tiny pale cluster at100,44..46 retained, no additional hardware. Original olive cloth groups/two lower-left brown fields/black UV and count/scale remain. Source broad painted shading remains with finer directional brush appearance; highpass correlations .690 leftcloth/.905 lowercloth, domain means58.31→58.63 and83.10→81.94. Whole storedRGB corr .9804, maxlocal+39 and58 weak pixels lifted, smoothing and narrow highlight strength caveat for coordinator; no new weave/creases/photographic grain. Actual Ordinary sourceA255 byte exact/POT512square. Not worker accepted/packed claim.'}
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
save(f'final-guards-{STEM}.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[ex['accepted_count'],ex['queue_count']],'postcall_readonly_counts':[len(sources),len(q)],'own_postcall_overlap':own,'precall_all_selected_ID_and_identical_called_RGBA_absent':True,'all_jobs_raw_prompts_calls_selected_and_shared_q_guarded':True,'sources_sha256':hashlib.sha256(sb).hexdigest(),'queue_sha256':hashlib.sha256(qb).hexdigest(),'shared_writes':False,'calls':3,'retries':0})
save(f'review-{STEM}.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':scope['ids'],'generated_ids':GENERATED,'ready_ids':READY,'pending_called_ids':[3047],'pending_uncalled_ids':[i for i in scope['ids']if i not in GENERATED],'call_count':3,'records':records,'source_parity':'All twelve fresh actual Materials.TextureID/templates/all actual Model consumers and Particle/nonparticle bindings checked against typed sourcequeue. HistoricalMMP/release/existing original fullRGBA dimensions/hash exact; actual source Type/alpha recorded. Mesh UV/dynamic dispatch unknown where not directly proven, no fake semantics.','QA':'Every generated fullraw/source/helper/native4x sourceA/storedRGB privately inspected. ENTIRE raw inverse4x, pure scalar/default source-mask padding/fulloriginalA only; no artRGB, crop, BBox, UVfit, retry or optout. OriginalA4x byte exact and all generated native targets POT. Broad native pattern group counts proven before prompt; exact source pale pixels printed before call; supplemental bright row band diagnostic saved postcall and honestly labelled postcall, never retroactive proof.','source_before_call':portable(before),'source_before_call_sha256':sha(before),'pattern_before_call':portable(BASE/f'pattern-constraints-{STEM}.json'),'supplemental_tiny_marks':portable(BASE/f'tiny-marks-review-{STEM}.json'),'final_guards':portable(BASE/f'final-guards-{STEM}.json'),'ownership':'Own new Eq29 independent files only. All old stable packages/shared metadata/build/Game/C++/Git untouched. Ready means review candidate; no acceptance/pack claim.'})
mp=BASE/f'sha256-{STEM}.json';files=sorted(BASE.glob(f'*{STEM}*.json'))+sorted((BASE/'generated').glob(f'*{STEM}-call.json'));manifest={portable(p):sha(p)for p in files if p!=mp};save(mp.name,manifest);assert all(sha(ROOT/p)==h for p,h in read(mp).items())
print(json.dumps({'ready':READY,'held_called':[3047],'uncalled_pending':9,'calls':3,'manifest_entries':len(manifest),'sha256':{n:sha(BASE/f'{n}-{STEM}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

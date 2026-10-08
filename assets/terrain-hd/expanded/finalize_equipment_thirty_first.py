from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
STEM='equipment-thirty-first'
GENERATED=[1821,1827,1921]
READY=[1821,1827]
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
notes={1821: 'Ready only coordinator native art/material review. This is a REPEATED source-only audit; prior whole source views/support in Eq15/Eq24 recorded in source-check with SHA, never claimed first inspected. No previous raw/toolcall/job before this sole built-in edit. Full native64x128/sourceRGB NN4x guide256x512/raw887x1774/native4x256x512/sourceA/storedRGB reviewed privately. Two existing tiny patterned upper-side fields remain faint and in source positions; source left diagnostic rows20/21/22/23/25 retain exact3 maxima each, x9/11/13 except row21 x8/10/12, no invented ideal grid or larger holes/stipple. Original central gray rectangle/weak tiny dark mark/lower warm field/gray slim outlines/black gaps remain. Fine matte gray painted grain retains scale/appearance, no new raised rim/parts. Grain HPcorr .870/.863 in central fields, means57.36→58.66 and76.01→74.91. Whole storedRGB corr .9894/maxlocal+20/weakgain8; narrow outline slightly stronger/small grain smoother are explicit caveats, no automatic brightness/1px hold. SourceA255/POT256x512 byte-exact, whole scalar/defaultpad, not worker accepted/packed claim.', 1827: 'Ready only coordinator native art/material review. Full native64x128/sourceRGB NN4x guide256x512/raw887x1774/native4x256x512/sourceA/storedRGB reviewed privately. EXACT9 source warm horizontal tonal bands at nativey51/56/63/69/75/81/87/93/99 persist after entire inverse scalar; unequal spacing/source field support retained. No additional bar/rod/hole or inferred mechanical/ammunition semantic. Source bluegray painted field/small gray round spots/weak left pale mark/right thin edges/lower angular fields/black gaps all remain. Fine matte painted source grain source-linked (HPcorr .959 left/.790 right, means77.12→80.50 and72.68→73.25). Existing strip warmth remains subdued; source peak row profiles same or weaker. Whole corr .9871/maxlocal+32 near lower left source edge/weakgain44. Explicit smoothing/narrow light edge-strength caveat, moderate paint not auto-held. SourceA255/POT256x512 byte exact, whole scalar/defaultpad only, not worker accepted/packed claim.', 1921: 'Pending coordinator native material/detail-strength review, not permanent refusal. Full native64square/sourceRGB NN8x512/raw1254/native4x256square/sourceA/storedRGB reviewed privately. Original gray/brown atlas layout and checked field broad source phase/count remain; rows26–29 exact checker positions, row25 faint44 tonal maximum merges (source5rows28 maxima→candidate27), no new cell grid/idealized phase extrapolation. This weak native maximum/threshold1px change is NOT sole hold. Main concern visible fullraw metal edges become brighter harder/rim-like and added fine etched/chipped-looking material contrasts with source subdued painted surface; local+55 at6,21 (RGB123/130/132→179/183/187), weakgain249. Checked pigment locally stronger/warm than source: domain mean27.84→34.64; source pale point50,34 RGB206/215/222→202/191/190 remains one paintpoint but becomes warm. HPcorr .972 body/.968 checks/.986 bottom; broad source pattern retained despite material-strength caveat, root may decide independently. Source Ordinary actualA0..255/MaterialAlpha alpha_test Clamp, full originalA4x byte-exact/POT256square/defaultpad. One sole call/raw/prompt preserved in rejected pending; no retry/artistRGB/crop/BBox/optout.'}
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
save(f'review-{STEM}.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':scope['ids'],'generated_ids':GENERATED,'ready_ids':READY,'pending_called_ids':[1921],'pending_uncalled_ids':[i for i in scope['ids']if i not in GENERATED],'call_count':3,'records':records,'source_parity':'All twelve fresh actual Materials.TextureID/templates/all actual Model consumers and Particle/nonparticle bindings checked against typed sourcequeue. HistoricalMMP/release/existing original fullRGBA dimensions/hash exact; actual source Type/alpha recorded. Mesh UV/dynamic dispatch unknown where not directly proven, no fake semantics.','QA':'Every generated fullraw/source/helper/native4x sourceA/storedRGB privately inspected. ENTIRE raw inverse4x, pure scalar/default source-mask padding/fulloriginalA only; no artRGB, crop, BBox, UVfit, retry or optout. OriginalA4x byte exact and all generated native targets POT. Exact original native faint repeat phase, 9warm bands and 28measured checked field maxima asserted and persisted before prompts; supplemental actual darkmark13px/source point RGB assertion persisted before calls. Postcall candidate/source phase and material diagnostics honestly postcall. 1821 prior source-only support history explicitly recorded, no first-view claim.','source_before_call':portable(before),'source_before_call_sha256':sha(before),'pattern_before_call':portable(BASE/f'pattern-constraints-{STEM}.json'),'supplemental_tiny_marks':portable(BASE/f'tiny-marks-review-{STEM}.json'),'final_guards':portable(BASE/f'final-guards-{STEM}.json'),'ownership':'Own new Eq31 independent files only. All old stable packages/shared metadata/build/Game/C++/Git untouched. Ready means review candidate; no acceptance/pack claim.'})
mp=BASE/f'sha256-{STEM}.json';files=sorted(BASE.glob(f'*{STEM}*.json'))+sorted((BASE/'generated').glob(f'*{STEM}-call.json'));manifest={portable(p):sha(p)for p in files if p!=mp};save(mp.name,manifest);assert all(sha(ROOT/p)==h for p,h in read(mp).items())
print(json.dumps({'ready':READY,'held_called':[1921],'uncalled_pending':9,'calls':3,'manifest_entries':len(manifest),'sha256':{n:sha(BASE/f'{n}-{STEM}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

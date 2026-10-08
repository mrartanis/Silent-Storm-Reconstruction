from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
STEM='equipment-thirtieth'
GENERATED=[676,2968,4257]
READY=[676,2968,4257]
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
notes={676: 'Ready only coordinator native art/material review. Full native128x64/sourceRGB guide512x256/raw1774x887/native512x256/sourceA/storedRGB privately inspected. Exact FOUR source right vertical dark seam phases93/104/114/124 persist after pure scalar/native inverse; unequal pitch preserved. Original left olive field/window/round fields/upper notch and little gray edge detail retained, no new part/vent/rod. Fine painted surface detail added within original broad shade; native highpass correlations .947 left/.963 right, means36.77→38.08 and68.67→65.04, HPstd5.51→6.35 and7.97→8.74. Explicit caveat raw fine pale mottling/fleck strength increased and local weak shadow lift+21 at80,47; whole storedRGB corr .9674/weakgain42. Moderate fine paint/shading variation, no worker automatic hold or acceptance claim; root determines final source-material fidelity. Full original sourceA255 byte exact/POT512x256, whole inverse/scalar/defaultpad only.', 2968: 'Ready only coordinator native art/material review. Full native128square/sourceRGB guide512/raw1254/native512/sourceA/storedRGB privately inspected. Exact TWO upper-right strap fields retain TWO light bar maxima at native24/27 each; no new bands/buckle/prongs. Existing red cross retains one component/four original arms/native bbox94,49,102,67; threshold red core60→64 pixels after whole inverse/scalar, four one-native-pixel left stem-edge threshold texels at(96,53),(96,54),(96,55),(96,63) alter support; original60 red core pixels all retained, not new symbol/enlarged arm. Left three olive rounded fields/central graygreen shaded strip/orange lower-left field/lower olive field/weak tiny pale marks/black regions retained. Original broad paint folds in central strip retained; HPcorr .942 there/.862 lower pack, HPstd5.55→7.35 and2.72→4.45, means41.60→46.79 and82.55→76.30. Caveat somewhat stronger fine brush texture/central strip lighter, left rounded edge maxlocal+32/weakgain121, whole corr .9809. Source A255 byte exact/POT512square, no sourceRGB cross or hardware patch; root final material decision.', 4257: 'Ready only coordinator native art/material review. Full native128x64/sourceRGB guide512x256/raw1774x887/native512x256/sourceA/storedRGB privately inspected. Existing original brown/tan camouflage patches, curved source band with small pale spots and dark right field with weak warm flecks retain whole reference shapes/source phase/scale; no new camo composition/parts/folds/stitches. Precall source brown thresholdR-G>24,G-B>12 mask2181 native pixels/40 connected threshold components fully persisted with native coordinates; these diagnostic threshold islands are not physical patch counts. Full source art privately checked independently. Fine matte paint surface remains source-linked: HPcorr .942 main field/.952 darkright, means55.84→55.99 and6.73→7.02. HPstd6.84→8.91 and4.55→4.92; caveat stronger contrast/light spots and finer irregular pigment edges, maxlocal+33 at32,9/source11585 41→14810754, weakgain7/wholecorr .9843. Moderate source-linked paint contrast alone not auto-hold; root decides final fullraw/native source-palette/pattern material fidelity. SourceA255 byte exact/POT512x256, no patches/crop/BBox/artistRGB/retry.'}
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
save(f'review-{STEM}.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':scope['ids'],'generated_ids':GENERATED,'ready_ids':READY,'pending_called_ids':[],'pending_uncalled_ids':[i for i in scope['ids']if i not in GENERATED],'call_count':3,'records':records,'source_parity':'All twelve fresh actual Materials.TextureID/templates/all actual Model consumers and Particle/nonparticle bindings checked against typed sourcequeue. HistoricalMMP/release/existing original fullRGBA dimensions/hash exact; actual source Type/alpha recorded. Mesh UV/dynamic dispatch unknown where not directly proven, no fake semantics.','QA':'Every generated fullraw/source/helper/native4x sourceA/storedRGB privately inspected. ENTIRE raw inverse4x, pure scalar/default source-mask padding/fulloriginalA only; no artRGB, crop, BBox, UVfit, retry or optout. OriginalA4x byte exact and all generated native targets POT. Exact source native seam phases, two bars per strap, red cross shape/core raster and brown pigment mask/component coordinates executed and persisted BEFORE prompts/calls. Supplemental candidate/source diagnostics saved postcall; never retroactive pre-call claims.','source_before_call':portable(before),'source_before_call_sha256':sha(before),'pattern_before_call':portable(BASE/f'pattern-constraints-{STEM}.json'),'supplemental_tiny_marks':portable(BASE/f'tiny-marks-review-{STEM}.json'),'final_guards':portable(BASE/f'final-guards-{STEM}.json'),'ownership':'Own new Eq30 independent files only. All old stable packages/shared metadata/build/Game/C++/Git untouched. Ready means review candidate; no acceptance/pack claim.'})
mp=BASE/f'sha256-{STEM}.json';files=sorted(BASE.glob(f'*{STEM}*.json'))+sorted((BASE/'generated').glob(f'*{STEM}-call.json'));manifest={portable(p):sha(p)for p in files if p!=mp};save(mp.name,manifest);assert all(sha(ROOT/p)==h for p,h in read(mp).items())
print(json.dumps({'ready':READY,'held_called':[],'uncalled_pending':9,'calls':3,'manifest_entries':len(manifest),'sha256':{n:sha(BASE/f'{n}-{STEM}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

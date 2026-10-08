from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
STEM='equipment-thirty-fourth'
GENERATED=[3034,5380,6473]
READY=[6473]
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
notes={3034: 'Pending coordinator pattern/material review, not permanent refusal. First call after prior source-only graygrid audit; genuine pattern alone does not exclude generation. Full original128square/helperNN4x512/raw1254/native4x512/sourceA/storedRGB privately viewed. Exact EIGHT original source paintfield groups and TWO warm flat source marks remain; upper53,69 source181/138/123→inverse154/117/99, lower53,93 source165/134/115→158/119/98, not new rivets/bolts. But gray tinychecker is idealized into a more regular and sharper lattice than irregular dim source, changing existing pitch/phase distribution, not merely isolated threshold smoothing. Exact strict native-maxima row positions coincide1/38; fullraw/private native show systematic regularization across this field, source native phase rows saved BEFOREprompt. This metric is diagnostic supporting visual pattern concern, not invented universal acceptance cutoff. Whole inversecorr.9957, blade-source local+38/weakgain148 also explicit. Brown fields/raw pixels remain source-based but faint material grain stronger. TypeOrdinary/sourceA255/POT512square and exact fullA4x; defaultpad/pure scalar only. One call, rejected raw/prompt/call saved; no retry/artRGB/crop/BBox/optout. Root may independently review.', 5380: 'Pending coordinator material/pattern review, not permanent refusal. First call after truthful Eq33 source-only re-audit0calls. Full original128x64/helperNN4x512x256/raw1774x887/native4x512x256/fullA/sourceRGB/native storedRGB viewed. FOUR original main paintfields and ONE dark original tinted spot retained; actual source spot minRGB16/12/8,A255, inverse center68,24→23/19/19, no new alpha-hole. Draft source numeric mask<8 failed BEFORE saved prompts/calls; actual original matrix observed and source-derived<=33 darkregion proves1component; separate beforeprompt correction saved, no false source black/alpha interpretation. But right finely painted grayblue streak field becomes coarser/jagged, brown original washes gain more pronounced rough brush faceting; source material/scale softness concern. Local inverse graypoint94,23 RGB49/52/57→84/90/97(+40), weakgain104, wholecorr.9851. Exact source-phase native maxima5/64 rows match; different weak maxima alone do not imply new strand counts, no physical thread/hardware meanings invented; private full views support material hold. Domains mean remain near source45.19→46.63/71.18→70.06/right53.32→51.76/lower64.11→62.64, so brightness scalar alone cannot remove changed local detail. Actual Ordinary/A255/POT/exact fullA4x, existing defaultpad/scalar only. One call, rejected snapshot saved; no retry/crop/BBox/artRGB/optout.', 6473: 'Ready ONLY coordinator final art/native review, not accepted. First call after fresh full historical/release/original128squareRGBA and actual Materials/templates/all diffuse consumers proof. Full original/helperNN4x512/raw1254/native4x512/defaultscalar/fullsourceA/storedRGB privately viewed. Original SIX broad paintfield groups retained: darkbrown L-shaped region with ONE existing small gray rectangular paintoutline; ONE small dark rounded/octagonal paintfield; ONE larger round camo field; THREE bottom camo bands separated by TWO narrow brown strokes y90..92/y103..105. Existing black/shadow regions A255 retained, not automatically alpha holes; no new strap/buckle/bolt interpreted from tinyrectangle. Native source wholecamo matrices/color-mask phase recorded BEFOREprompt, private whole source/raw/native show original patch shapes/count/relative scale, with slightly sharper/smoother edges and finer matte pigment texture. Explicit caveats raw overall brighter, pure scalar reduces; local hue/tonal differences remain, e.g source51,83 RGB30/29/32→54/41/26(+22) at a camo boundary, source body gray/brown/olive hue hierarchy stays broadly source-based. Whole inversecorr.9815/weakgain183; uppercamomean26.24→24.67, lowerband46.87→46.39, upperband33.29→37.00/middle32.96→34.70. Exact geometric patch counts are not inferred from threshold-component counts; moderate1pixel boundary smoothing/local paint lift is not automatically newparts. Actual Ordinary/sourceA255/POT512square/fullA4x byteexact; no bright raisedhardware/metalgloss observed in pure native. Defaultpad/pure scalar/fullA only, no artRGB/crop/BBox/optout/retry. Root final whole art/material and actual native runtime validation authoritative.'}
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
save(f'final-guards-{STEM}.json',{'audit_utc':datetime.now(timezone.utc).isoformat(),'precall_counts':[ex['accepted_count'],ex['queue_count']],'postcall_source_entry_count':len(sources),'postcall_queue_entry_count':len(q),'postcall_queue_status_counts':{status:sum(r.get('status')==status for r in q)for status in sorted({str(r.get('status'))for r in q})},'validated_queue_count':sum(r.get('status')=='validated'for r in q),'source_count_semantics':'Source entries include generated-pending-validation; only queue validated status is counted as validated, root native validation remains authoritative.','own_postcall_overlap':own,'precall_accepted_called_held_jobs_aliases_and_identical_RGBA_absent':True,'prior_sourceonly_selected0calls_reaudit_allowed':True,'all_jobs_raw_prompts_calls_held_aliases_and_shared_q_guarded':True,'sources_sha256':hashlib.sha256(sb).hexdigest(),'queue_sha256':hashlib.sha256(qb).hexdigest(),'shared_writes':False,'calls':3,'retries':0})
save(f'review-{STEM}.json',{'status':'stable-ready-subset-awaiting-coordinator-native-material-review','selected_ids':scope['ids'],'generated_ids':GENERATED,'ready_ids':READY,'pending_called_ids':[3034,5380],'pending_uncalled_ids':[i for i in scope['ids']if i not in GENERATED],'call_count':3,'records':records,'source_parity':'All twelve fresh actual Materials.TextureID/templates/all actual Model consumers and Particle/nonparticle bindings checked against typed sourcequeue. HistoricalMMP/release/existing original fullRGBA dimensions/hash exact; actual source Type/alpha recorded. Mesh UV/dynamic dispatch unknown where not directly proven, no fake semantics.','QA':'Every generated fullraw/source/helper/native4x sourceA/storedRGB privately inspected. ENTIRE raw inverse4x, pure scalar/default source-mask padding/fulloriginalA only; no artRGB, crop, BBox, UVfit, retry or optout. OriginalA4x byte exact and all generated native targets POT. Exact original8fields/2warm flatmarks/checker native rows,4paintfields/1source tinteddarkspot/graystreak native rows,6broad camofields/3bands/2separators/1grayrectangle and whole native matrices were persisted BEFOREprompts/calls. Previous source-only selected0calls permitted truthful re-audit, no accepted/called/held repeated. Supplemental candidate/source phase/alpha/material diagnostics saved postcall and honestly labelled; first-ever readonly view never claimed. Current sourceentries may include root generated-pending-validation, not all accepted.','source_before_call':portable(before),'source_before_call_sha256':sha(before),'pattern_before_call':portable(BASE/f'pattern-constraints-{STEM}.json'),'supplemental_tiny_marks':portable(BASE/f'tiny-marks-review-{STEM}.json'),'final_guards':portable(BASE/f'final-guards-{STEM}.json'),'ownership':'Own new Eq34 independent files only. All old stable packages/shared metadata/build/Game/C++/Git untouched. Ready means review candidate; no acceptance/pack claim.'})
mp=BASE/f'sha256-{STEM}.json';files=sorted(BASE.glob(f'*{STEM}*.json'))+sorted((BASE/'generated').glob(f'*{STEM}-call.json'));manifest={portable(p):sha(p)for p in files if p!=mp};save(mp.name,manifest);assert all(sha(ROOT/p)==h for p,h in read(mp).items())
print(json.dumps({'ready':READY,'held_called':[3034,5380],'uncalled_pending':9,'calls':3,'manifest_entries':len(manifest),'sha256':{n:sha(BASE/f'{n}-{STEM}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

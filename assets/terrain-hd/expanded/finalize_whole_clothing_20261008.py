"""Freeze the complete artwork union without mutating shared HD manifests."""
from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,subprocess,sys
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent; ROOT=B.parents[2]; HD=B.parent; ST='clothing-complete-20261008'
sys.path.insert(0,str(HD))
from match_brightness import match_cell,stored_luma,pad_rgb_under_source_mask
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def rel(p):return p.resolve().relative_to(ROOT).as_posix()
def rp(s):
 p=Path(s); return p if p.is_absolute() else ROOT/p
def ref(p):return {'path':rel(p),'SHA256':sha(p)}
def save(p,v):
 if p.exists():
  assert read(p)==v,('frozen output differs',p)
  return
 p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def canonical(v):return hashlib.sha256(json.dumps(v,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()).hexdigest()

queuep=B/'groups/source-queues/characters-clothing.json'; category=read(queuep)
ids={r['id'] for r in category}; assert len(ids)==len(category)==300
sourcep=HD/'sources.json'; sources=read(sourcep)['textures']; cur={r['id']:r for r in sources}
base=json.loads(subprocess.check_output(['git','show','402fed29:assets/terrain-hd/sources.json'],cwd=ROOT))['textures']
accepted={r['id']:r for r in base if r['id'] in ids}; assert len(accepted)==95
unaccepted=ids-set(accepted); assert len(unaccepted)==205
audits={r['id']:r for r in read(B/f'source-check-{ST}.json')}; assert set(audits)==unaccepted
records=[]; accepted_proofs=[]
for i,r in sorted(accepted.items()):
 assert cur[i]==r,('accepted metadata changed',i)
 files=[]
 for key in ['png','uncalibrated_png','reference_png','generated_png']:
  if not r.get(key):continue
  path=HD/r[key]; gpath=path.relative_to(ROOT).as_posix()
  expected=hashlib.sha256(subprocess.check_output(['git','show',f'402fed29:{gpath}'],cwd=ROOT)).hexdigest()
  assert sha(path)==expected,('accepted PNG changed',i,key)
  files.append({'kind':key,**ref(path)})
 a={'id':i,'status':'accepted-preserved','source_entry_canonical_SHA256':canonical(r),'files':files}
 accepted_proofs.append(a);records.append(a)

chunks=['jobs-clothing-complete-20261008.json','jobs-reuse-old-clothing-complete-20261008.json',
 'jobs-reuse-siblings-clothing-complete-20261008.json','jobs-inherited-tail-clothing-complete-20261008.json',
 'jobs-clothing-tail-complete-20261008.json','jobs-reuse-accepted799-clothing-complete-20261008.json',
 'jobs-clothing-garments-complete-20261008-FINAL.json']
jobs=[];seen=set();job_sources={}
for name in chunks:
 for old in read(B/name):
  j=dict(old);i=j['id'];assert i in unaccepted and i not in seen,(i,name)
  seen.add(i);job_sources[i]=ref(B/name)
  r=audits[i]; rid=r['resource_when_gfx_texture_usedxt_0'];a=next(a for a in r['aliases'] if a['resource_id']==rid)
  assert a['present'] and a['historical_release_bytes_exact'] and a['historical_RGBA_exact']
  assert r['actual_native_Type']=='Ordinary'
  j['generated']=rel(rp(j['generated']))
  if 'reference_pngs' not in j:
   legacy=read(rp(j['reused_existing_generation_call']['path']))
   exact=legacy.get('exact_prompt') or legacy.get('prompt')
   assert exact==j['prompt'],(i,'old exact prompt')
   j['reference_pngs']=legacy.get('reference_pngs') or legacy.get('referenced_image_paths') or legacy.get('references')
   assert j['reference_pngs'],(i,'old exact references missing')
  j['reference_pngs']=[rel(rp(p)) for p in j['reference_pngs']]
  j['source_alpha_reference']=a['fullRGBA_PNG'];j['source_resource_id']=rid
  j['source_resource_selection_reason']=f"Individual actual Format{r['actual_Format']} / bIsDXT{r['CTexture_Import_bIsDXT']} under gfx_texture_usedxt0 selects WHOLE native resource{rid} RGB/A. Historical/release fullRGBA exact; native TypeOrdinary, source logical canvas, UV and actual material modes preserved. Frozen source-audit qualifies older immutable generic selector descriptions."
  j['category_artwork_proof']=job_sources[i];j['source_selected_RGBA_SHA256']=a['native_RGBA_SHA256']
  assert not any(j.get(k) for k in ['uv_regions','register_alpha_bbox','register_rgb_bbox'])
  if i==6238:assert not j.get('reuse_source_id')
  jobs.append(j)
assert len(jobs)==168
# All direct donors precede their full-source exact siblings. The metadata remains raw-reuse,
# not accepted production/native byte reuse, and6238 explicitly has no reuse_source_id.
byjob={r['id']:r for r in jobs}; ordered=[];done=set()
def insert(i):
 if i in done:return
 j=byjob[i]
 donor=j.get('reused_from_id') or j.get('reused_from_ID') or j.get('reuse_source_id')
 if donor in byjob:insert(donor)
 ordered.append(j);done.add(i)
for i in sorted(byjob):insert(i)
jobs=ordered
assert next(j for j in jobs if j['id']==2011)['generation_variant']==ST+'-v9'
assert next(j for j in jobs if j['id']==3313)['generation_variant']==ST+'-v5'

numeric=[];projection_differences=[];predir=B/f'private-{ST}/final-native4x';predir.mkdir(exist_ok=True)
for j in jobs:
 i=j['id'];r=audits[i];s=Image.open(ROOT/j['source_alpha_reference']).convert('RGBA');raw=Image.open(ROOT/j['generated']).convert('RGBA')
 assert hashlib.sha256(s.tobytes()).hexdigest()==j['source_selected_RGBA_SHA256']
 assert raw.getchannel('A').getextrema()==(255,255),(i,'nonopaque raw would differ from RGB-FIRST importer')
 target=(s.width*4,s.height*4);a=s.getchannel('A').resize(target,Image.Resampling.LANCZOS)
 inp=raw.convert('RGB').resize(target,Image.Resampling.LANCZOS).convert('RGBA')
 padded,pc,cc=pad_rgb_under_source_mask(inp,a)
 corr,gain,lum,rounding=match_cell(padded,stored_luma(s),False,a);corr.putalpha(a)
 assert corr.getchannel('A').tobytes()==a.tobytes()
 pred=predir/f'{i}-final-pure-native4x.png'
 if pred.exists():assert Image.open(pred).convert('RGBA').tobytes()==corr.tobytes(),(i,'immutable final prediction differs')
 else:corr.save(pred)
 inv=corr.convert('RGB').resize(s.size,Image.Resampling.LANCZOS)
 sr=np.asarray(s.convert('RGB')).astype(float);iv=np.asarray(inv).astype(float)
 row={'id':i,'generation_variant':j['generation_variant'],'source_resource_id':j['source_resource_id'],
  'source_RGBA_SHA256':j['source_selected_RGBA_SHA256'],'raw':ref(ROOT/j['generated']),
  'prediction':ref(pred),'prediction_RGBA_SHA256':hashlib.sha256(corr.tobytes()).hexdigest(),
  'full_selected_A4x_exact':True,'logical_size':list(s.size),'target_size':list(target),'native_Type':'Ordinary',
  'gain':gain,'target_stored_luma':stored_luma(s),'predicted_stored_luma':lum,'luma_absolute_error':abs(lum-stored_luma(s)),
  'rounding':rounding,'RGB_padding_texels':pc,'hidden_clear_texels':cc,
  'RGB_correlation':float(np.corrcoef(sr.ravel(),iv.ravel())[0,1]),'RGB_native_MAE':float(np.abs(sr-iv).mean())}
  # Compare known worker projection when an exact candidate-specific metric is available.
 known=j.get('worker_numeric_QA')
 if not known:
  candidates=[B/f'private-{ST}/native4x/{i}-{j["generation_variant"]}-metrics.json',B/f'private-clothing-tail-complete-20261008/native4x/{i}-{j["generation_variant"]}-metrics.json']
  known=next((rel(p) for p in candidates if p.exists()),None)
 if known:
  m=read(rp(known));q=m.get('production_preview') or m.get('pure_preview') or m.get('normalized_preview')
  if q and rp(q).exists():
   prev=Image.open(rp(q)).convert('RGBA');same=prev.tobytes()==corr.tobytes()
   row['worker_prediction_RGBA_byte_exact']=same;row['worker_numeric_proof']=ref(rp(known))
   if not same:
    row['old_worker_prediction']=ref(rp(q));row['old_vs_selected_RGBA_MAE']=np.abs(np.asarray(prev).astype(float)-np.asarray(corr).astype(float)).mean((0,1)).tolist()
    projection_differences.append(row)
 numeric.append(row)
 records.append({'id':i,'status':'HD-artwork-ready','generation_variant':j['generation_variant'],
  'source_resource_id':j['source_resource_id'],'source_RGBA_SHA256':j['source_selected_RGBA_SHA256'],
  'proof_file':job_sources[i]['path'],'proof_SHA256':job_sources[i]['SHA256'],'reason':j['review_concern'],
  'numeric_prediction':ref(pred),'raw_SHA256':row['raw']['SHA256'],'reuse_from_id':j.get('reused_from_id')})

qualificationp=B/f'individual-final-selected-source-projection-qualification-{ST}.json'
if projection_differences:
 dp=B/f'selected-source-final-projection-differences-{ST}.json'
 save(dp,{'status':'requires-whole-selected-source-projection-review','records':projection_differences})
 if not qualificationp.exists():
  print(json.dumps({'needs_selected_projection_whole_review':[r['id']for r in projection_differences],'proof':ref(dp)}));sys.exit(3)
 quals=read(qualificationp)['records'];qm={r['id']:r for r in quals}
 assert set(qm)=={r['id']for r in projection_differences}
 for r in projection_differences:
  q=qm[r['id']];assert q['status']=='ready' and q['selected_prediction_SHA256']==r['prediction']['SHA256']
  byjob[r['id']]['individual_selected_projection_qualification']=ref(qualificationp)
  byjob[r['id']]['review_concern']+=' '+q['reason']
  for record in records:
   if record['id']==r['id']:record['selected_source_projection_qualification']=ref(qualificationp)

retained=[]
zero=set([1881,1885,1889,1893,2008,2442,2545,2549,3071,4454,4518,4564,4572,7712]);wire={3085,3847,3848}
proofzero=B/f'coordinator-original-purpose-{ST}.json';proofwire=B/'coordinator-original-purpose-clothing-wireframes-20261008.json'
assert {r['id']for r in read(proofzero)['records']}==zero
assert {r['id']for r in read(proofwire)['records']}==wire
for i in sorted(zero|wire):
 p=proofzero if i in zero else proofwire; rr=next(r for r in read(p)['records']if r['id']==i)
 reason='Individual whole actual RGB/A and typed alpha_test material/shader proof: complete selected A0 fails every diffuse alpha-test pixel/mip. Retain native original technical control; not an artistic diffuse candidate or a runtime model screenshot.' if i in zero else rr['individual_source_purpose']
 row={'id':i,'status':'retained-original-functional-source-control','reason':reason,'proof_file':rel(p),'proof_SHA256':sha(p),'individual_proof_record_id':i,'source_resource_id':audits[i]['resource_when_gfx_texture_usedxt_0']}
 retained.append(row);records.append(row)
missing={6137,6138,6141,6142,6144,6145,6146,7611,7628,7629,7630,7660,7661}
for i in sorted(missing):
 r=audits[i];assert not r['original_available']
 p=B/f'original-missing-{ST}-{i}.json'
 save(p,{'id':i,'status':'original-source-unavailable','source_audit':ref(B/f'source-check-{ST}.json'),'actual_typed_audit':r,'new_generation_calls':0,'reason':'Individual historical/release source resource unavailable in audited inventory; no usable original RGB/A canvas to preserve.'})
 row={'id':i,'status':'original-source-unavailable','reason':'Individual audited historical/release original canvas unavailable; generation omitted for absent source.','proof_file':rel(p),'proof_SHA256':sha(p)}
 retained.append(row);records.append(row)
refusals={1760:B/f'service-refusal-{ST}-1760.json',4947:B/'generated/4947-clothing-garments-complete-20261008-v1-actual-service-refusal.json',5670:B/'generated/5670-clothing-garments-complete-20261008-v2-actual-tool-error.json'}
for i,p in refusals.items():
 assert p.exists();row={'id':i,'status':'actual-imagegen-service-refusal','reason':'Real built-in imagegen moderation_blocked service refusal, exact actual arguments/source/call error retained; preserve original, no bypass or further call.','proof_file':rel(p),'proof_SHA256':sha(p),'own_generation_call_refused':True,'related_no_call_hold':False}
 retained.append(row);records.append(row)
for i in [1761,1762,801,2446]:
 p=B/f'original-related-hold-{ST}-{i}.json';assert p.exists()
 row={'id':i,'status':'held-without-own-generation-call-due-to-related-actual-refusal1760','reason':'Individually audited related uniform/award/collar/badge source family1760. No own generation call/refusal; original retained due to relationship with actual refusal1760, explicitly distinguished from actual service refusal.','proof_file':rel(p),'proof_SHA256':sha(p),'own_generation_call_refused':False,'related_no_call_hold':True,'related_actual_refusal_id':1760}
 retained.append(row);records.append(row)
assert len(records)==len({r['id']for r in records})==300
assert {r['id']for r in records}==ids
assert len(retained)==37 and seen|{r['id']for r in retained}==unaccepted
np=B/f'whole-category-numeric-QA-{ST}-final.json'
save(np,{'status':'complete-artwork-numeric-projection','ready_ID_count':168,'checks':numeric,
 'all_complete_selected_A4x_exact':True,'all_selected_historical_release_RGBA_exact':True,'all_raw_opaque':True,
 'recipe':'Entire raw RGB FIRST LANCZOS4x, default fullselectedA source-mask padding/global match_cell scalar, full selected A restored. Ordinary native straightRGB; no crop/registration/localRGB drawing. Native package/build/runtime remain coordinator checks.',
 'gain_min':min(r['gain']for r in numeric),'gain_max':max(r['gain']for r in numeric),'max_luma_error':max(r['luma_absolute_error']for r in numeric)})
jp=B/f'jobs-{ST}-final.json';save(jp,jobs)
cp=B/f'category-final-{ST}.json'
save(cp,{'scope':'characters-clothing','status':'complete','stage':'artwork-complete-awaiting-unified-import-native-build-runtime','source_queue':ref(queuep),
 'original_category_ID_count':300,'accepted_preserved':95,'accepted_snapshot_baseline_git':'402fed29','ready_ID_count':168,'ready_ids':[j['id']for j in jobs],
 'eligible_art_unprocessed':0,'artistic_holds_remaining':0,'retained_original_count':37,'retained_originals':retained,'all_ID_dispositions':sorted(records,key=lambda r:r['id']),
 'accepted_preservation':accepted_proofs,'ready_jobs':ref(jp),'numeric_QA':ref(np),
 'functional_source_control_IDs':sorted(zero|wire),'missing_source_IDs':sorted(missing),'actual_service_refusal_IDs':sorted(refusals),'related_no_call_hold_IDs':[801,1761,1762,2446],
 'accepted_textures_regenerated':False,'source_selector_is_individual_actual_whole_RGBA':True,'2011_independent_review':ref(B/'independent-art-review-2011-heads-worker-final.json'),
 '6238_individual_existing799_raw_review':ref(B/'review-reuse-accepted799-clothing-complete-20261008.json'),
 'helper_whole_QA':ref(B/'whole-category-QA-clothing-garments-complete-20261008-FINAL.json'),'4299_independent_review':ref(B/'independent-art-review-clothing-complete-20261008-4299-v2.json'),
 'private_visual_QA':'Whole source RGB/A, full raw, whole calibrated native projection individually reviewed by owner or delegated garment worker; inherited proofs qualified individually. Fine paint/local contrast qualifications retained in each reason. No public comparisons/HTML.',
 'no_runtime_acceptance_claim':True,'shared_metadata_import_build_native_runtime_Git_owner':'root','runtime_visual_reviewer':'clothing_complete or delegated heads/garment worker','utc':datetime.now(timezone.utc).isoformat()})
# Freeze immutable existing artifacts, including earlier attempts and exact call references.
files=set()
for pattern in [f'*{ST}*','*clothing-tail-complete-20261008*','*clothing-coordinator-tail20-20261008*']:
 files.update(p for p in B.rglob(pattern) if p.is_file())
helperman=B/'SHA-manifest-clothing-garments-complete-20261008-FINAL.json'
for f in read(helperman)['files']:
 p=rp(f['path']);assert sha(p)==f['SHA256'],('helper frozen file changed',f['path']);files.add(p)
files.add(helperman)
# Recursively collect on-disk references from exact call/audit/proof objects, never glob shared manifests.
def collect(v):
 if isinstance(v,dict):
  for x in v.values():collect(x)
 elif isinstance(v,list):
  for x in v:collect(x)
 elif isinstance(v,str) and '\n' not in v and len(v)<500:
  try:
   p=rp(v)
   if p.is_file() and p.resolve().is_relative_to(ROOT): files.add(p)
  except (OSError,ValueError):pass
for p in [B/f'source-check-{ST}.json',cp,jp,helperman]+[B/name for name in chunks]:collect(read(p))
for j in jobs:
 files.add(ROOT/j['generated']);files.update(ROOT/p for p in j['reference_pngs']);files.add(ROOT/j['source_alpha_reference'])
# Read proof/reference JSON data to close the immutable provenance graph, including legacy
# call arguments, canonical prompt files, raw hashes and previously rejected versions.
scanned=set()
while True:
 todo=[p for p in files if p.suffix.lower()=='.json' and p not in scanned]
 if not todo:break
 for p in todo:
  scanned.add(p)
  try:collect(read(p))
  except (UnicodeDecodeError,json.JSONDecodeError):pass
mp=B/f'SHA-manifest-{ST}-final.json';files.discard(mp)
save(mp,{'scope':'characters-clothing-artwork-final','ready_jobs':ref(jp),'category_final':ref(cp),'numeric_QA':ref(np),
 'helper_manifest':ref(helperman),'files':[{'path':rel(p),'SHA256':sha(p),'bytes':p.stat().st_size}for p in sorted(files)],
 'covers_existing_calls_raws_references_proofs_and_rejected_versions':True,'worker_ready_is_not_runtime_acceptance':True})
print(json.dumps({'status':'complete-artwork','category':300,'accepted_preserved':95,'ready':168,'retained_originals':37,'art_unprocessed':0,
 'jobs':ref(jp),'category_final':ref(cp),'SHA_manifest':ref(mp),'SHA_files':len(files),'numeric_QA':ref(np)}))

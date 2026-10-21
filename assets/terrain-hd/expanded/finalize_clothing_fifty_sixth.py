from pathlib import Path
from datetime import datetime,timezone
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-sixth'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
def ref(p):return {'path':rel(p),'file_SHA256':sha(p)}
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
source=B/f'source-check-{ST}.json';before=B/f'source-check-{ST}-before-call.json';assert source.read_bytes()==before.read_bytes()
P=read(before);selected=read(B/f'selected-{ST}.json');scope=read(B/f'scope-{ST}.json');M=read(B/f'native-matrices-{ST}.json')['sources'];A=read(B/f'before-alias-native-matrices-{ST}.json')['sources'];actual=read(B/f'actual-records-{ST}.json');alias=read(B/f'before-alias-proof-{ST}.json');ex=read(B/f'exclusions-{ST}.json');donors=read(B/f'both-alias-donor-guard-{ST}.json')
assert len(P)==len(selected)==12 and scope['unique_native_RGBA_count']==12 and not scope['sole_call_ids']
assert alias['source_before_call_ref']['file_SHA256']==sha(source)
canon=lambda r:hashlib.sha256(json.dumps(r,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode('utf-8')).hexdigest()
for table,rows in actual['tables'].items():
 for k,r in rows.items():assert canon(r)==actual['record_SHA256'][table][k]
records=[]
for r in P:
 i=r['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');v=np.array(o);assert o.getchannel('A').getextrema()==(255,255)and r['source_RGBA_RGB_A_NN_privately_inspected']and r['both_aliases_RGBA_RGB_A_whole_privately_inspected_before_call']and r['imagegen_call_count']==0
 assert np.array_equal(v,np.array(M[str(i)]['native_RGBA_matrix'],dtype=np.uint8))and sha(B/f'original/{i}.png')==r['original_PNG_file_SHA256']
 assert r['material_evidence']['actual_source_native_type']=='Ordinary'and r['fresh_actual_static_consumers']and len(np.unique(v.reshape(-1,4),axis=0))>1
 h=Image.open(ROOT/r['helper_recipe']['output']);assert h.size==o.size and h.convert('RGB').tobytes()==o.convert('RGB').tobytes()
 for k in ['native_matrix_ref','common_actual_records']:assert sha(ROOT/r[k]['path'])==r[k]['file_SHA256']
 a=next(z for z in alias['rows']if z['id']==i);assert a['actual_Texture_Format']=='dxt1'and a['CTexture_Import_bIsDXT']and a['resolved_resource_if_gfx_texture_usedxt_0']==i|0x01000000
 assert a['cross_alias_comparison']['dimensions_equal']and a['cross_alias_comparison']['different_A_pixels']==0 and not a['cross_alias_comparison']['full_RGBA_equal']
 for z in a['aliases']:
  assert z['historical_baseline_payload_exact']and z['historical_baseline_RGBA_exact']
  if z['resource_id']!=i:assert hashlib.sha256(np.array(A[str(z['resource_id'])]['complete_original_alias_RGBA_matrix'],dtype=np.uint8).tobytes()).hexdigest()==z['native_RGBA_SHA256']
 assert not list(B.rglob(f'{i}-*raw*.png'))and not list(B.rglob(f'{i}-*call*.json'))and i not in ex['ids']and r['source_rgba_sha256']not in ex['blocked_hashes']
 d=next(v for v in donors['candidate_aliases']if v['id']==i);assert all(not v.get('sameRGBA_guarded_resource_ids')and not v.get('missing')for v in d['aliases'])
 records.append({'id':i,'status':'genuine-art-source-only-pending-literal-detail-count-phase-proof','call_count':0,'reason':r['source_only_reason'],'literal_wholepainting':r['literal_wholepainting_observations'],'actual_static_consumers':r['fresh_actual_static_consumers'],'native_matrix_ref':r['native_matrix_ref'],'common_actual_records':r['common_actual_records'],'both_alias_native_proof':r['both_alias_native_proof'],'recommended_original_only':False,'not_permanent_refusal':True,'no_generated_candidate':True})
save(f'jobs-{ST}.json',[]);save(f'source-only-detail-{ST}.json',records)
S=read(B.parent/'sources.json')['textures'];Q=read(B/'queue.json');overlap=[{'id':r['id'],'status':r.get('status')}for r in S+Q if r['id']in scope['selected_ids']];assert not overlap
save(f'final-guards-{ST}.json',{'utc':datetime.now(timezone.utc).isoformat(),'before_source_entry_count':ex['source_entry_count'],'before_queue_count':ex['queue_count'],'final_source_entry_count':len(S),'final_queue_count':len(Q),'parent_reported_published':{'sourceentries':2485,'queue':2963,'commit':'64e855d2eace720b5d047b8303cbffee1f82434e'},'source_entry_count_not_published_acceptance_claim':True,'own_current_overlap':overlap,'sources_SHA256':sha(B.parent/'sources.json'),'queue_SHA256':sha(B/'queue.json'),'both_alias_donor_guard_ref':ref(B/f'both-alias-donor-guard-{ST}.json'),'guarded_decoded_matching_dimensions_resources':len(donors['guarded_matching_dimensions_resources']),'source_code_fullSHA_refs':[ref(B/f'{n}_clothing_fifty_sixth.py')for n in ['guard','alias_guard','survey','audit','alias_audit','prepare','finalize']],'shared_writes':False,'calls':0,'no_forced_quota':True})
save(f'review-{ST}.json',{'batch':ST,'status':'stable-source-only-bounded-genuine-art-pending','selected_ids':scope['selected_ids'],'unique_compressed_native_RGBA_count':12,'generated_ids':[],'ready_ids':[],'held_ids':[],'genuine_pending_uncalled_ids':scope['selected_ids'],'recommended_original_only_ids':[],'call_count':0,'max_authorized_calls':3,'no_quota_no_retries':True,'no_technical_structural_classification':True,'not_allremaining_exhaustion_claim':True,'records':records,'source_before_call':ref(before),'source_check':ref(source),'pattern_constraints':ref(B/f'pattern-constraints-{ST}.json'),'native_matrix':ref(B/f'native-matrices-{ST}.json'),'actual_records':ref(B/f'actual-records-{ST}.json'),'consumer_code_proof':ref(B/f'consumer-code-proof-{ST}.json'),'both_aliases_before_ref':ref(B/f'before-alias-proof-{ST}.json'),'source_parity':'12genuinepainted/POT/Ordinary/A255/actualMaterialTemplateModels color sources,68unique actualDBrows,24resource historical-release payload/nativeRGBA exact separately. Allpairs have equaldims/fullA255 but differentRGB/payload, actualFormatdxt1 conditionalhighbit dispatch proven. Canonical original PNG matches compressed source_i. BothwholeRGB/A privatelyinspected; no runtime/Game state measured byworker. Twelve prior source-only ZEROcall re-audits honestlydisclosed, no raw/prompt/call/accepted/held/exactRGBA generated donors repeated. Genericfullrow native pigment diagnostics are not closed physicalpart/glyphcounts. Individual oldsmallprintedglyph/grid/irregularcountphase remains unresolved; no forcedcall or robot/clothing/pattern/category ban. No original-onlyrecommendation or wholeRGBAconstant classification.','ownership':'Own Cl56 only; Cl55/54/53/52/Heads26/25/24/Cl51/olderfrozen unchanged. Root sole shared/import/build/native/Game/Git/finalacceptance.'})
assert before.read_bytes()==source.read_bytes()
mp=B/f'sha256-{ST}.json';files=list(B.glob(f'*{ST}*.json'))+list(B.glob('*clothing_fifty_sixth*.py'))+list((B/'generated').glob(f'*{ST}*.png'))+list((B/f'private-{ST}').rglob('*'))
save(mp.name,{rel(p):sha(p)for p in sorted(set(files))if p.is_file()and p!=mp});assert all(sha(ROOT/p)==h for p,h in read(mp).items())
old={}
for st in ['clothing-fifty-fifth','clothing-fifty-fourth','clothing-fifty-third','clothing-fifty-second','heads-twenty-sixth','heads-twenty-fifth','heads-twenty-fourth','clothing-fifty-first']:
 dd=read(B/f'sha256-{st}.json');bad=[p for p,h in dd.items()if sha(ROOT/p)!=h];assert not bad,(st,bad);old[st]=0
print(json.dumps({'source_only':12,'ready':[],'held':[],'calls':0,'original_recommendations':[],'actual_DB_rows':sum(len(v)for v in actual['tables'].values()),'manifestentries':len(read(mp)),'differences':0,'older_frozen_differences':old,'max_individual_artifact_bytes':max((ROOT/p).stat().st_size for p in read(mp)),'current_sourceentries_queue':[len(S),len(Q)],'SHA256':{n:sha(B/f'{n}-{ST}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,re,sys
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-fifth';i=int(sys.argv[1]);sys.path.insert(0,str(B.parent))
from prepare_sources import ReleaseResources,decode_mmp
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ref(p):return {'path':p.relative_to(ROOT).as_posix(),'file_SHA256':sha(p)}
S=read(B.parent/'sources.json')['textures'];Q=read(B/'queue.json');assert not any(r['id']==i for r in S+Q)
assert not list(B.rglob(f'{i}-*raw*.png'))and not list(B.rglob(f'{i}-*call*.json'))
blocked={r['id']for r in S+Q}
for r in S:blocked.update(r.get('aliases',[]))
for p in B.glob('jobs*.json'):
 d=read(p);rs=d if isinstance(d,list)else d.get('jobs',[]);assert not any(r.get('id')==i for r in rs if isinstance(r,dict)),p
 blocked.update(r['id']for r in rs if isinstance(r,dict)and isinstance(r.get('id'),int))
for p in B.rglob('*'):
 if p.is_file()and any(t in p.name for t in ['raw','call','prompt']):
  mm=re.match(r'(\d+)-',p.name)
  if mm and int(mm[1])!=i:blocked.add(int(mm[1]))
for p in B.glob('*review*.json'):
 d=read(p)
 if not isinstance(d,dict):continue
 for k,vs in d.items():
  if isinstance(vs,list)and(any(t in k for t in ['held','refus','unavailable','blocked','stopped'])or k.startswith(('generated','called','attempted'))):
   for v in vs:
    z=v if isinstance(v,int)else v.get('id')if isinstance(v,dict)else None
    if isinstance(z,int):blocked.add(z)
 for v in d.get('records',[]):
  if isinstance(v,dict)and(v.get('call_count',0)>0 or v.get('imagegen_call_count',0)>0 or any(t in str(v.get('status',''))for t in ['held','refus','unavailable'])):
   if isinstance(v.get('id'),int):blocked.add(v['id'])
assert i not in blocked
release=ReleaseResources('G:/SS/lab/baseline/res');keys={}
for rid in [i,i|0x01000000]:
 data=release.read(rid);o=decode_mmp(data).convert('RGBA');keys[(o.size,hashlib.sha256(o.tobytes()).hexdigest())]=rid
pool=read(B/f'both-alias-donor-guard-{ST}.json');oldids={r['logical_id']for r in pool['guarded_matching_dimensions_resources']}
for r in pool['guarded_matching_dimensions_resources']:assert (tuple(r['size']),r['RGBA_SHA256'])not in keys
new=[]
for j in sorted({k&0x00ffffff for k in blocked}-oldids):
 for rid in [j,j|0x01000000]:
  try:data=release.read(rid)
  except KeyError:continue
  o=decode_mmp(data).convert('RGBA')
  if o.size not in [v[0]for v in keys]:continue
  h=hashlib.sha256(o.tobytes()).hexdigest();assert (o.size,h)not in keys,(j,rid,i)
  new.append({'logical_id':j,'resource_id':rid,'size':list(o.size),'RGBA_SHA256':h,'payload_SHA256':hashlib.sha256(data).hexdigest()})
proof=read(B/f'before-alias-proof-{ST}.json');row=next(r for r in proof['rows']if r['id']==i);assert all(r['historical_baseline_payload_exact']and r['historical_baseline_RGBA_exact']for r in row['aliases'])
for a in row['aliases']:assert hashlib.sha256(release.read(a['resource_id'])).hexdigest()==a['baseline_payload_SHA256']
out={'id':i,'phase':'BEFORE FIRST actualbuiltin toolcall, fresh source/currentqueue/alljobs/reviews/raw/call/held and BOTHnative baseline alias exactRGBA donor guard','utc':datetime.now(timezone.utc).isoformat(),'source_entry_count':len(S),'queue_entry_count':len(Q),'source_SHA256':sha(B.parent/'sources.json'),'queue_SHA256':sha(B/'queue.json'),'source_before_call_ref':ref(B/f'source-check-{ST}-before-call.json'),'pattern_before_call_ref':ref(B/f'pattern-constraints-{ST}.json'),'both_alias_before_ref':ref(B/f'before-alias-proof-{ST}.json'),'both_alias_donor_guard_ref':ref(B/f'both-alias-donor-guard-{ST}.json'),'fresh_added_matching_dimension_guarded_aliases':new,'guarded_logicalIDs_count':len({k&0x00ffffff for k in blocked}),'own_prepared_prompt_excluded_because_nottoolcall':True,'no_previous_call_or_raw_or_job_or_held':True,'both_baseline_alias_historical_release_parity_freshly_verified':True,'source_code_fullSHA_refs':[ref(B/f'{name}_clothing_fifty_fifth.py')for name in ['guard','alias_guard','survey','audit','alias_audit','prepare','precall_guard']],'qualification':'Compact immutable donor matrixhashrefs plus freshnew root/raw/call additions, no large duplicated native arrays. Entire aliases nativeRGBA matched separately, actualdispatch conditional proved; no runtime Game. Currententry counts not automatic publishedacceptance.'}
(B/f'precall-guard-{ST}-{i}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print({'FIRST':i,'current':[len(S),len(Q)],'fresh_added_matching_aliases':len(new),'bothalias_donors':0})

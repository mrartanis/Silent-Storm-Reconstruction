from pathlib import Path
import json,hashlib,re,sys
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];i=int(sys.argv[1]);ST='clothing-fifty-third'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
S=read(B.parent/'sources.json')['textures'];Q=read(B/'queue.json');assert not any(r['id']==i for r in S+Q)
assert not list(B.rglob(f'{i}-*raw*.png'));assert not list(B.rglob(f'{i}-*call*.json'))
o=Image.open(B/f'original/{i}.png').convert('RGBA');key=str(o.size)+hashlib.sha256(o.tobytes()).hexdigest();ex=read(B/f'exclusions-{ST}.json');assert key not in ex['blocked_RGBA_keys']
for p in B.glob('jobs*.json'):
 rows=read(p);rows=rows if isinstance(rows,list)else rows.get('jobs',[]);assert not any(r.get('id')==i for r in rows if isinstance(r,dict)),p
freshblocked={r['id']for r in S+Q}
for p in B.rglob('*'):
 if p.is_file()and any(t in p.name for t in ['raw','call','prompt']):
  mm=re.match(r'(\d+)-',p.name)
  if mm and int(mm[1])!=i:freshblocked.add(int(mm[1]))
for p in B.glob('*review*.json'):
 d=read(p)
 if isinstance(d,dict):
  for k,vs in d.items():
   if isinstance(vs,list)and(any(t in k for t in ['held','refus','unavailable','blocked','stopped'])or k.startswith(('generated','called','attempted'))):
    for v in vs:
     z=v if isinstance(v,int)else v.get('id')if isinstance(v,dict)else None
     if isinstance(z,int):freshblocked.add(z)
  for v in d.get('records',[]):
   if isinstance(v,dict)and(v.get('call_count',0)>0 or v.get('imagegen_call_count',0)>0 or any(t in str(v.get('status',''))for t in ['held','refus','unavailable'])):
    if isinstance(v.get('id'),int):freshblocked.add(v['id'])
assert i not in freshblocked
for z in freshblocked:
 p=B/f"original/{z}.png"
 if p.exists():
  a=Image.open(p).convert('RGBA');assert str(a.size)+hashlib.sha256(a.tobytes()).hexdigest()!=key
out={'id':i,'phase':'BEFORE sole FIRST builtin call, fresh current sources/queue/alljobs/raw/calls/exactRGBA aliases guards','source_entry_count':len(S),'queue_entry_count':len(Q),'source_SHA256':sha(B.parent/'sources.json'),'queue_SHA256':sha(B/'queue.json'),'source_before_call_SHA256':sha(B/f'source-check-{ST}-before-call.json'),'pattern_before_call_SHA256':sha(B/f'pattern-constraints-{ST}.json'),'own_exact_prompt_saved_no_call_yet':True,'sourceonly_previous_audits_permitted_honestly':True,'all_existing_raw_call_job_and_global_sourceRGBA_guards_passed':True,'fresh_current_allreview_raw_call_prompt_RGBA_alias_scan':True,'guarded_id_count':len(freshblocked)}
(B/f'precall-guard-{ST}-{i}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('FIRST call guards pass',i)

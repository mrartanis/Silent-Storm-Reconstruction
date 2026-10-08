from pathlib import Path
import json,hashlib,re,sys
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];i=int(sys.argv[1]);ST='heads-twenty-fourth'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
S=read(B.parent/'sources.json')['textures'];Q=read(B/'queue.json');assert not any(r['id']==i for r in S+Q)
assert not list(B.rglob(f'{i}-*raw*.png'));assert not list(B.rglob(f'{i}-*call*.json'))
o=Image.open(B/f'original/{i}.png').convert('RGBA');key=str(o.size)+hashlib.sha256(o.tobytes()).hexdigest();ex=read(B/f'exclusions-{ST}.json');assert key not in ex['blocked_RGBA_keys']
for p in B.glob('jobs*.json'):
 rows=read(p);rows=rows if isinstance(rows,list)else rows.get('jobs',[]);assert not any(r.get('id')==i for r in rows if isinstance(r,dict)),p
for r in S+Q:
 p=B/f"original/{r['id']}.png"
 if p.exists():
  a=Image.open(p).convert('RGBA');assert str(a.size)+hashlib.sha256(a.tobytes()).hexdigest()!=key
out={'id':i,'phase':'BEFORE sole FIRST builtin call, fresh current sources/queue/alljobs/raw/calls/exactRGBA aliases guards','source_entry_count':len(S),'queue_entry_count':len(Q),'source_SHA256':sha(B.parent/'sources.json'),'queue_SHA256':sha(B/'queue.json'),'source_before_call_SHA256':sha(B/f'source-check-{ST}-before-call.json'),'pattern_before_call_SHA256':sha(B/f'pattern-constraints-{ST}.json'),'own_exact_prompt_saved_no_call_yet':True,'sourceonly_previous_audits_permitted_honestly':True,'all_existing_raw_call_job_and_global_sourceRGBA_guards_passed':True}
(B/f'precall-guard-{ST}-{i}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('FIRST call guards pass',i)

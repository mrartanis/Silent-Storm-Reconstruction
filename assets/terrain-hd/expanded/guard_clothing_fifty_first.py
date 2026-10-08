from pathlib import Path
import json,re,hashlib
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-fifty-first'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
S=read(B.parent/'sources.json')['textures'];Q=read(B/'queue.json');blocked={r['id']for r in S+Q}
for r in S:blocked.update(r.get('aliases',[]))
for p in B.glob('jobs*.json'):
 d=read(p);rs=d if isinstance(d,list)else [r for v in d.values()if isinstance(v,list)for r in v]if isinstance(d,dict)else []
 blocked.update(r if isinstance(r,int)else r.get('id',r.get('texture',{}).get('ID'))if isinstance(r,dict)else None for r in rs)
for p in B.rglob('*'):
 if p.is_file()and any(t in p.name for t in ['raw','call','prompt']):
  m=re.match(r'(\d+)-',p.name)
  if m:blocked.add(int(m[1]))
for p in B.glob('review*.json'):
 d=read(p)
 if isinstance(d,dict):
  for k,rs in d.items():
   if isinstance(rs,list)and any(t in k for t in ['held','refus','unavailable','blocked','stopped']):
    for r in rs:
     i=r if isinstance(r,int)else r.get('id')if isinstance(r,dict)else None
     if isinstance(i,int):blocked.add(i)
hashes={}
for i in blocked:
 p=B/f'original/{i}.png'
 if p.exists():
  o=Image.open(p).convert('RGBA');hashes.setdefault(str(o.size)+hashlib.sha256(o.tobytes()).hexdigest(),[]).append(i)
rows=[];skipped=[];unavailable=[]
for r in read(B/'groups/source-queues/characters-clothing.json'):
 i=r['id'];p=B/f'original/{i}.png'
 if r.get('status')!='pending' or not p.exists() or not r.get('source_rgba_sha256'):
  unavailable.append({'id':i,'source_status':r.get('status'),'source_reason':r.get('reason'),'original_available':p.exists()});continue
 o=Image.open(p).convert('RGBA');key=str(o.size)+hashlib.sha256(o.tobytes()).hexdigest()
 if i in blocked or key in hashes:skipped.append({'id':i,'blocked_id':i in blocked,'same_RGBA_guarded_IDs':hashes.get(key,[])});continue
 rows.append(r)
(B/f'remaining-{ST}.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(B/f'exclusions-{ST}.json').write_text(json.dumps({'ids':sorted(i for i in blocked if isinstance(i,int)),'blocked_RGBA_keys':hashes,'blocked_hashes':sorted({k[ k.index(")")+1:]for k in hashes}),'source_entry_count':len(S),'source_entry_count_not_accepted_claim':True,'queue_count':len(Q),'queue_status_counts':{st:sum(r.get('status')==st for r in Q)for st in sorted({str(r.get('status'))for r in Q})},'policy':'Previous source-only audit/helpers/selected0calls allowed truthful re-audit; accepted/sharedqueue/jobs/raw/prompt/calls/held/refusal and exact sameRGBA aliases excluded.','sources_SHA256':hashlib.sha256((B.parent/'sources.json').read_bytes()).hexdigest(),'queue_SHA256':hashlib.sha256((B/'queue.json').read_bytes()).hexdigest(),'skipped':skipped,'noneligible_source_records':unavailable,'stable_Eq32_Eq33_guard_reviews_read':[{'scope':st,'ready_ids':read(B/f'review-equipment-{st}.json')['ready_ids'],'called':read(B/f'review-equipment-{st}.json')['generated_ids']}for st in ['thirty-second','thirty-third']]},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(len(S),len(Q),'uncalled/re-audit',len(rows));print([(r['id'],r['logical_size'],r['texture']['SrcName'])for r in rows])


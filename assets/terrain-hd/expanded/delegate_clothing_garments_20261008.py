from pathlib import Path
import json,hashlib
from datetime import datetime,timezone
B=Path(__file__).resolve().parent;ST='clothing-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
groups=read(B/f'generation-plan-{ST}.json')['groups'];selected={r['id']:r for r in read(B/f'selected-{ST}.json')};done={r['id']for n in [f'jobs-{ST}.json',f'jobs-reuse-old-{ST}.json',f'jobs-reuse-siblings-{ST}.json']for r in read(B/n)};started={int(p.name.split('-')[0])for p in (B/'generated').glob(f'*-{ST}-*-before-call.json')};delegate=[]
for g in groups:
 if min(g['ids'])<3847 or set(g['ids'])&(done|started):continue
 src=selected[g['ids'][0]]['texture']['SrcName']
 if 'face' in src.lower()or 'hands' in src.lower():continue
 delegate.append(dict(g,source_name=src))
p=B/f'delegated-garments-{ST}.json';assert not p.exists(),'Delegation is frozen; do not rewrite'
v={'scope':'clothing-garments-complete-20261008','parent_scope':ST,'created_utc':datetime.now(timezone.utc).isoformat(),'unique_source_groups':len(delegate),'ID_count':sum(len(r['ids'])for r in delegate),'groups':delegate,'source_audit':f'assets/terrain-hd/expanded/source-check-{ST}.json','source_audit_SHA256':sha(B/f'source-check-{ST}.json'),'actual_records':f'assets/terrain-hd/expanded/actual-records-{ST}.json','actual_records_SHA256':sha(B/f'actual-records-{ST}.json'),'excludes_all_parent_started_ready_and_exact_sibling_groups':True,'shared_metadata_writes':False,'category_build_owner':'root once after both subpackages complete'}
p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Frozen helper allocation',len(delegate),'unique groups',v['ID_count'],'IDs');print([g['ids']for g in delegate])

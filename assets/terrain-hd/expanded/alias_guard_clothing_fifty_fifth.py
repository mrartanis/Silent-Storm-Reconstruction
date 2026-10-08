from pathlib import Path
import json,hashlib,sys,struct
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-fifth';sys.path.insert(0,str(B.parent))
from prepare_sources import ReleaseResources,decode_mmp
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
release=ReleaseResources('G:/SS/lab/baseline/res');ex=read(B/f'exclusions-{ST}.json');remaining=read(B/f'remaining-{ST}.json');dims={tuple(r['logical_size'])for r in remaining}
def header(i):
 p=release.root/'Textures'/str(i)
 if p.exists():
  with p.open('rb')as f:return f.read(24)
 loc=release.index.get(i)
 if loc:
  with loc[0].open('rb')as f:f.seek(loc[1]);return f.read(24)
 return None
rows=[];blocked={};missing=[]
for i in sorted({k&0x00ffffff for k in ex['ids']}):
 for rid in [i,i|0x01000000]:
  h=header(rid)
  if h is None:missing.append(rid);continue
  v=struct.unpack('<6I',h)
  if (v[3],v[4])not in dims:continue
  data=release.read(rid);o=decode_mmp(data).convert('RGBA');key=str(o.size)+hashlib.sha256(o.tobytes()).hexdigest();blocked.setdefault(key,[]).append(rid)
  loc=release.index.get(rid);rows.append({'logical_id':i,'resource_id':rid,'native_header':list(v),'size':list(o.size),'payload_SHA256':hashlib.sha256(data).hexdigest(),'RGBA_SHA256':hashlib.sha256(o.tobytes()).hexdigest(),'archive':str(loc[0]).replace('\\','/')if loc else None,'archive_offset':loc[1]if loc else None,'archive_payload_size':loc[2]if loc else len(data)})
eligible=[];skipped=[];candidate=[]
for r in remaining:
 checks=[];matches=[]
 for rid in [r['id'],r['id']|0x01000000]:
  if header(rid)is None:checks.append({'resource_id':rid,'missing':True});continue
  data=release.read(rid);o=decode_mmp(data).convert('RGBA');key=str(o.size)+hashlib.sha256(o.tobytes()).hexdigest();donors=blocked.get(key,[]);matches+=donors
  checks.append({'resource_id':rid,'native_size':list(o.size),'payload_SHA256':hashlib.sha256(data).hexdigest(),'RGBA_SHA256':hashlib.sha256(o.tobytes()).hexdigest(),'sameRGBA_guarded_resource_ids':donors})
 candidate.append({'id':r['id'],'aliases':checks})
 if matches:skipped.append({'id':r['id'],'sameRGBA_guarded_resources':sorted(set(matches))})
 else:eligible.append(r)
save(f'both-alias-donor-guard-{ST}.json',{'source_entry_count':ex['source_entry_count'],'queue_count':ex['queue_count'],'sources_SHA256':ex['sources_SHA256'],'queue_SHA256':ex['queue_SHA256'],'guarded_native_dimension_filter':[list(k)for k in sorted(dims)],'guarded_matching_dimensions_resources':rows,'candidate_aliases':candidate,'skipped_exactRGBA_alias_donors':skipped,'guarded_missing_resources':missing,'qualification':'Fresh readonly original guarded ID plus highbit resource fullRGBA/dimensions hashes. Header dimension filters only skip mathematically incompatible donors. Missing aliases unknown, no fabricated parity. Exact Type/dispatch/history perselected audited separately BEFORE. No sharedwrites.'})
save(f'both-alias-eligible-{ST}.json',eligible)
print(json.dumps({'guarded_decoded_resources':len(rows),'eligible':len(eligible),'skipped':skipped,'ids':[r['id']for r in eligible]},indent=2))

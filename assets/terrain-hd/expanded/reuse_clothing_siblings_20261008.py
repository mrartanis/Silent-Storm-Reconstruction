from pathlib import Path
import json,hashlib
from datetime import datetime,timezone
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
plan=read(B/f'generation-plan-{ST}.json');source={r['id']:r for r in read(B/f'source-check-{ST}.json')};base={r['id']:r for r in read(B/f'jobs-{ST}.json')};accepted={r['id']for r in read(B.parent/'sources.json')['textures']};queue={r['id']:r for r in read(B/'queue.json')};jobs=[];records=[]
for donor in [916,1879,1887,2006,3898,2016,2237,3069]:
 job=base[donor];group=next(g for g in plan['groups']if donor in g['ids']);d=source[donor];da=next(a for a in d['aliases']if a['resource_id']==d['resource_when_gfx_texture_usedxt_0']);db=read(B/f'generated/{donor}-{job["generation_variant"]}-before-call.json');call=B/f'generated/{donor}-{job["generation_variant"]}-call.json';raw=ROOT/job['generated'];c=read(call);assert sha(raw)==c['raw_SHA256']
 for i in group['ids']:
  if i==donor:continue
  assert i not in accepted and queue.get(i,{}).get('status')not in ['validated','generation-unavailable','imagegen-unavailable']
  e=source[i];a=next(a for a in e['aliases']if a['resource_id']==e['resource_when_gfx_texture_usedxt_0']);assert a['native_RGBA_SHA256']==da['native_RGBA_SHA256']and a['native_size']==da['native_size']and e['actual_native_Type']==d['actual_native_Type'];assert sorted(set((m['Alpha'],m['AddressMode'])for m in e['actual_material_consumers']))==sorted(set((m['Alpha'],m['AddressMode'])for m in d['actual_material_consumers']))
  jobs.append({'id':i,'generation_variant':f'{ST}-donor-{donor}-v1','generated':job['generated'],'prompt':job['prompt'],'reference_pngs':job['reference_pngs'],'source_alpha_reference':a['fullRGBA_PNG'],'source_resource_id':a['resource_id'],'reused_from_id':donor,'review_concern':'Exact whole selected-native RGBA, dimensions, UV canvas, native type and actual material Alpha/AddressMode match reviewed donor. One existing donor generation reused, no new call. Native4x brightness/full A expected identical; target original aliases and previous history preserved.'})
  records.append({'id':i,'status':'ready-exact-selected-source-sibling','donor':donor,'donor_call':call.relative_to(ROOT).as_posix(),'donor_call_SHA256':sha(call),'donor_raw_SHA256':sha(raw),'selected_source_RGBA_SHA256':a['native_RGBA_SHA256'],'native_size':a['native_size'],'material_modes':group['material_modes'],'native_Type':e['actual_native_Type'],'target_source_resource_id':a['resource_id'],'whole_UV_canvas_same':True,'new_calls':0})
(B/f'jobs-reuse-siblings-{ST}.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');(B/f'review-reuse-siblings-{ST}.json').write_text(json.dumps({'scope':ST,'records':records,'reviewed_utc':datetime.now(timezone.utc).isoformat(),'shared_writes':False},ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Exact selected-native siblings ready without calls',len(jobs),[r['id']for r in jobs])

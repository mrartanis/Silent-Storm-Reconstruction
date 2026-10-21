from pathlib import Path
import json,sys,hashlib
from datetime import datetime,timezone
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
metrics={r['id']:r for r in read(B/f'old-selected-metrics-{ST}.json')};sources=read(B.parent/'sources.json')['textures'];accepted={r['id']for r in sources};queue={r['id']:r for r in read(B/'queue.json')}
p=B/f'review-reuse-old-{ST}.json';report=read(p)if p.exists()else{'scope':ST,'records':[],'new_calls':0};jobsfile=B/f'jobs-reuse-old-{ST}.json';jobs=read(jobsfile)if jobsfile.exists()else[]
for i in map(int,sys.argv[1:]):
 assert i not in accepted and queue.get(i,{}).get('status')not in ['validated','generation-unavailable','imagegen-unavailable']
 m=metrics[i];raw=ROOT/m['raw'];assert sha(raw)==m['raw_SHA256'];calls=m['old_history']['calls'];call=next((r for r in calls if '/generated/'in r['path']),calls[0]);c=read(ROOT/call['path']);prompt=c.get('exact_prompt')or c.get('prompt')or c['literal_actual_tool_arguments']['prompt'];assert isinstance(prompt,str)and prompt
 assert sha(ROOT/call['path'])==call['SHA256']
 tag=ST+'-oldreview-v1'
 if i in [6763,6764]:literal='Two tiny top paint groups with four horizontal bands each and one weak left pale dab remain, flat brown skin-colored field has weak painted modulation. No anatomy added.'
 elif i in [6809,6811]:literal='Four soft skin-highlight lobes and bottom V-shadow remain, separated left dark leather strip retains its old elongated folds and two small lower paint pieces. Fine painted grain stronger on the leather.'
 elif i in [6812,6813,6814,6815,6816,6817]:literal='Three main fields retain the dark left cloth, central olive or gray cloth with its existing upper band/diagonal fold fan, and right skin shade patch. Two lower round buckle paint marks, bottom leather fields, right dark strip and every original cutout remain. Fine brush modulation and old fold clarity are stronger without new seams, weave or parts.'
 else:literal='Existing source paint regions and original material retained.'
 reason='Whole selected source RGB, full saved old raw, pure selected-source-A4x and inverse native reviewed. '+literal+' Whole native UV and original pattern scale retained. Historical generation used its documented reference (not retroactively relabeled selected-UC). New numeric projection uses whole actual selected UC and full A; earlier raw/call/reference/BEFORE immutable.'
 job={'id':i,'generation_variant':tag,'generated':m['raw'],'prompt':prompt,'source_alpha_reference':m['source_RGBA'],'source_resource_id':m['source_resource_id'],'review_concern':reason,'reused_existing_generation_call':call}
 rec={'id':i,'status':'ready-existing-generation','generation_variant':tag,'reviewed_utc':datetime.now(timezone.utc).isoformat(),'reason':reason,'call_metadata':call,'raw_SHA256':m['raw_SHA256'],'whole_source_raw_pure_inverse_privately_viewed':True,'numeric':{k:v for k,v in m.items()if k!='old_history'},'fresh_sources_SHA256':sha(B.parent/'sources.json'),'fresh_queue_SHA256':sha(B/'queue.json')}
 jobs=[r for r in jobs if r['id']!=i]+[job];report['records']=[r for r in report['records']if r['id']!=i]+[rec]
jobsfile.write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');p.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Reviewed existing-generation ready IDs:',[r['id']for r in jobs])

from pathlib import Path
import json,hashlib
from datetime import datetime,timezone
B=Path(__file__).resolve().parent;ST='clothing-complete-20261008';i=1760
bp=B/f'generated/{i}-{ST}-v1-before-call.json';b=json.loads(bp.read_text(encoding='utf-8'))
d={'id':i,'generation_variant':f'{ST}-v1','status':'actual-imagegen-service-refusal','tool':'built-in image_gen.imagegen','utc':datetime.now(timezone.utc).isoformat(),'before_call_path':bp.relative_to(B.parents[2]).as_posix(),'before_call_SHA256':hashlib.sha256(bp.read_bytes()).hexdigest(),'literal_actual_tool_arguments':{'prompt':(B/f'generated/{i}-{ST}-v1-exact-call-argument.txt').read_bytes().decode('utf-8'),'referenced_image_paths':[(B.parents[2]/p).as_posix()for p in b['references']],'transparent_background':False},'error':'HTTP400 moderation_blocked; rejected by safety system; safety_violations=[abuse]; moderation_stage=output; categories=[other]','request_id':'f824a05f-2456-4f7f-8a50-81b6f2e6daf8','raw_output_exists':False,'retry_performed':False,'refusal_not_bypassed':True,'near_related_prepared_without_calls':[1761,1762]}
p=B/f'service-refusal-{ST}-1760.json';assert not p.exists();p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Actual service refusal1760 saved, no retry')

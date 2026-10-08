from pathlib import Path
import json,hashlib
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='equipment-forty-first'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
before=B/f'source-check-{ST}-before-call.json';assert before.read_bytes()==(B/f'source-check-{ST}.json').read_bytes()
detail=next(r for r in read(B/f'detail-metrics-{ST}.json')if r['id']==5333);assert detail['source_peak_xy']==[30,78]
review=B/f'review-{ST}.json';old=review.read_bytes();snapshot=B/f'review-{ST}-before-postcall-coordinate-correction.json';assert not snapshot.exists();snapshot.write_bytes(old)
def corrected(v):
 if isinstance(v,str):return v.replace('Sourcepeak50,78 RGB214/207/214','Sourcepeak30,78 RGB214/207/214').replace('productionstrongest same50,78','productionstrongest50,78')
 if isinstance(v,list):return[corrected(x)for x in v]
 if isinstance(v,dict):return{k:corrected(x)for k,x in v.items()}
 return v
for p in [review,B/f'postcall-source-status-{ST}.json',B/f'rejected/{ST}/5333-attempt1-reason.json']:
 v=read(p);n=corrected(v);assert n!=v;p.write_text(json.dumps(n,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
cp=B/f'postcall-corrections-{ST}.json';c=read(cp);c['postcall_corrections'].append({'id':5333,'scope':'Postcall prose coordinate only; initial prose mistook productionnative peak50,78 for sourcepeak30,78. Exact BEFOREnativeRGBA/palette/prompt and numericaldetail metrics alreadycorrect, NOsourcebefore/toolargument/raw/ref changes.','actual_source_native_peak_xy':[30,78],'actual_source_peak_RGBA':[214,207,214,255],'production_inverse_at_actual_source_peak_RGB':detail['production_at_source_peak_RGB'],'production_native4x_peak_native_floor_xy':[50,78],'prior_review_snapshot':{'path':snapshot.relative_to(ROOT).as_posix(),'file_SHA256':sha(snapshot)}});save(cp,c)
r=read(review);r['postcall_corrections_ref']['file_SHA256']=sha(cp);save(review,r)
for p in [B/'make_finalize_equipment_forty_first.py',B/'finalize_equipment_forty_first.py']:
 t=p.read_text(encoding='utf-8');n=corrected(t);assert n!=t;p.write_text(n,encoding='utf-8')
mp=B/f'sha256-{ST}.json';files=list(B.glob(f'*{ST}*.json'))+list(B.glob('*equipment_forty_first*.py'))+list((B/'generated').glob(f'*{ST}*.png'))+list((B/'generated').glob(f'*{ST}*-call.json'))+list((B/f'private-{ST}').rglob('*.png'))+list((B/f'rejected/{ST}').rglob('*'))
for i in [5431,5333]:files.extend([B/f'generated/{i}-raw.png',B/f'generated/{i}-prompt.txt'])
save(mp,{p.relative_to(ROOT).as_posix():sha(p)for p in sorted(set(files))if p.is_file()and p!=mp});assert all(sha(ROOT/p)==h for p,h in read(mp).items());assert before.read_bytes()==(B/f'source-check-{ST}.json').read_bytes()
print(json.dumps({'ready':[],'held':[5431,5333],'calls':2,'postcall_coordinate_correction':True,'beforeproof_byteunchanged':True,'manifest_entries':len(read(mp)),'manifest_differences':0,'SHA256':{n:sha(B/f'{n}-{ST}.json')for n in ['jobs','review','source-check','selected','sha256']}},indent=2))

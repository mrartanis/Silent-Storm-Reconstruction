from pathlib import Path
import json,hashlib
B=Path('assets/terrain-hd/expanded');ST='effects-forty-first'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
p=B/f'source-check-{ST}.json';rows=read(p)
for r in rows:
 r['source_RGBA_RGB_A_NN_privately_inspected']=True
 r['source_RGB_whole_survey_privately_viewed']=True
 r['source_only_reason']='Genuine eight-by-eight painted frame remains pending, zero calls: one dark central field and two weak lateral washes are proven native paint, anatomical detail cannot be resolved faithfully. No analytical functional purpose inferred from name/type. Whole storedRGB and complete alpha retained.'
 r['literal_native_count_proof']='One central dark vertical paint field, two pale lateral patches, maximum RGB208/208/129 at(5,3). Full64 nativeRGBA pixels in immutable compact matrix; seven actual instance bindings do not imply seven image cells.'
save(p.name,rows);(B/f'before-call-source-check-{ST}.json').write_bytes(p.read_bytes())
save(f'jobs-{ST}.json',[])
save(f'review-{ST}.json',{'scope':ST,'status':'stable-source-only','selected_ids':[3732],'ready_ids':[],'held_ids':[],'generated_ids':[],'genuine_pending_ids':[3732],'original_only_recommendations':[],'imagegen_call_count':0,'interrupted_call_count':0,'records':[{'id':3732,'status':'genuine-pending-source-only','call_count':0,'reason':rows[0]['source_only_reason']}],'shared_metadata_untouched':True,'next_scope':'clothing-complete-20261008; effects halted by user scope'})
print('Effects41 source-only persisted; generated/interrupted calls0. Clothing57 no artifacts.')

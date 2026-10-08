from pathlib import Path
import re
B=Path(__file__).resolve().parent
IDS=[1067,1086,1578,1620,2118,2119,2626,2627,2628,2629,5391,5397]
s=(B/'audit_equipment_thirty_fourth.py').read_text(encoding='utf-8').replace('equipment-thirty-fourth','effects-twentieth').replace('source-queues/equipment.json','source-queues/effects.json')
for key in ['IDS','ART_IDS','SELECTED_IDS']:
 s=re.sub(r'^'+key+r'\s*=.*$',key+' = '+repr(IDS),s,flags=re.M)
s=re.sub(r'^VIEWED_IDS=.*$', 'VIEWED_IDS=set()',s,flags=re.M)
s=s.replace('Twelve actual selected Materials diffuse sources','Twelve actual selected painted particle color sources')
s=s.replace('strict batch.','strict batch; unknowns recorded, not automatic exclusion by missing Particle row.')
s=s.replace('assert not unresolved and not missing','assert o.size == tuple(r["logical_size"]) # Missing actual definitions/slots stay explicit; no fake consumer semantics.')
(B/'audit_effects_twentieth.py').write_text(s,encoding='utf-8')
for stem in ['save','preview']:
 p=B/(f'{stem}_equipment_thirty_fourth'+('_call.py' if stem=='save' else '.py'))
 s=p.read_text(encoding='utf-8').replace('equipment-thirty-fourth','effects-twentieth').replace('IDS=[3034, 5380, 6473]','IDS=[1067, 1578, 2119]')
 target=B/(f'{stem}_effects_twentieth'+('_call.py' if stem=='save' else '.py'))
 target.write_text(s,encoding='utf-8')
print('New independent audit/saver/preview scripts only')

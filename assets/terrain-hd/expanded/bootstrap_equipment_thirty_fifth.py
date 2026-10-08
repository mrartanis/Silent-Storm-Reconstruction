from pathlib import Path
import re
B=Path(__file__).resolve().parent
IDS=[675,2213,3384,3783,3784,4867,4948,5652,5676,5677,6669,7653]
s=(B/'audit_equipment_thirty_fourth.py').read_text(encoding='utf-8').replace('equipment-thirty-fourth','equipment-thirty-fifth')
for key in ['IDS','ART_IDS','SELECTED_IDS']:s=re.sub(r'^'+key+r'\s*=.*$',key+' = '+repr(IDS),s,flags=re.M)
s=re.sub(r'^VIEWED_IDS=.*$', 'VIEWED_IDS=set()',s,flags=re.M)
(B/'audit_equipment_thirty_fifth.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';src=B/(f'{stem}_equipment_thirty_fourth'+suffix)
 s=src.read_text(encoding='utf-8').replace('equipment-thirty-fourth','equipment-thirty-fifth').replace('IDS=[3034, 5380, 6473]','IDS=[4867, 5676, 7653]')
 (B/(f'{stem}_equipment_thirty_fifth'+suffix)).write_text(s,encoding='utf-8')
print('Own Equipment35 scripts only')

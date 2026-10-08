from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='equipment-thirty-sixth';IDS=[5235,5236,5237,5243,5244,5248,5249,5329,5331,5333,5383,5390];CALL=[5236,5390]
s=(B/'audit_equipment_thirty_fifth.py').read_text(encoding='utf-8').replace('equipment-thirty-fifth',ST)
for name in ['IDS','ART_IDS','SELECTED_IDS']:s=re.sub(r'(?m)^'+name+r' = .*$',name+' = '+repr(IDS),s)
(B/'audit_equipment_thirty_sixth.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_equipment_thirty_fifth'+suffix)).read_text(encoding='utf-8').replace('equipment-thirty-fifth',ST);s=re.sub(r'(?m)^IDS=.*$','IDS='+repr(CALL),s)
 (B/(f'{stem}_equipment_thirty_sixth'+suffix)).write_text(s,encoding='utf-8')
s=(B/'precall_guard_heads_fifteenth.py').read_text(encoding='utf-8').replace('heads-fifteenth',ST);(B/'precall_guard_equipment_thirty_sixth.py').write_text(s,encoding='utf-8')
print('OwnEq36 scripts only')

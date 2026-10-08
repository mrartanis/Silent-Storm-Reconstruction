from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='equipment-thirty-seventh';IDS=[5238,5239,5240,5241,5242,5245,5246,5247,5250,5251,5372,5409];CALL=[5238]
s=(B/'audit_equipment_thirty_sixth.py').read_text(encoding='utf-8').replace('equipment-thirty-sixth',ST)
for name in ['IDS','ART_IDS','SELECTED_IDS']:s=re.sub(r'(?m)^'+name+r' = .*$',name+' = '+repr(IDS),s)
(B/'audit_equipment_thirty_seventh.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_equipment_thirty_sixth'+suffix)).read_text(encoding='utf-8').replace('equipment-thirty-sixth',ST);s=re.sub(r'(?m)^IDS=.*$','IDS='+repr(CALL),s)
 (B/(f'{stem}_equipment_thirty_seventh'+suffix)).write_text(s,encoding='utf-8')
(B/'precall_guard_equipment_thirty_seventh.py').write_text((B/'precall_guard_equipment_thirty_sixth.py').read_text(encoding='utf-8').replace('equipment-thirty-sixth',ST),encoding='utf-8')

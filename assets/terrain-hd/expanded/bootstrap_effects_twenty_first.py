from pathlib import Path
import re
B=Path(__file__).resolve().parent
IDS=[717,718,893,894,895,896,897,898,901,1086,1620,5397]
s=(B/'audit_effects_twentieth.py').read_text(encoding='utf-8').replace('effects-twentieth','effects-twenty-first')
for key in ['IDS','ART_IDS','SELECTED_IDS']:s=re.sub(r'^'+key+r'\s*=.*$',key+' = '+repr(IDS),s,flags=re.M)
s=re.sub(r'^VIEWED_IDS=.*$', 'VIEWED_IDS=set()',s,flags=re.M)
(B/'audit_effects_twenty_first.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_effects_twentieth'+suffix)).read_text(encoding='utf-8').replace('effects-twentieth','effects-twenty-first').replace('IDS=[1067, 1578, 2119]','IDS=[1086, 1620, 5397]')
 (B/(f'{stem}_effects_twenty_first'+suffix)).write_text(s,encoding='utf-8')
print('New effects21 independent scripts only')

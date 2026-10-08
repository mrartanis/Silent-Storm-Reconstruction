from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='effects-twenty-second';IDS=[1078,2485,2118,2575,2577,4739,4744,5261,5391,5392,2523,2626];CALL=[1078,4739,4744]
s=(B/'audit_effects_twenty_first.py').read_text(encoding='utf-8').replace('effects-twenty-first',ST)
for name in ['IDS','ART_IDS','SELECTED_IDS']:s=re.sub(r'(?m)^'+name+r' = .*$',name+' = '+repr(IDS),s)
(B/'audit_effects_twenty_second.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_effects_twenty_first'+suffix)).read_text(encoding='utf-8').replace('effects-twenty-first',ST);s=re.sub(r'(?m)^IDS=.*$','IDS='+repr(CALL),s)
 (B/(f'{stem}_effects_twenty_second'+suffix)).write_text(s,encoding='utf-8')
print('Own Effects22 scripts only')

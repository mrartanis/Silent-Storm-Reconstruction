from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='effects-twenty-third';IDS=[2118,2523,2575,2577,2626,2627,2628,2629,5261,717,901,902];CALL=[5261]
s=(B/'audit_effects_twenty_second.py').read_text(encoding='utf-8').replace('effects-twenty-second',ST)
for name in ['IDS','ART_IDS','SELECTED_IDS']:s=re.sub(r'(?m)^'+name+r' = .*$',name+' = '+repr(IDS),s)
(B/'audit_effects_twenty_third.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_effects_twenty_second'+suffix)).read_text(encoding='utf-8').replace('effects-twenty-second',ST);s=re.sub(r'(?m)^IDS=.*$','IDS='+repr(CALL),s)
 (B/(f'{stem}_effects_twenty_third'+suffix)).write_text(s,encoding='utf-8')
(B/'precall_guard_effects_twenty_third.py').write_text((B/'precall_guard_effects_twenty_second.py').read_text(encoding='utf-8').replace('effects-twenty-second',ST),encoding='utf-8')

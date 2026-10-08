from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='heads-sixteenth';IDS=[6692,6698,6699,6700,6701,6703,6709,6713,6714,6719,6720,6721]
s=(B/'audit_heads_fifteenth.py').read_text(encoding='utf-8').replace('heads-fifteenth',ST)
s=re.sub(r'IDS=\[[^\n]+','IDS='+repr(IDS),s,count=1)
s=s.replace("['tenth','eleventh','twelfth','thirteenth','fourteenth']","['tenth','eleventh','twelfth','thirteenth','fourteenth','fifteenth']")
s=s.replace('Root32 published c30f7b43:2400nativevalidated/q2862 task baseline.','Root34 published10df1911:2414nativevalidated/q2876 task baseline.')
(B/'audit_heads_sixteenth.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_heads_fifteenth'+suffix)).read_text(encoding='utf-8').replace('heads-fifteenth',ST);s=re.sub(r'(?m)^IDS=.*$','IDS=[6698,6719,6720]',s)
 (B/(f'{stem}_heads_sixteenth'+suffix)).write_text(s,encoding='utf-8')
(B/'precall_guard_heads_sixteenth.py').write_text((B/'precall_guard_heads_fifteenth.py').read_text(encoding='utf-8').replace('heads-fifteenth',ST),encoding='utf-8')

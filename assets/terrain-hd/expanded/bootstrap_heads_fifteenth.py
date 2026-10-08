from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='heads-fifteenth';IDS=[6695,6697,6702,6704,6710,6711,6712,6713,6714,6717,6718,6722]
s=(B/'audit_heads_fourteenth.py').read_text(encoding='utf-8').replace('heads-fourteenth',ST)
s=re.sub(r'IDS=\[[^\n]+','IDS='+repr(IDS),s,count=1)
s=s.replace("['tenth','eleventh','twelfth','thirteenth']","['tenth','eleventh','twelfth','thirteenth','fourteenth']")
s=s.replace('Root2400source entries/q2862 pending32 validation at task dispatch; committed/pushed2390 per coordinator. Fresh source entry counts are not automatically accepted production counts.','Root32 published c30f7b43:2400nativevalidated/q2862 task baseline. Fresh source entries may include newer pending-validation; queue statuses and root validation authoritative.')
(B/'audit_heads_fifteenth.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_heads_fourteenth'+suffix)).read_text(encoding='utf-8').replace('heads-fourteenth',ST);s=re.sub(r'(?m)^IDS=.*$','IDS=[6710,6711,6712]',s)
 (B/(f'{stem}_heads_fifteenth'+suffix)).write_text(s,encoding='utf-8')
print('Independent Heads15 scripts only')

from pathlib import Path
import re
B=Path(__file__).resolve().parent;ST='heads-fourteenth';IDS=[1126,1127,1128,6691,3322,3323,3324,5259,6772,6773,1897,2183]
s=(B/'audit_heads_thirteenth.py').read_text(encoding='utf-8').replace('heads-thirteenth',ST)
s=re.sub(r"IDS=\[[^\n]+",'IDS='+repr(IDS),s,count=1)
s=s.replace("['tenth','eleventh','twelfth']","['tenth','eleventh','twelfth','thirteenth']")
s=s.replace("'source_entry_count_at_audit':2379,'queue_entry_count_at_audit':2841","'source_entry_count_at_audit':len(json.loads((B.parent/'sources.json').read_text(encoding='utf-8'))['textures']),'queue_entry_count_at_audit':len(json.loads((B/'queue.json').read_text(encoding='utf-8')))")
s=s.replace('Eight entries pending root native validation; source entry count is not accepted production count. Committed validated2371/q2833 per coordinator.', 'Root2400source entries/q2862 pending32 validation at task dispatch; committed/pushed2390 per coordinator. Fresh source entry counts are not automatically accepted production counts.')
(B/'audit_heads_fourteenth.py').write_text(s,encoding='utf-8')
for stem in ['save','preview','detail']:
 suffix='_call.py'if stem=='save'else'.py';s=(B/(f'{stem}_heads_thirteenth'+suffix)).read_text(encoding='utf-8').replace('heads-thirteenth',ST)
 s=re.sub(r'(?m)^IDS=.*$','IDS=[1126,3323,3324]',s)
 (B/(f'{stem}_heads_fourteenth'+suffix)).write_text(s,encoding='utf-8')
print('Independent Heads14 scripts only')

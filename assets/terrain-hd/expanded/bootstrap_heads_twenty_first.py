from pathlib import Path
B=Path(__file__).resolve().parent
old='heads-twentieth';new='heads-twenty-first';ids='[3322,5259,6691,6699,6701,6718,6721,6772,6773,6781,6782,6921]'
s=(B/'audit_heads_twentieth.py').read_text(encoding='utf-8').replace(old,new).replace('[6692,6697,6699,6700,6718,6721,6722,6772,6773,6774,6775,6776]',ids)
(B/'audit_heads_twenty_first.py').write_text(s,encoding='utf-8')
for name in ['save_heads_twentieth_call.py','precall_guard_heads_twentieth.py','preview_heads_twentieth.py','material_phase_heads_twentieth.py','detail_heads_twentieth.py','postcall_material_heads_twentieth.py']:
 s=(B/name).read_text(encoding='utf-8').replace(old,new).replace('[6697,6700,6722]','[3322,5259,6691]')
 (B/name.replace('twentieth','twenty_first')).write_text(s,encoding='utf-8')
print('OwnHeads21 originalnative RGBtoolreference/privateNN, oldscopes unchanged')

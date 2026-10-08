from pathlib import Path
B=Path(__file__).resolve().parent;ST='heads-twenty-second';IDS=[6699,6701,6702,6703,6708,6709,6713,6714,6718,6721,6728,6783];CALL=[6718,6721]
p=B/'audit_heads_twenty_second.py';assert not p.exists();s=(B/'audit_heads_twenty_first.py').read_text(encoding='utf-8').replace('heads-twenty-first',ST).replace('IDS=[3322,5259,6691,6699,6701,6718,6721,6772,6773,6781,6782,6921]','IDS='+repr(IDS))
needle="'all_actual_ComplexHeads_refs':[ref('ComplexHeads',v['ID'])for v in complexheads],"
replacement=needle+"'all_actual_THMID_texture_sibling_refs':[{'layer_ref':ref(tab,v['ID']),'texture_ref':ref('Textures',v['TextureID']),'actual_native_Type':T['Textures'][v['TextureID']]['Type'],'role':'registration/source layer row only; not selected imagegen nor inferred runtimeweight'}for tab in dyn_tables for v in T[tab].values()if v['THMID']==row['THMID']],"
assert needle in s;s=s.replace(needle,replacement)
p.write_text(s,encoding='utf-8')
for prefix in ['save','precall_guard','preview','material_phase','detail','postcall_material']:
 src=B/(f'{prefix}_equipment_forty_second'+('_call.py'if prefix=='save'else'.py'));out=B/(f'{prefix}_heads_twenty_second'+('_call.py'if prefix=='save'else'.py'));assert not out.exists();t=src.read_text(encoding='utf-8').replace('equipment-forty-second',ST).replace('[1915, 1919, 1920]',repr(CALL)).replace('[1915,1919,1920]',repr(CALL));out.write_text(t,encoding='utf-8')
print('OwnHeads22 allTHMID sibling source texture rows refs, two provisional FIRST targets')

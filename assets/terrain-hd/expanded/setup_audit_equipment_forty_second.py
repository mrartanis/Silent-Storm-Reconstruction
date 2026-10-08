from pathlib import Path
B=Path(__file__).resolve().parent;ST='equipment-forty-second';IDS=[1914,1915,1916,1917,1918,1919,1920,1977,3035,3039,3914,5402];CALL=[1915,1919,1920]
for prefix in ['audit','save','precall_guard','preview','material_phase','detail','postcall_material']:
 src=B/(f'{prefix}_equipment_forty_first'+('_call.py'if prefix=='save'else'.py'));out=B/(f'{prefix}_equipment_forty_second'+('_call.py'if prefix=='save'else'.py'));assert not out.exists();s=src.read_text(encoding='utf-8').replace('equipment-forty-first',ST)
 if prefix=='audit':s=s.replace('IDS=[5431,5329,5333,5235,5236,5239,5243,5244,3384,6901,6902,2213]','IDS='+repr(IDS))
 if prefix in ['preview','material_phase']:s=s.replace('[5431,5333]',repr(CALL))
 out.write_text(s,encoding='utf-8')
print('Independent Eq42 12selected/3provisional FIRST targets; no frozen changes')

from pathlib import Path
B=Path(__file__).resolve().parent;ST='clothing-forty-second';IDS=[3847,3848,3892,4442,4451,4549,5673,6812,6816,7610,7632,7641];CALL=[6812,6816,7632]
for prefix in['audit','save','precall_guard','preview','material_phase','detail','postcall_material']:
 src=B/(f'{prefix}_equipment_forty_second'+('_call.py'if prefix=='save'else'.py'));p=B/(f'{prefix}_clothing_forty_second'+('_call.py'if prefix=='save'else'.py'));assert not p.exists();s=src.read_text(encoding='utf-8').replace('equipment-forty-second',ST)
 if prefix=='audit':s=s.replace('IDS=[1914, 1915, 1916, 1917, 1918, 1919, 1920, 1977, 3035, 3039, 3914, 5402]','IDS='+repr(IDS))
 if prefix in['preview','material_phase']:s=s.replace('[1915, 1919, 1920]',repr(CALL)).replace('[1915,1919,1920]',repr(CALL))
 p.write_text(s,encoding='utf-8')
print('OwnClothing42 12actualsources/3provisional FIRST targets')

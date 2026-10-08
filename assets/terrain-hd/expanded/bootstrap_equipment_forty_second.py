from pathlib import Path
B=Path(__file__).resolve().parent;ST='equipment-forty-second'
for prefix in ['guard','survey']:
 p=B/f'{prefix}_equipment_forty_second.py';assert not p.exists();s=(B/f'{prefix}_equipment_forty_first.py').read_text(encoding='utf-8').replace('equipment-forty-first',ST);p.write_text(s,encoding='utf-8')
(B/f'private-{ST}/survey').mkdir(parents=True,exist_ok=True)
print('IndependentEq42 freshguard scripts only, old41 unchanged')

from pathlib import Path
B=Path(__file__).resolve().parent;ST='clothing-forty-second'
p=B/'guard_clothing_forty_second.py';assert not p.exists();p.write_text((B/'guard_equipment_forty_second.py').read_text(encoding='utf-8').replace('equipment-forty-second',ST).replace('groups/source-queues/equipment.json','groups/source-queues/characters-clothing.json'),encoding='utf-8');(B/f'private-{ST}/survey').mkdir(parents=True,exist_ok=True)
print('Independent clothing42 freshguard only')

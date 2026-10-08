from pathlib import Path
B=Path(__file__).resolve().parent;ST='heads-twenty-second'
p=B/'guard_heads_twenty_second.py';assert not p.exists();p.write_text((B/'guard_heads_twenty_first.py').read_text(encoding='utf-8').replace('heads-twenty-first',ST),encoding='utf-8');(B/f'private-{ST}/survey').mkdir(parents=True,exist_ok=True)
print('Independent Heads22 freshguard only')

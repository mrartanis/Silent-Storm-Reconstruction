from pathlib import Path
B=Path(__file__).resolve().parent;OLD='clothing-complete-20261008';NEW='clothing-tail-complete-20261008'
files=[('work_clothing_complete_20261008.py','work_clothing_tail_complete_20261008.py'),('prepare_guided_clothing_complete_20261008.py','prepare_guided_clothing_tail_complete_20261008.py')]
for src,dst in files:
 p=B/dst;assert not p.exists();s=(B/src).read_text(encoding='utf-8').replace(f"ST='{OLD}'",f"ST='{NEW}';AUDIT_ST='{OLD}'")
 s=s.replace("source-check-{ST}","source-check-{AUDIT_ST}").replace("actual-records-{ST}","actual-records-{AUDIT_ST}").replace("consumer-code-proof-{ST}","consumer-code-proof-{AUDIT_ST}")
 if dst.startswith('work_'):
  s=s.replace("mode=sys.argv[1]","assert int(sys.argv[2]) in read(B/'handoff-clothing-coordinator-tail20-20261008-to-clothing-complete.json')['IDs']\nmode=sys.argv[1]")
  s=s.replace("out.mkdir(exist_ok=True)","out.mkdir(parents=True,exist_ok=True)")
 else:
  s=s.replace("i=int(sys.argv[1]);","i=int(sys.argv[1]);assert i in read(B/'handoff-clothing-coordinator-tail20-20261008-to-clothing-complete.json')['IDs'];(B/f'private-{ST}/sources').mkdir(parents=True,exist_ok=True);")
 p.write_text(s,encoding='utf-8')
print('Separate tail worker prefix initialized; root files unchanged')

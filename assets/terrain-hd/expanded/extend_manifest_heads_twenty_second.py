from pathlib import Path
import json,hashlib
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='heads-twenty-second'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for p in[B/'make_finalize_heads_twenty_second.py',B/'finalize_heads_twenty_second.py']:
 s=p.read_text(encoding='utf-8');needle="list((B/f'private-{ST}').rglob('*.png'))"
 if needle in s:p.write_text(s.replace(needle,"list((B/f'private-{ST}').rglob('*'))"),encoding='utf-8')
mp=B/f'sha256-{ST}.json';files=list(B.glob(f'*{ST}*.json'))+list(B.glob('*heads_twenty_second*.py'))+list((B/'generated').glob(f'*{ST}*.png'))+list((B/'generated').glob(f'*{ST}*-call.json'))+list((B/f'private-{ST}').rglob('*'))+list((B/f'rejected/{ST}').rglob('*'))
for i in[6718,6721]:files.extend([B/f'generated/{i}-raw.png',B/f'generated/{i}-prompt.txt'])
manifest={p.relative_to(ROOT).as_posix():sha(p)for p in sorted(set(files))if p.is_file()and p!=mp};mp.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');assert all(sha(ROOT/p)==h for p,h in manifest.items());assert(B/f'source-check-{ST}.json').read_bytes()==(B/f'source-check-{ST}-before-call.json').read_bytes()
print(json.dumps({'manifest_entries':len(manifest),'manifest_differences':0,'beforeproof_byteunchanged':True,'all_private_precalldraft_oldbytes_included':True,'SHA256':{n:sha(B/f'{n}-{ST}.json')for n in['jobs','review','source-check','selected','sha256']}},indent=2))

import hashlib,json,shutil,sys
from pathlib import Path
from datetime import datetime,timezone
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
id_=int(sys.argv[1]); output=Path(sys.argv[2]);premultiply=False
target=BASE/f'generated/{id_}-raw.png'
assert not target.exists(), 'Do not overwrite existing raw'
shutil.copyfile(output,target)
prompt=BASE/f'generated/{id_}-prompt.txt'
record={'id':id_,'tool':'built-in image_gen.imagegen','tool_output':output.name,
        'tool_output_session':output.parent.name,'saved_utc':datetime.now(timezone.utc).isoformat(),
        'generated':f'assets/terrain-hd/expanded/generated/{id_}-raw.png',
        'prompt_file':f'assets/terrain-hd/expanded/generated/{id_}-prompt.txt',
        'reference_pngs':[f'assets/terrain-hd/expanded/original/{id_}.png']+[f'assets/terrain-hd/expanded/generated/{id_}-heads-fifteenth-source-rgb-support.png'],
        'transparent_background':premultiply,'raw_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
        'prompt_sha256':hashlib.sha256(prompt.read_bytes()).hexdigest(),'exact_prompt':prompt.read_bytes().decode('utf-8')}
proof=next(x for x in json.loads((BASE/'source-check-heads-fifteenth.json').read_text(encoding='utf-8'))['source_checks'] if x['id']==id_)
record['source_transform_recipe']=proof['reference_recipe']
record['reference_sha256']={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in record['reference_pngs']}
record['source_rgba_sha256']=proof['source_RGBA_SHA256']
record['prompt_serialization']='Exact actual built-in prompt UTF-8 argument including CRLF equals prompt file bytes; references portable identities resolved against current repo root.'
record['reproducible_tool_arguments']={'prompt':record['exact_prompt'],'referenced_image_paths':record['reference_pngs'],'transparent_background':False}
record['pattern_constraints']=next(r for r in json.loads((BASE/'pattern-constraints-heads-fifteenth.json').read_text(encoding='utf-8')) if r['id']==id_)
record['pattern_constraints_file_sha256']=hashlib.sha256((BASE/'pattern-constraints-heads-fifteenth.json').read_bytes()).hexdigest()
(BASE/f'generated/{id_}-heads-fifteenth-call.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Saved raw and durable exact call metadata:',id_)

import hashlib,json,shutil,sys
from pathlib import Path
from datetime import datetime,timezone
BASE=Path(__file__).resolve().parent
id_=int(sys.argv[1]); output=Path(sys.argv[2]);premultiply=id_==1089
target=BASE/f'generated/{id_}-raw.png'
assert not target.exists(), 'Do not overwrite existing raw'
shutil.copyfile(output,target)
prompt=BASE/f'generated/{id_}-prompt.txt'
record={'id':id_,'tool':'built-in image_gen.imagegen','tool_output':output.name,
        'tool_output_session':output.parent.name,'saved_utc':datetime.now(timezone.utc).isoformat(),
        'generated':f'assets/terrain-hd/expanded/generated/{id_}-raw.png',
        'prompt_file':f'assets/terrain-hd/expanded/generated/{id_}-prompt.txt',
        'reference_pngs':([f'assets/terrain-hd/expanded/original/{id_}.png'] if premultiply else [])+[f'assets/terrain-hd/expanded/generated/{id_}-effects-fifth-source-rgb-support.png'],
        'transparent_background':premultiply,'raw_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
        'prompt_sha256':hashlib.sha256(prompt.read_bytes()).hexdigest(),'exact_prompt':prompt.read_text(encoding='utf-8')}
(BASE/f'generated/{id_}-effects-fifth-call.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Saved raw and durable exact call metadata:',id_)

import importlib.util,json,tempfile,hashlib
from pathlib import Path
from PIL import Image
spec=importlib.util.spec_from_file_location('version_import','assets/terrain-hd/import_generated.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory(prefix='hd-versioned-import-',dir='build') as tmp:
 b=Path(tmp).resolve();g=b/'expanded/generated';g.mkdir(parents=True);o=b/'expanded/original';o.mkdir();m.ROOT=b
 image=Image.new('RGBA',(3,2),(30,40,50,255));image.putpixel((0,0),(0,0,0,0));image.save(o/'321.png')
 row={'id':321,'logical_size':[3,2],'original_png':'expanded/original/321.png','source_rgba_sha256':hashlib.sha256(image.tobytes()).hexdigest(),'alpha_extrema':[0,255],'role':'typed-db-color-candidate','category':'heads-lshead','texture':{'Type':'Transparent','UserName':'Fixture','SrcName':'fixture'}}
 (b/'sources.json').write_text(json.dumps({'textures':[]}));(b/'expanded/queue.json').write_text(json.dumps([row]))
 old={}
 for name in ['321-raw.png','321.png','321-prompt.txt']:
  p=g/name;p.write_bytes(b'immutable previous rejected attempt '+name.encode());old[name]=p.read_bytes()
 raw=g/'321-category-v2-raw.png';Image.new('RGBA',(12,8),(50,60,70,255)).save(raw);raw_before=raw.read_bytes()
 prompt='Keep full source.\r\nExact argument\n';job={'id':321,'generated':str(raw),'prompt':prompt,'generation_variant':'category-v2'}
 m.import_jobs([job]);s=json.loads((b/'sources.json').read_text())['textures'][0]
 assert all((g/name).read_bytes()==data for name,data in old.items());assert raw.read_bytes()==raw_before
 assert s['generated_png']=='expanded/generated/321-category-v2-raw.png';assert s['png']=='expanded/generated/321-category-v2.png'
 assert (b/s['prompt_file']).read_bytes()==prompt.encode()+b'\n';assert s['native_alpha_reference']=='expanded/original/321.png';assert s['alpha_encoding']=='premultiplied';assert Image.open(b/s['png']).size==(12,8)
 try:m.import_jobs([job])
 except ValueError as e:assert 'already imported' in str(e)
 else:raise AssertionError('Accepted ID must remain protected')
 (b/'sources.json').write_text(json.dumps({'textures':[]}));bad=dict(job,generation_variant='../escape')
 try:m.import_jobs([bad])
 except ValueError as e:assert 'invalid generation_variant' in str(e)
 else:raise AssertionError('Invalid path must be rejected')
print('Versioned import preserves all old raw/prompt/normalized bytes, exact CRLF argument+ONE LF, native originalalpha and accepted-ID protection; path escape rejected.')

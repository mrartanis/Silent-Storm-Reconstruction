import importlib.util,json,tempfile,struct,sys
from pathlib import Path
from PIL import Image
sys.path.insert(0,'assets/terrain-hd');import prepare_sources as p;import import_generated as imp
with tempfile.TemporaryDirectory(prefix='hd-source-variant-',dir='build') as tmp:
 b=Path(tmp).resolve();(b/'expanded/original').mkdir(parents=True);(b/'historical').mkdir();(b/'release/Textures').mkdir(parents=True);p.ROOT=imp.ROOT=b
 (b/'sources.json').write_text(json.dumps({'textures':[]}));(b/'expanded/queue.json').write_text('[]')
 def mmp(pixels):return struct.pack('<6I',0x504d4d,6,0,2,2,1)+Image.frombytes('RGBA',(2,2),pixels).tobytes('raw','BGRA')
 primary=bytes([0,0,0,0]*4);variant=bytes([20,30,40,0,30,40,50,128,40,50,60,255,50,60,70,64]);i=321;alias=i|0x01000000
 for key,pixels in [(i,primary),(alias,variant)]:
  data=mmp(pixels);(b/f'historical/{key}').write_bytes(data);(b/f'release/Textures/{key}').write_bytes(data)
 original=b/'expanded/original/321.png';Image.frombytes('RGBA',(2,2),primary).save(original);old=original.read_bytes()
 row={'texture':{'ID':i,'Type':'Ordinary','Format':'dxt1','Width':2,'Height':2,'UserName':'Fixture','SrcName':'fixture'},'category':'heads-lshead','role':'typed-db-color-candidate','source_resource_id':alias,'source_resource_selection_reason':'Audited bIsDXT/usedxt0 selector chooses uncompressed original'}
 r=p.prepare([row],b/'historical',b/'release',b/'prepared.json',append=True);assert len(r)==1 and r[0]['status']=='pending';ref=b/r[0]['original_png'];assert Image.open(ref).convert('RGBA').tobytes()==variant;assert original.read_bytes()==old
 g=b/'expanded/generated';g.mkdir();raw=g/'321-category-v2-raw.png';Image.new('RGBA',(8,8),(35,45,55,255)).save(raw)
 imp.import_jobs([{'id':i,'generated':str(raw),'prompt':'Exact original variant','generation_variant':'category-v2'}]);asset=json.loads((b/'sources.json').read_text())['textures'][0]
 assert asset['source_resource_id']==alias and asset['native_alpha_reference']==r[0]['original_png'];assert asset['reference_png']==r[0]['original_png'];assert original.read_bytes()==old
 prior=dict(r[0],status='structural-mask-or-solid',reason='Old compressed-primary-only classification')
 (b/'sources.json').write_text(json.dumps({'textures':[]}));(b/'expanded/queue.json').write_text(json.dumps([prior]))
 reviewed=dict(row,reviewed_original_reclassification_reason='Whole actual UC resource is diffuse art consumed by typed material')
 result=p.prepare([reviewed],b/'historical',b/'release',b/'revisited.json',append=True,revisit_originals=[i])
 registry=json.loads((b/'expanded/queue.json').read_text());assert len(registry)==1 and registry[0]['status']=='pending';assert registry[0]['previous_original_disposition']==prior;assert original.read_bytes()==old
 (b/'expanded/queue.json').write_text(json.dumps([prior]))
 try:p.prepare([row],b/'historical',b/'release',b/'unreviewed.json',append=True,revisit_originals=[i])
 except ValueError:pass
 else:raise AssertionError('Unreviewed original revisit must be rejected')
 (b/'sources.json').write_text(json.dumps({'textures':[asset]}))
 try:p.prepare([reviewed],b/'historical',b/'release',b/'accepted.json',append=True,revisit_originals=[i])
 except ValueError:pass
 else:raise AssertionError('Accepted texture revisit must be rejected')
 (b/'sources.json').write_text(json.dumps({'textures':[]}));(b/'expanded/queue.json').write_text('[]')
 for bad in [dict(row,source_resource_id=999),dict(row,source_resource_selection_reason='')]:
  try:p.prepare([bad],b/'historical',b/'release',b/'bad.json')
  except ValueError:pass
  else:raise AssertionError('Unproved/wrong alias must be rejected')
print('Selected full native RGBA/A reference follows audited loader variant; old primary PNG immutable, provenance propagates, wrong/unproved alias rejected.')

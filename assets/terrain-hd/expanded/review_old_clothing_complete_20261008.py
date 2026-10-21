from pathlib import Path
import json,sys,hashlib
import numpy as np
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent; ROOT=B.parents[2]; ST='clothing-complete-20261008'
sys.path.insert(0,str(B.parent))
from match_brightness import stored_luma,match_cell,pad_rgb_under_source_mask
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
out=B/f'private-{ST}/old-selected';out.mkdir(exist_ok=True)
metrics=[]
for r in read(B/f'source-check-{ST}.json'):
 if r['initial_status']!='pending-art-review' or not r['prior_history']['raws']:continue
 i=r['id']; a=next(a for a in r['aliases'] if a['resource_id']==r['resource_when_gfx_texture_usedxt_0'])
 source=ROOT/a['fullRGBA_PNG']; o=Image.open(source).convert('RGBA')
 generic=B/f'generated/{i}-raw.png'; raw=generic if generic.exists() else ROOT/r['prior_history']['raws'][0]['path']
 im=Image.open(raw).convert('RGB'); size=(o.width*4,o.height*4); alpha=o.getchannel('A').resize(size,Image.Resampling.LANCZOS)
 inp=im.resize(size,Image.Resampling.LANCZOS).convert('RGBA'); padded,pc,cc=pad_rgb_under_source_mask(inp,alpha)
 corrected,gain,lum,rounding=match_cell(padded,stored_luma(o),r['actual_native_Type']=='Transparent',alpha);corrected.putalpha(alpha)
 pure=out/f'{i}-pure.png';corrected.save(pure);inv=corrected.convert('RGB').resize(o.size,Image.Resampling.LANCZOS);inverse=out/f'{i}-inverse.png';inv.save(inverse)
 v=np.array(inv).astype(float);s=np.array(o.convert('RGB')).astype(float)
 metrics.append({'id':i,'source_RGBA':rel(source),'source_resource_id':a['resource_id'],'source_RGBA_SHA256':a['native_RGBA_SHA256'],'raw':rel(raw),'raw_SHA256':sha(raw),'pure':rel(pure),'inverse':rel(inverse),'native_size':list(o.size),'gain':gain,'luma_target':stored_luma(o),'luma_actual':lum,'source_alpha_restored_exact':corrected.getchannel('A').tobytes()==alpha.tobytes(),'padding_count':pc,'cleared_count':cc,'correlation':float(np.corrcoef(v.ravel(),s.ravel())[0,1]),'old_history':r['prior_history']})
(B/f'old-selected-metrics-{ST}.json').write_text(json.dumps(metrics,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for start in range(0,len(metrics),6):
 c=Image.new('RGB',(3*535,2*295),(25,25,25));d=ImageDraw.Draw(c)
 for k,m in enumerate(metrics[start:start+6]):
  o=Image.open(ROOT/m['source_RGBA']).convert('RGB');v=Image.open(ROOT/m['inverse']);x=k%3*535;y=k//3*295
  sz=o.size
  if max(sz)>256:sz=(int(o.width*256/max(sz)),int(o.height*256/max(sz)))
  c.paste(o.resize(sz),(x,y+26));c.paste(v.resize(sz),(x+270,y+26));d.text((x,y),f'{m["id"]} selected original / old raw scalar {m["gain"]:.3f}',fill='white')
 c.save(out/f'survey-{start:03}.png')
print('Whole selected native originals and brightness-normalized old raws:',len(metrics))

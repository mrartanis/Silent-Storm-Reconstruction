from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-forty-third'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
out=[]
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');s=o.convert('RGB');sz=(o.width*4,o.height*4);s4=s.resize(sz,Image.Resampling.LANCZOS);sr=s4.resize(o.size,Image.Resampling.LANCZOS);p4=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');pn=p4.resize(o.size,Image.Resampling.LANCZOS);regions=[]
 for d in c['source_domains']:
  x0,y0,x1,y1=d['bbox'];n=np.array(s)[y0:y1,x0:x1];rnd=np.array(sr)[y0:y1,x0:x1];p=np.array(pn)[y0:y1,x0:x1];ss=np.array(s4)[y0*4:y1*4,x0*4:x1*4];pp=np.array(p4)[y0*4:y1*4,x0*4:x1*4]
  regions.append({'name':d['name'],'native_bbox':d['bbox'],'original_native_mean_std_RGB':[n.mean((0,1)).tolist(),n.std((0,1)).tolist()],'source_RGB_FIRST_4x_LANCZOS_mean_std_RGB':[ss.mean((0,1)).tolist(),ss.std((0,1)).tolist()],'production4x_storedRGB_mean_std_RGB':[pp.mean((0,1)).tolist(),pp.std((0,1)).tolist()],'source_ROUNDTRIP_native_mean_RGB':rnd.mean((0,1)).tolist(),'production_inverse_native_mean_RGB':p.mean((0,1)).tolist()})
 z=(np.array(o.getchannel('A'))==0);src=np.array(s);prod=np.array(pn)
 out.append({'id':i,'qualification':'Numerical material/phase diagnostics only. ENTIRE sourceRGB FIRST independently LANCZOS4x/whole inverse, originalA separately restored. No alpha-aware Pillow RGBA resizing/unpremultiply, no region gain/crop/artist repair. Source LANCZOS roundtrip can naturally soften one native sentinelpixel; compare that too rather than claiming its255 loss alone is artistic drift.','source_original_RGBA_corner':list(o.getpixel((0,o.height-1))),'source_RGB_roundtrip_corner':list(sr.getpixel((0,o.height-1))),'production_RGB_inverse_corner':list(pn.getpixel((0,o.height-1))),'sourceA0_native_pixel_count':int(z.sum()),'sourceA0_storedRGB_original_mean':src[z].mean(0).tolist()if z.any()else None,'sourceA0_storedRGB_production_inverse_mean':prod[z].mean(0).tolist()if z.any()else None,'domains':regions})
(B/f'postcall-material-details-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print([(r['id'],r['source_original_RGBA_corner'],r['source_RGB_roundtrip_corner'],r['production_RGB_inverse_corner'])for r in out])

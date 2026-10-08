from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='heads-twenty-first';i=6691
s=np.array(Image.open(B/f'original/{i}.png').convert('RGB'),dtype=float)
g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize((64,64),Image.Resampling.LANCZOS),dtype=float)
regions=[]
for name,b in [('upper existing reflectionpaint',[28,17,39,29]),('lower existing reflectionpaint',[28,29,39,43])]:
 x0,y0,x1,y1=b;v=s[y0:y1,x0:x1];w=g[y0:y1,x0:x1];levels=[]
 for threshold in [130,150,170,190]:
  row={'meanRGB_threshold':threshold}
  for n,a in [('source',v),('production',w)]:
   yy,xx=np.where(a.mean(2)>=threshold);row[n]={'literal_native_pixel_count':len(xx),'bbox':([x0+int(xx.min()),y0+int(yy.min()),x0+int(xx.max())+1,y0+int(yy.max())+1]if len(xx)else None)}
  levels.append(row)
 regions.append({'name':name,'native_bbox':b,'source_mean_RGB':v.mean((0,1)).tolist(),'production_mean_RGB':w.mean((0,1)).tolist(),'source_RGB_max':v.max((0,1)).tolist(),'production_RGB_max':w.max((0,1)).tolist(),'photometric_profile':levels})
p={'id':i,'qualification':'Postcall numerical photometric phase diagnostics only: thresholds describe original existing blurred colorpaint intensity profiles, NOT anatomical glint/component counts/acceptance thresholds. No source BEFOREproof rewrite or artisticRGBrepair. SourceRGB and productioninverse resized independently fromA.','regions':regions}
(B/f'eye-phase-{ST}.json').write_text(json.dumps(p,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(p,indent=2))

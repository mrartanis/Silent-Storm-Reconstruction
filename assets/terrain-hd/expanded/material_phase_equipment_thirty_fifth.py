from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='equipment-thirty-fifth'
cons=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));out=[]
for c in cons:
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');s=np.asarray(o)[:,:,:3].astype(float)
 p=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');g=np.asarray(p.resize(o.size,Image.Resampling.LANCZOS)).astype(float)
 ds=[]
 for d in c['source_domains']:
  x0,y0,x1,y1=d['bbox'];a=s[y0:y1,x0:x1];z=g[y0:y1,x0:x1];ds.append({'name':d['name'],'bbox':d['bbox'],'source_mean_RGB':a.mean((0,1)).tolist(),'production_inverse_mean_RGB':z.mean((0,1)).tolist(),'source_RGBstd':a.std((0,1)).tolist(),'production_inverse_RGBstd':z.std((0,1)).tolist(),'source_peak_RGB':a.max((0,1)).tolist(),'production_inverse_peak_RGB':z.max((0,1)).tolist()})
 r={'id':i,'diagnostic':'Postcall whole raw/source/defaultscalar/fullsourceA inverse-native diagnostics. Domains/thresholds NEVER used to repair/crop/composite. Exact source physical semantics not inferred from masks/maxima; full material private views authoritative.','domains':ds}
 if i==4867:
  def maxima(a):
   v=a[28:41,:3].mean((1,2));return [y+28 for y in range(1,len(v)-1)if v[y]>v[y-1]and v[y]>v[y+1]]
  r['source_six_stroke_phase_y']=maxima(s);r['production_inverse_stroke_phase_y']=maxima(g);r['source_and_generated_left_phase_rows']=[{'y':y,'source_RGB':s[y,:3].tolist(),'production_inverse_RGB':g[y,:3].tolist()}for y in range(28,44)]
 else:
  r['original_and_generated_roundfield_native_matrix']={'bbox':[30,8,46,24],'sourceRGB':s[8:24,30:46].tolist(),'production_inverse_RGB':g[8:24,30:46].tolist()}
  r['original_and_generated_tiny_outline_native_matrix']={'bbox':[26,22,32,27],'source_RGB':s[22:27,26:32].tolist(),'production_inverse_RGB':g[22:27,26:32].tolist()}
  r['original_and_generated_short_stroke_native_matrix']={'bbox':[26,34,31,35],'source_RGB':s[34:35,26:31].tolist(),'production_inverse_RGB':g[34:35,26:31].tolist()}
 out.append(r)
(B/f'material-phase-metrics-{ST}.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
print(json.dumps([{'id':r['id'],'phase':r.get('production_inverse_stroke_phase_y'),'domains':r['domains']}for r in out],indent=2))

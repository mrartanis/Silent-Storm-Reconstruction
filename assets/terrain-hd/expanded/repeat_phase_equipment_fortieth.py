from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='equipment-fortieth';out=[]
for i in [5237,5383]:
 o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),dtype=float);sl=s.mean(2);gl=g.mean(2);d={'id':i,'qualification':'Postcall numerical paint phase/count diagnostics ONLY, same nativeRGB independentA. Literal maxima/rowprofiles are NOT physicalhardware or acceptance thresholds; actual sourcepattern/silhouette material reviewed privately. No sourceproof change/artistRGBrepair/registration/crop.','checks':[]}
 if i==5383:
  for y in [5,8,11,14,17,20,23]:
   r={'native_y':y,'native_x_interval':[17,106]}
   for n,a in [('source',sl),('production',gl)]:
    xs=[x for x in range(17,106)if a[y,x]>a[y,x-1]and a[y,x]>=a[y,x+1]];r[n]={'literal_row_maximum_count':len(xs),'native_maximum_x':xs,'successive_pitch':[b-a for a,b in zip(xs,xs[1:])]}
   d['checks'].append(r)
  for x,y in [(125,32),(92,60),(114,12)]:d['checks'].append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist(),'source_region_qualification':'Actual sourceblackUV/moving bladeoutline/rightend paint, not material-hole inference from blackRGB alone.'})
 else:
  for name,y0,y1 in [('upperdarkseparator',8,15),('middledarkseparator',23,31),('lowerdarkseparator',36,46)]:
   r={'region':name,'native_x_interval':[1,43],'native_y_interval':[y0,y1]}
   for n,a in [('source',sl),('production',gl)]:
    v=a[y0:y1,1:43].mean(1);r[n]={'native_minimum_meanRGB_row':y0+int(v.argmin()),'row_meanRGB_profile':v.tolist()}
   d['checks'].append(r)
 out.append(d)
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for d in out:print(d['id'],[(r.get('native_y',r.get('region',r.get('native_xy'))),r.get('source',r.get('source_RGB')),r.get('production',r.get('production_inverse_RGB')))for r in d['checks']])

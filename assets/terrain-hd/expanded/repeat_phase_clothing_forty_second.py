from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-forty-second';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
R=read(B/f'natural-RGB-roundtrip-{ST}.json')['sources']
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o);g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS));rnd=np.array(R[str(i)]['source_RGB_LANCZOS4x_then_native_RGB_matrix']);checks=[]
 if i==7632:
  for name,a in [('source',s),('source_RGB_natural_roundtrip',rnd),('production',g)]:
   v=a[5,:72,2].astype(float);maxima=[x for x in range(1,71)if v[x]>v[x-1]and v[x]>=v[x+1]];checks.append({'native_y':5,'diagnostic':name,'native_blue_profile_x0_to71':v.tolist(),'literal_blue_local_maximum_x':maxima,'qualification':'Withinstripe texture can have multiple maxima; do not equate these maxima to physicalstripe count. BEFORE ninegroups/pitch authoritative.'})
  for x in [91,95,100]:
   d={'native_x':x,'qualification':'Three original weakgray paintcolumns, middle phase1 versus outer phase0; originalfullnative BEFORE matrix contains all. Outerpair beforeconstraint only describes TWO outercolumns, not total stripshape count.'}
   for name,a in [('source',s),('source_RGB_natural_roundtrip',rnd),('production',g)]:
    v=a[:,x].mean(1);ys=[y for y in range(1,26)if v[y]>v[y-1]and v[y]>=v[y+1]];d[name]={'meanRGB_profile_y0_to26':v[:27].tolist(),'literal_paintshade_maximum_y':ys}
   checks.append(d)
 xy={6812:[(244,15),(245,16),(65,117),(102,101),(0,127),(95,33),(185,99)],6816:[(147,1),(134,100),(146,118),(102,101),(0,127),(95,33),(185,99)],7632:[(91,7),(4,84),(4,85),(113,117),(113,116),(114,117),(108,68),(0,127)]}[i]
 for x,y in xy:checks.append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'source_RGB_natural_roundtrip':rnd[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist(),'qualification':'Literal source paint/opaque black UV sample. No new physicalhardware interpretation or automatic pixelthreshold.'})
 out.append({'id':i,'count_phase_notes':'Entire original paintdomains and weakpigment matrix checked, sourceRGB operatorroundtrip separate A. Literal shade maxima not physicalparts counts. Source NN privateonly, originalnative opaqueRGB tool. No artistRGBpatch/crop/BBox/padoptout/retry.','checks':checks})
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in out:print(r['id'],[(d['native_xy'],d['source_RGB'],d['source_RGB_natural_roundtrip'],d['production_inverse_RGB'])for d in r['checks']if 'native_xy'in d])

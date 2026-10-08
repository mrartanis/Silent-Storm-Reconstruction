from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='heads-twenty-second';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
rnds=read(B/f'natural-roundtrip-{ST}.json')['sources']
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');src=np.array(o,dtype=float).mean(2);rnd=np.array(rnds[str(i)]['complete_native_RGB_roundtrip'],dtype=float).mean(2);prod=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),dtype=float).mean(2);checks=[]
 for p in c['native_repeat_count_phase_evidence']:
  y=p['native_y'];x0,x1=p['native_x_interval'];r={k:p[k]for k in['paintdomain','native_y','native_x_interval']}
  for name,a in[('source',src),('source_natural_RGB_roundtrip',rnd),('production',prod)]:
   xs=[x for x in range(x0,x1)if a[y,x]>a[y,x-1]and a[y,x]>=a[y,x+1]];r[name]={'literal_paintshade_maximum_count':len(xs),'native_maximum_x':xs,'native_successive_pitch':[b-a for a,b in zip(xs,xs[1:])],'native_meanRGB_profile':a[y,x0:x1].tolist()}
  checks.append(r)
 out.append({'id':i,'qualification':'Exact source versus naturalRGB4xroundtrip BEFOREmatrix versus generatedpureRGB512→128 entireinverse independentlyA. LiteralCOLOR shade maxima notphysicalhairfiber/lockcounts nor numericalthreshold. Sourcephase/blur/material reviewed at complete raw+native4x privately; no artistrepair/UVregistration/crop.','checks':checks})
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print([(d['id'],[(p['paintdomain'],p['native_y'],p['source']['literal_paintshade_maximum_count'],p['source_natural_RGB_roundtrip']['literal_paintshade_maximum_count'],p['production']['literal_paintshade_maximum_count'])for p in d['checks']])for d in out])

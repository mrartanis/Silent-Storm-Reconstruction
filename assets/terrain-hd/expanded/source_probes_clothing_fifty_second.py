from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-second'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
out=[];R=read(B/f'natural-RGB-roundtrip-{ST}.json')['sources'];D={r['id']:r for r in read(B/f'detail-metrics-{ST}.json')}
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];s=np.array(Image.open(B/f'original/{i}.png').convert('RGB'));p=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize((s.shape[1],s.shape[0]),Image.Resampling.LANCZOS));rnd=np.array(R[str(i)]['source_RGB_LANCZOS4x_then_native_RGB_matrix'],dtype=np.uint8)
 points={tuple(d['source_peak_xy']):d['name']+' native peak' for d in c['source_domains']}
 for k in ['source_peak_xy','max_local_mean_lift_xy','max_local_mean_loss_xy']:points[tuple(D[i][k])]=k
 for x,y,label in [(72,80,'interior original muted red paint'),(72,229,'old upper lower-strip paint profile'),(72,240,'old middle lower-strip paint profile'),(72,252,'old bottom lower-strip paint profile'),(145,200,'middle gray painting between lightbands'),(110,59,'old upper adjacent gray paint'),(104,103,'old lower adjacent gray paint'),(0,255,'source lowerleft corner'),(255,255,'source lowerright corner')]:points[(x,y)]=label
 out.append({'id':i,'qualification':'Exact numeric points derived from unchanged BEFORE source domains and postcall numerical extrema, plus literal native shade positions. Not physicalpart counts or acceptance thresholds; wholeRGB inverse separately sourceA.','points':[{'native_xy':[x,y],'label':name,'source_RGB':s[y,x].tolist(),'natural_roundtrip_RGB':rnd[y,x].tolist(),'production_inverse_RGB':p[y,x].tolist()}for (x,y),name in points.items()]})
(B/f'postcall-source-specific-probes-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(out,ensure_ascii=False))

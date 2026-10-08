from pathlib import Path
import json,numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
qa=[]
for i in [7455,7459,7460]:
 o=Image.open(BASE/f'original/{i}.png').convert('RGBA')
 s=np.asarray(o.convert('RGB'),float).mean(2)
 g=np.asarray(Image.open(BASE/f'private-equipment-seventeenth/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),float).mean(2)
 rows=[]
 for label,ys in [('upper',[1,2]),('lower',[11,12])]:
  sv=s[ys].mean(0);gv=g[ys].mean(0);xs=[41,46,51,56]
  sp=[min(range(x-1,x+2),key=lambda z:sv[z])for x in xs];gp=[min(range(x-1,x+2),key=lambda z:gv[z])for x in xs]
  assert sp==xs
  rows.append({'edge':label,'source_native_Y':ys,'source_X_dark_centers':sp,'candidate_X_dark_centers':gp,'exact_phase_count':gp==sp,'source_values':[float(sv[x])for x in xs],'candidate_values':[float(gv[x])for x in xs]})
 qa.append({'id':i,'original_size':[64,64],'cut_count_per_edge':4,'source_X_pitch':5,'records':rows,'strict_cut_phase_count':all(r['exact_phase_count']for r in rows),'method':'Source original64square and ENTIREcandidate native256 inverse64 read-only. Within defined source dark-column centers +/-1, use rowmean of Y1/2 and11/12 respectively. No source/candidate crop, registration, BBox, composition or artRGB patch. Pattern count/phase diagnostic only; fullsource/fullraw/material/native review also required.'})
(BASE/'pattern-metrics-equipment-seventeenth.json').write_text(json.dumps(qa,indent=2)+'\n',encoding='utf-8')
print([(r['id'],r['strict_cut_phase_count'])for r in qa])

import json,hashlib
from pathlib import Path
import numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
rows=json.loads((BASE/'pattern-constraints-equipment-sixteenth.json').read_text(encoding='utf-8'))
qa=[]
for c in rows:
 i=c['id'];o=Image.open(BASE/f'original/{i}.png').convert('RGBA');a=np.asarray(o.convert('RGB'),float).mean(2)
 g=np.asarray(Image.open(BASE/f'private-equipment-sixteenth/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),float).mean(2)
 x0,y0,x1,y1=c['bbox'];sv=a[:,x0:x1].mean(1);gv=g[:,x0:x1].mean(1)
 anchors=c['dark_divider_rows'] if i==3046 else c['periodic_bright_rows'];select=min if i==3046 else max
 sp=[select(range(y-1,y+2),key=lambda z:sv[z]) for y in anchors];gp=[select(range(y-1,y+2),key=lambda z:gv[z]) for y in anchors]
 assert sp==anchors,(i,sp,anchors)
 qa.append({'id':i,'source_RGBA_sha256':hashlib.sha256(o.tobytes()).hexdigest(),'source_constrained_rows':anchors,'candidate_constrained_rows':gp,'exact_constrained_phase_count':gp==sp,'method':'Within EACH explicitly source-defined feature row ±1, read-only row-average extrema across complete source bbox X span. Dark dividers for3046; thin bright strips for3049. This is not a global peak-count claim over irregular source caps/other surface shading. Entire original and entire calibrated native resize only; not crop/registration/compositing/artRGBpatch.','row_values':[{'source_y':y,'source':float(sv[y]),'candidate':float(gv[y]),'source_neighbors':[float(sv[y-1]),float(sv[y+1])],'candidate_neighbors':[float(gv[y-1]),float(gv[y+1])]} for y in anchors],'complete_fullsource_and_fullraw_private_review_required':True})
(BASE/'pattern-metrics-equipment-sixteenth.json').write_text(json.dumps(qa,indent=2)+'\n',encoding='utf-8')
print([(r['id'],r['exact_constrained_phase_count'])for r in qa])

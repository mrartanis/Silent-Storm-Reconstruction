import json,hashlib
from pathlib import Path
import numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
def flood(a,seed,box):
 x0,y0,x1,y1=box;seen=set();todo=[seed]
 while todo:
  x,y=todo.pop()
  if (x,y) in seen or x<x0 or x>=x1 or y<y0 or y>=y1 or a[y,x].max()>5:continue
  seen.add((x,y));todo += [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]
 return {'count':len(seen),'bbox':[min(x for x,y in seen),min(y for x,y in seen),max(x for x,y in seen)+1,max(y for x,y in seen)+1]} if seen else {'count':0}
results=[]
for i in [5330,5332,5381]:
 p=BASE/f'original/{i}.png';q=BASE/f'private-equipment-nineteenth/native4x/{i}-stored-rgb-private.png'
 src=Image.open(p).convert('RGB');s=np.array(src);g=np.array(Image.open(q).convert('RGB').resize(src.size,Image.Resampling.LANCZOS))
 row={'id':i,'source_original_png_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'private_candidate_png_sha256':hashlib.sha256(q.read_bytes()).hexdigest(),'diagnostic':'Read-only entire candidate native4x inverse to original dimensions. Connected black component threshold maxRGB<=5. ROI limits prevent following unrelated black gaps; only measurement, never reference/cropping/registration/final compositing. No exact periodic woodgrain phase claimed.'}
 if i in [5330,5332]:
  seed,box=((53,32),(37,18,69,47)) if i==5330 else ((101,64),(77,40,128,93))
  row.update(ring_seed=list(seed),ring_diagnostic_limit=list(box),original_black_core=flood(s,seed,box),candidate_black_core=flood(g,seed,box),material_review='Original irregular soft mottled ring/wood paint becomes more regular concentric rim/horizontal striations; pending, no count-only acceptance.')
 else:row['material_review']='Upper contour/oval smooths original irregular steps and interior horizontal field dark patches soften/disappear; pending despite dim source palette.'
 results.append(row)
(BASE/'pattern-metrics-equipment-nineteenth.json').write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Private diagnostics saved for3; no image edits.')

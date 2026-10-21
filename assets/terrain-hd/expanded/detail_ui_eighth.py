import json
from pathlib import Path
from PIL import Image
import numpy as np
BASE=Path(__file__).resolve().parent
def holes(image):
    a=np.asarray(image.convert('RGB'));mask=a.max(2)<5
    h,w=mask.shape;seen=np.zeros_like(mask);result=[]
    for y,x in zip(*np.where(mask)):
        if seen[y,x]:continue
        stack=[(int(x),int(y))];seen[y,x]=True;xs=[];ys=[]
        while stack:
            xx,yy=stack.pop();xs.append(xx);ys.append(yy)
            for nx,ny in [(xx-1,yy),(xx+1,yy),(xx,yy-1),(xx,yy+1)]:
                if 0<=nx<w and 0<=ny<h and mask[ny,nx] and not seen[ny,nx]:
                    seen[ny,nx]=True;stack.append((nx,ny))
        box=[min(xs),min(ys),max(xs)+1,max(ys)+1];bw=box[2]-box[0];bh=box[3]-box[1]
        if bw>=20 and bh>=10 and len(xs)/(bw*bh)>.85:result.append(box)
    return result
src=Image.open(BASE/'original/5207.png')
raw=Image.open(BASE/'generated/5207-raw.png').resize(src.size,Image.Resampling.LANCZOS)
s=holes(src);g=holes(raw)
assert len(s)==len(g)==31
matches=[]
for box in s:
    nearest=min(g,key=lambda b:sum(abs(v-u) for v,u in zip(box,b)))
    matches.append({'source_bbox':box,'inverse_raw_bbox':nearest,'max_logical_coordinate_delta':max(abs(v-u) for v,u in zip(box,nearest))})
record={'id':5207,'method':'Read-only max stored RGB <5 connected components, width>=20,height>=10,area/rectangle>.85. Entire raw Lanczos to original512x512 for diagnostic only. No art transform, crop, patch or BBox registration.',
        'source_hole_count':len(s),'inverse_raw_hole_count':len(g),'source_top_clipped_count':sum(b[1]==0 for b in s),'complete_hole_count':sum(b[1]>0 for b in s),'matches':matches,
        'interpretation':'All31 original black rectangular holes remain (4 top-clipped +27 full). Arrow directions and every source stroke privately inspected separately. Diagnostic count is not material acceptance; stronger pitting and relative bevels remain pending.'}
(BASE/'detail-proof-ui-eighth-5207.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(len(s),len(g),'maximum logical boundary delta',max(x['max_logical_coordinate_delta'] for x in matches))

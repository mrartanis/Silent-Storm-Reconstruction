from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-thirty-first';out=[]
for i in [1764,4780,5298]:
 o=Image.open(B/f'original/{i}.png').convert('RGBA');a=np.array(o);rgb=a[:,:,:3];mask=rgb.max(2)>0;seen=np.zeros(mask.shape,bool);cs=[]
 for y,x in zip(*np.where(mask)):
  if seen[y,x]:continue
  seen[y,x]=1;stack=[(int(y),int(x))];pts=[]
  while stack:
   yy,xx=stack.pop();pts.append((yy,xx))
   for dy in [-1,0,1]:
    for dx in [-1,0,1]:
     ny,nx=yy+dy,xx+dx
     if 0<=ny<o.height and 0<=nx<o.width and mask[ny,nx] and not seen[ny,nx]:seen[ny,nx]=1;stack.append((ny,nx))
  yy,xx=zip(*pts);cs.append({'pixel_count':len(pts),'native_bbox':[min(xx),min(yy),max(xx)+1,max(yy)+1],'max_RGB':rgb[list(yy),list(xx)].max(0).tolist()})
 l=rgb.mean(2);y,x=np.unravel_index(l.argmax(),l.shape)
 out.append({'id':i,'before_tool_prompt':True,'native_positive_RGB_8connected_domains':cs,'native_peak_xy':[int(x),int(y)],'native_peak_RGBA':a[y,x].tolist(),'actual_RGBA_extrema':[list(z)for z in o.getextrema()],'semantic_qualification':'Literal bitmap-connected regions only, not physical particles/paintparts inferred from RGB/A. Full exact native matrix once in common native-matrices file. All weak/zero bits are authoritative original, no threshold helper.'})
 o.getchannel('A').resize((512,512),Image.Resampling.NEAREST).save(B/f'private-{ST}/survey/{i}-alpha-nearest.png')
(B/f'source-count-survey-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(out,ensure_ascii=False,indent=2))

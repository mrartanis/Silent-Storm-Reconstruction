from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-twenty-seventh';CALL={4096,4097};out=[]
for r in json.loads((B/f'source-check-{ST}-before-call.json').read_text(encoding='utf-8')):
 if r['id']in CALL:continue
 i=r['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');a=np.array(o);rgb=a[:,:,:3];l=rgb.mean(2);mask=rgb.max(2)>0;seen=np.zeros(mask.shape,bool);cs=[]
 for y,x in zip(*np.where(mask)):
  if seen[y,x]:continue
  seen[y,x]=1;stack=[(int(y),int(x))];pts=[]
  while stack:
   yy,xx=stack.pop();pts.append((yy,xx))
   for dy in [-1,0,1]:
    for dx in [-1,0,1]:
     ny,nx=yy+dy,xx+dx
     if 0<=ny<o.height and 0<=nx<o.width and mask[ny,nx] and not seen[ny,nx]:seen[ny,nx]=1;stack.append((ny,nx))
  yy,xx=zip(*pts);cs.append({'pixel_count':len(pts),'native_bbox':[min(xx),min(yy),max(xx)+1,max(yy)+1],'source_max_RGB':rgb[list(yy),list(xx)].max(0).tolist()})
 y,x=np.unravel_index(l.argmax(),l.shape)
 out.append({'id':i,'source_RGBA_sha256':r['source_rgba_sha256'],'native_size':list(o.size),'source_RGBA_extrema':[list(z)for z in o.getextrema()],'whole_native_RGBA_matrix':a.tolist(),'source_max_mean_RGBA_xy':[int(x),int(y)],'source_max_mean_RGBA':a[y,x].tolist(),'center_row_mean_RGB':l[o.height//2].tolist(),'center_column_mean_RGB':l[:,o.width//2].tolist(),'literal_positive_RGB_8connected_pixel_domain_count':len(cs),'pixel_domains':cs,'qualification':'Actual pixel-connected components are NOT physicalparticles/paintparts fromRGBA threshold. Complete nativeRGBA includes every weakdim bit. Source-only fields remain genuinepending, no analytical/technical blanket classification from simple appearance or filename. Only numeric diagnostics, never sourcecrop/artistpatch/helperthreshold.','reason':r['source_only_reason'],'imagegen_call_count':0,'genuine_pending':True,'not_accepted':True})
(B/f'source-only-detail-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Source-only complete native10 pixel matrices')

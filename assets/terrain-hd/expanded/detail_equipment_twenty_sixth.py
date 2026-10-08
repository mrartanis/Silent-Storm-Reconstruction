import json
from pathlib import Path
import numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
IDS=[2448,3051,3041]
out=[]
def components(mask):
 visited=np.zeros(mask.shape,dtype=bool);records=[]
 for y,x in zip(*np.where(mask)):
  if visited[y,x]:continue
  todo=[(int(y),int(x))];visited[y,x]=True;coords=[]
  while todo:
   py,px=todo.pop();coords.append((py,px))
   for dy in (-1,0,1):
    for dx in (-1,0,1):
     yy,xx=py+dy,px+dx
     if 0<=yy<mask.shape[0] and 0<=xx<mask.shape[1] and mask[yy,xx] and not visited[yy,xx]:visited[yy,xx]=True;todo.append((yy,xx))
  ys,xs=zip(*coords);records.append({'pixel_count':len(coords),'source_pixel_bbox':[min(xs),min(ys),max(xs)+1,max(ys)+1]})
 return sorted(records,key=lambda r:r['pixel_count'],reverse=True)
for id_ in IDS:
 original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
 s=np.asarray(original.convert('RGB'),dtype=np.int16)
 g=np.asarray(Image.open(BASE/f'private-equipment-twenty-sixth/native4x/{id_}-stored-rgb-private.png').resize(original.size,Image.Resampling.LANCZOS).convert('RGB'),dtype=np.int16)
 sl=s.max(2);gl=g.max(2);delta=gl-sl;ys,xs=np.where((sl>0)&(delta>12));idx=sorted(range(len(xs)),key=lambda i:int(delta[ys[i],xs[i]]),reverse=True)[:10]
 examples=[{'logical_xy':[int(xs[i]),int(ys[i])],'source_rgb':s[ys[i],xs[i]].tolist(),'candidate_rgb':g[ys[i],xs[i]].tolist(),'max_channel_increase':int(delta[ys[i],xs[i]])} for i in idx]
 a=np.asarray(original.getchannel('A'))
 native=Image.open(BASE/f'private-equipment-twenty-sixth/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
 assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
 row={'id':id_,'diagnostic':'Entire calibrated native storedRGB inverse LANCZOS to original native source dimensions, positive-source pixels only. Local increases/connected-component bboxes diagnose source material/count only; not proof of newly invented parts, never used for registration or compositing.', 'stored_native_luma_spatial_correlation':float(np.corrcoef(s.mean(2).ravel(),g.mean(2).ravel())[0,1]),'max_positive_source_local_channel_increase':int(delta[sl>0].max()),'weak_source_pixels_max_lt60_gain_gt12':int(((sl>0)&(sl<60)&(delta>12)).sum()),'examples':examples,'source_native_alpha_components_8connected_positiveA':components(a>0),'source_material_components_8connected_RGBmax_gt5':components(sl>5),'native_alpha4x_byte_exact':True}
 out.append(row);print(id_,round(row['stored_native_luma_spatial_correlation'],4),'sourceA-components',len(row['source_native_alpha_components_8connected_positiveA']))
(BASE/'detail-metrics-equipment-twenty-sixth.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

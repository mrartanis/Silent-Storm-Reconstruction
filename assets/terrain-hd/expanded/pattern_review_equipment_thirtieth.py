from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageFilter

BASE=Path(__file__).resolve().parent
STEM='equipment-thirtieth'
rows=[]
domains={676:[[10,30,70,55],[94,10,127,50]],2968:[[40,20,55,115],[89,70,106,80]],4257:[[4,2,90,62],[96,20,120,58]]}
for i in [676,2968,4257]:
 im=Image.open(BASE/f'original/{i}.png').convert('RGB')
 candidate=Image.open(BASE/f'private-{STEM}/native4x/{i}-stored-rgb-private.png').resize(im.size,Image.Resampling.LANCZOS).convert('RGB')
 s=np.asarray(im,dtype=float);g=np.asarray(candidate,dtype=float)
 sl=s.mean(2);gl=g.mean(2)
 sh=sl-np.asarray(im.filter(ImageFilter.GaussianBlur(2))).mean(2)
 gh=gl-np.asarray(candidate.filter(ImageFilter.GaussianBlur(2))).mean(2)
 row={'id':i,'diagnostic':'POSTCALL native inverse whole scalar source/candidate material diagnostic. Domains are fixed numerical checks only; no artistic RGB insertion, fitting or crop. Before-call source constraints immutable.','grain':[]}
 for box in domains[i]:
  x0,y0,x1,y1=box;ss=sl[y0:y1,x0:x1];gg=gl[y0:y1,x0:x1]
  row['grain'].append({'native_domain':box,'source_luma_mean':float(ss.mean()),'candidate_luma_mean':float(gg.mean()),'source_luma_std':float(ss.std()),'candidate_luma_std':float(gg.std()),'source_highpass_std':float(sh[y0:y1,x0:x1].std()),'candidate_highpass_std':float(gh[y0:y1,x0:x1].std()),'highpass_correlation':float(np.corrcoef(sh[y0:y1,x0:x1].ravel(),gh[y0:y1,x0:x1].ravel())[0,1])})
 if i==676:
  profiles=[]
  for name,a in [('source',s),('candidate',g)]:
   p=a[20:46,92:128].mean((0,2));xs=[x+92 for x in range(1,len(p)-1)if min(p[x-1],p[x+1])-p[x]>8]
   assert xs==[93,104,114,124]
   profiles.append({'name':name,'profile':p.tolist(),'native_dark_seam_x':xs,'count':4})
  row['four_native_seams_exact_phase']=profiles
 elif i==2968:
  fields=[]
  for box in [[88,14,98,38],[100,14,112,38]]:
   x0,y0,x1,y1=box;f={'domain':box}
   for name,a in [('source',s),('candidate',g)]:
    p=a[y0:y1,x0:x1].mean(2).max(1);ys=[j+y0 for j in range(1,len(p)-1)if p[j]>p[j-1]and p[j]>=p[j+1]and p[j]>110]
    assert ys==[24,27]
    f[name]={'count':2,'native_light_bar_y':ys,'profile':p.tolist()}
   fields.append(f)
  row['two_strap_fields_exact_two_light_bar_phase']=fields
  masks=[]
  for name,a in [('source',s),('candidate',g)]:
   c=a[48:68,92:104];m=(c[:,:,0]>c[:,:,1]*1.5)&(c[:,:,0]>c[:,:,2]*1.5)&(c[:,:,0]>55)
   yy,xx=np.where(m);box=[int(xx.min()+92),int(yy.min()+48),int(xx.max()+93),int(yy.max()+49)]
   assert box==[94,49,102,67]
   coords=[[int(x+92),int(y+48)]for y,x in zip(yy,xx)]
   row[f'{name}_red_cross']={'diagnostic_red_pixel_count':int(m.sum()),'native_bbox':box,'coordinates':coords}
   masks.append(m)
  row['cross_core_masks_IoU']=float((masks[0]&masks[1]).sum()/(masks[0]|masks[1]).sum())
  row['cross_caveat']='Exact source red threshold60 native pixels becomes64 after whole4x/subsequent inverse scalar. Same whole cross bbox/four arms/one component; four one-native-pixel left stem-edge threshold texels at(96,53),(96,54),(96,55),(96,63) alter support; original60 red core pixels all retained, no new symbol or enlarged arm. No sourceRGB mask/mark fix.'
 else:
  sm=(s[:,:,0]-s[:,:,1]>24)&(s[:,:,1]-s[:,:,2]>12)
  gm=(g[:,:,0]-g[:,:,1]>24)&(g[:,:,1]-g[:,:,2]>12)
  row['native_brown_threshold']=dict(criterion='R-G>24,G-B>12',source_pixel_count=int(sm.sum()),candidate_pixel_count=int(gm.sum()),mask_IoU=float((sm&gm).sum()/(sm|gm).sum()))
  row['threshold_semantics']='Colour-threshold masks measure source pigment phase/strength; they are not physical camouflage fragment count and no threshold equality implied after subtle hue change. Full original pigment patches and broad/tiny source-scale artwork privately inspected, no new spots accepted.'
 rows.append(row)
(BASE/f'material-pattern-metrics-{STEM}.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
(BASE/f'tiny-marks-review-{STEM}.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
print('676 four seam phases, 2968 two bars/field and cross native bbox asserted; material/source pattern metrics saved')

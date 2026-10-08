from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageFilter
BASE=Path(__file__).resolve().parent
STEM='equipment-thirty-second'
rows=[]
domains={5378:[[4,5,28,35],[37,18,56,36],[37,41,60,59]],5389:[[2,3,22,24],[30,15,43,30],[51,19,62,30]],5404:[[10,28,24,51],[10,8,24,22]]}
for i in [5378,5389,5404]:
 original=Image.open(BASE/f'original/{i}.png').convert('RGBA')
 im=original.convert('RGB');candidate=Image.open(BASE/f'private-{STEM}/native4x/{i}-stored-rgb-private.png').resize(im.size,Image.Resampling.LANCZOS).convert('RGB')
 s=np.asarray(im,dtype=float);g=np.asarray(candidate,dtype=float)
 sl=s.mean(2);gl=g.mean(2);sh=sl-np.asarray(im.filter(ImageFilter.GaussianBlur(2))).mean(2);gh=gl-np.asarray(candidate.filter(ImageFilter.GaussianBlur(2))).mean(2)
 r={'id':i,'timing':'Postcall readonly numerical checks on ENTIRE native inverse scalar preview. Fixed domains numerical only, not ROI edit/crop/BBox/source art insertion. Original counts/phase/alpha proof frozen before prompts/calls.','material_domains':[]}
 for box in domains[i]:
  x0,y0,x1,y1=box;ss=sl[y0:y1,x0:x1];gg=gl[y0:y1,x0:x1]
  r['material_domains'].append({'native_diagnostic_domain':box,'source_luma_mean':float(ss.mean()),'candidate_luma_mean':float(gg.mean()),'source_luma_std':float(ss.std()),'candidate_luma_std':float(gg.std()),'source_highpass_std':float(sh[y0:y1,x0:x1].std()),'candidate_highpass_std':float(gh[y0:y1,x0:x1].std()),'highpass_correlation':float(np.corrcoef(sh[y0:y1,x0:x1].ravel(),gh[y0:y1,x0:x1].ravel())[0,1])})
 if i==5378:
  coords=[]
  for name,a in [('source',s),('candidate',g)]:
   m=a[40:64,35:63].mean(2)<30;yy,xx=np.where(m);rr=[[int(x+35),int(y+40)]for y,x in zip(yy,xx)]
   r[f'{name}_lower_brown_dark_mark']={'criterion':'RGBmean<30','count':len(rr),'native_coords':rr};coords.append(set(map(tuple,rr)))
  r['dark_tonal_mask_IoU']=float(len(coords[0]&coords[1])/len(coords[0]|coords[1]))
  r['meaning']='Source marks are irregular faint paint, no lettering/physical part interpretation. Threshold count changes are diagnostic, not sole decision; material of gray wash/red pigment independently reviewed.'
 elif i==5389:
  rr=[]
  for name,a in [('source',s),('candidate',g)]:
   p=a[:12,29:63].mean((1,2));ys=[j for j in range(1,len(p)-1)if min(p[j]-p[j-1],p[j]-p[j+1])>5];assert ys==[1,4]
   rr.append({'name':name,'count':2,'native_warm_band_y':ys,'profile':p.tolist()})
  r['exact_two_upper_warm_bands']=rr
 else:
  rr=[]
  for name,a in [('source',s),('candidate',g)]:
   p=a[6:26,10:25].mean((1,2));ys=[j+6 for j in range(1,len(p)-1)if p[j]>p[j-1]and p[j]>=p[j+1]]
   rr.append({'name':name,'count':len(ys),'native_soft_maxima_y':ys,'profile':p.tolist()})
  assert rr[0]['native_soft_maxima_y']==[10,12,14,16,18,20,22]
  assert rr[1]['native_soft_maxima_y']==[12,14,16,18,20,22]
  r['original_soft_tonal_phase']=rr
  a=np.asarray(original,dtype=int);assert (a[57,17:20]==[0,0,0,0]).all()
  r['existing_bottom_dark_feature']={'native_source_RGBA_row57_x17_18_19':a[57,17:20].tolist(),'source_fact':'All three native core pixels are ORIGINAL RGB0/A0; original alpha mask already carries this bitmap opening. No physical slot/tool function or dark pigment inferred. Root fullsourceA4x is unchanged. Inverse storedRGB may sample padded neighboring color under originalA0; invisible-alpha support is not an RGB art patch.','candidate_inverse_RGB_row57_x17_18_19':g[57,17:20].tolist()}
  r['caveat']='Source weak first local tone maximumy10 (luma99 versus neighboring97) smooths into monotone top transition after inverse. Other6 source maxima12/14/16/18/20/22 retain exact phase. These soft tonal ripples are not physical thread/rib counts; a weak threshold/1px smoothing change alone is not a material/geometry hold. Full source wash privately inspected, no new raised lines accepted.'
 rows.append(r)
for name in ['material-pattern-metrics','tiny-marks-review']:(BASE/f'{name}-{STEM}.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
print('5389exact2 bands;5404remaining6 native phase + originalA0 bottom feature;5378material/stain diagnostics saved')

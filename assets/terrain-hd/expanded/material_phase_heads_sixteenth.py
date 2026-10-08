from pathlib import Path
import json,numpy as np
from PIL import Image,ImageFilter
B=Path(__file__).resolve().parent;ST='heads-sixteenth';OUT=B/f'private-{ST}/native4x'
cons=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));rows=[]
for c in cons:
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');n=Image.open(OUT/f'{i}-calibrated-native-alpha-private.png').convert('RGBA');s=np.asarray(o);g=np.asarray(n.convert('RGB').resize(o.size,Image.Resampling.LANCZOS));d=[]
 assert n.getchannel('A').tobytes()==o.getchannel('A').resize(n.size,Image.Resampling.LANCZOS).tobytes()
 for name,r in c['region_diagnostics'].items():
  x0,y0,x1,y1=r['native_bbox'];a=s[y0:y1,x0:x1];p=g[y0:y1,x0:x1];y,x=np.unravel_index(p.mean(2).argmax(),p.shape[:2]);d.append({'name':name,'native_bbox':r['native_bbox'],'source_mean_RGB':a[:,:,:3].mean((0,1)).tolist(),'production_mean_RGB':p.mean((0,1)).tolist(),'source_std_RGB':a[:,:,:3].std((0,1)).tolist(),'production_std_RGB':p.std((0,1)).tolist(),'source_max_RGB':a[:,:,:3].max((0,1)).tolist(),'production_max_RGB':p.max((0,1)).tolist(),'source_peak_mean_xy':r['max_mean_source_xy'],'production_peak_mean_xy':[x0+int(x),y0+int(y)],'source_alpha_extrema':r['sourceA_extrema']})
 src=o.convert('RGB').resize(n.size,Image.Resampling.LANCZOS).convert('RGBA');src.putalpha(o.getchannel('A').resize(n.size,Image.Resampling.LANCZOS));bg=Image.new('RGBA',n.size,(128,128,128,255))
 Image.alpha_composite(bg,src).convert('RGB').save(OUT/f'{i}-source-alpha-over-gray-private.png');Image.alpha_composite(bg,n).convert('RGB').save(OUT/f'{i}-production-alpha-over-gray-private.png')
 sr=np.asarray(src.convert('RGB'),dtype=float);pr=np.asarray(n.convert('RGB'),dtype=float)
 sh=sr-np.asarray(src.convert('RGB').filter(ImageFilter.GaussianBlur(1)),dtype=float);ph=pr-np.asarray(n.convert('RGB').filter(ImageFilter.GaussianBlur(1)),dtype=float)
 rows.append({'id':i,'diagnostic':'ENTIRE pure scalar/fulloriginalA512 source and candidate; whole LANCZOS128 inverse for numeric source regions, NEVER used to repair/composite artwork. Alpha-over-gray ONLY fullcanvas private visibility diagnostic, no CPU/GPU/runtime test or actualFaceGen weight claim.','regions':d,'full4x_highpass_Gaussian_radius1_source_std_RGB':sh.std((0,1)).tolist(),'full4x_highpass_Gaussian_radius1_production_std_RGB':ph.std((0,1)).tolist(),'fullA4x_byteexact':True,'nativeType':'Ordinary','source_brow_count':0 if i==6698 else 2,'production_brow_count_private_wholeview':0 if i==6698 else 2,'source_alpha_over_gray_private':f'assets/terrain-hd/expanded/private-{ST}/native4x/{i}-source-alpha-over-gray-private.png','production_alpha_over_gray_private':f'assets/terrain-hd/expanded/private-{ST}/native4x/{i}-production-alpha-over-gray-private.png'})
 print(i,[(r['name'],[round(v,2)for v in r['source_mean_RGB']],[round(v,2)for v in r['production_mean_RGB']],r['source_max_RGB'],r['production_max_RGB'])for r in d if any(t in r['name']for t in ['brow','plain','corner','oval'])])
(B/f'material-phase-{ST}.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

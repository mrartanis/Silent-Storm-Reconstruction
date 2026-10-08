from pathlib import Path
import json,numpy as np
from PIL import Image,ImageFilter
B=Path(__file__).resolve().parent;ST='equipment-thirty-seventh';OUT=B/f'private-{ST}/native4x'
cons=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));rows=[]
for c in cons:
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');n=Image.open(OUT/f'{i}-calibrated-native-alpha-private.png').convert('RGBA');s=np.asarray(o.convert('RGB'));g=np.asarray(n.convert('RGB').resize(o.size,Image.Resampling.LANCZOS));d=[]
 assert n.getchannel('A').tobytes()==o.getchannel('A').resize(n.size,Image.Resampling.LANCZOS).tobytes()
 for z in c['source_domains']:
  x0,y0,x1,y1=z['bbox'];a=s[y0:y1,x0:x1];p=g[y0:y1,x0:x1];d.append({'name':z['name'],'native_bbox':z['bbox'],'source_mean_RGB':a.mean((0,1)).tolist(),'production_mean_RGB':p.mean((0,1)).tolist(),'source_std_RGB':a.std((0,1)).tolist(),'production_std_RGB':p.std((0,1)).tolist(),'source_max_RGB':a.max((0,1)).tolist(),'production_max_RGB':p.max((0,1)).tolist()})
 if i==5238:
  v=g.mean(2)[15:24,0:26].mean(1);p=[y+15 for y in range(1,len(v)-1)if v[y]>v[y-1]and v[y]>v[y+1]]
  phase={'source_upper_parallel_native_y_phase':[16,18,20,22],'production_upper_parallel_native_y_phase':p,'source_upper_dimline_y':7,'source_original_tiny_point_RGB':s[58,20:22].tolist(),'production_original_tiny_point_RGB':g[58,20:22].tolist(),'production_tiny_point_neighborhood_RGB':g[56:61,18:24].tolist()};assert p==[16,18,20,22]
 else:
  phase={'source_four_interior_raycrest_x':[2,4,7,9],'native_y':[42,43],'production_four_interior_raycrest_x':[],'outerclipboundary_x':0,'qualification':'Ray painttone counts, not inferred number of physical wireturns.'}
  for y in [42,43]:
   v=g.mean(2)[y,0:11];p=[x for x in range(1,len(v)-1)if v[x]>v[x-1]and v[x]>v[x+1]];phase['production_four_interior_raycrest_x'].append({'y':y,'x':p,'mean_RGB_profile':v.tolist()});assert p==[2,4,7,9]
 src=o.convert('RGB').resize(n.size,Image.Resampling.LANCZOS).convert('RGBA');src.putalpha(o.getchannel('A').resize(n.size,Image.Resampling.LANCZOS));bg=Image.new('RGBA',n.size,(128,128,128,255))
 Image.alpha_composite(bg,src).convert('RGB').save(OUT/f'{i}-source-alpha-over-gray-private.png');Image.alpha_composite(bg,n).convert('RGB').save(OUT/f'{i}-production-alpha-over-gray-private.png')
 sr=np.asarray(src.convert('RGB'),dtype=float);pr=np.asarray(n.convert('RGB'),dtype=float);sh=sr-np.asarray(src.convert('RGB').filter(ImageFilter.GaussianBlur(1)),dtype=float);ph=pr-np.asarray(n.convert('RGB').filter(ImageFilter.GaussianBlur(1)),dtype=float)
 rows.append({'id':i,'diagnostic':'ENTIRE private pure scalar/fulloriginalA256, storedRGB FIRST then ENTIRE RGB LANCZOS64 inverse; no alpha-aware RGB inverse artifacts. Whole gray128alpha composite only private visibility diagnostic, not actualCPU/GPU/nativevalidation. No crop/registration/composite artwork repair.','regions':d,'native_pattern_phase':phase,'full4x_highpass_Gaussian_radius1_source_std_RGB':sh.std((0,1)).tolist(),'full4x_highpass_Gaussian_radius1_production_std_RGB':ph.std((0,1)).tolist(),'fullA4x_byteexact':True})
 print(i,phase)
(B/f'material-phase-{ST}.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

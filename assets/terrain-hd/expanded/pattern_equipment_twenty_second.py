import json,numpy as np
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
rows=[]
for i in [1746,2447]:
 o=Image.open(BASE/f'original/{i}.png').convert('RGBA');n=Image.open(BASE/f'private-equipment-twenty-second/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA');g=np.array(Image.open(BASE/f'private-equipment-twenty-second/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS));s=np.array(o.convert('RGB'))
 row={'id':i,'periodic_pattern':'None claimed. Original irregular painted fields/cuts are reference; no invented count or phase assertion for smooth cloth.','sourceA4x_byte_exact':n.getchannel('A').tobytes()==o.getchannel('A').resize(n.size,Image.Resampling.LANCZOS).tobytes(),'diagnostics':'After-call read-only whole native inverse profile/peak inspection. Local ROIs solely measurement, never toolreference crop, fitting/composition or source artistic RGB insert.'}
 if i==1746:
  marks=[]
  for x0,y0,x1,y1 in [(0,29,14,42),(0,43,16,63)]:
   m={'diagnostic_domain':[x0,y0,x1,y1]}
   for key,a in [('source',s),('candidate',g)]:
    t=a[y0:y1,x0:x1];l=t.mean(2);k=np.unravel_index(l.argmax(),l.shape);m[key]={'peak_native_xy':[int(k[1]+x0),int(k[0]+y0)],'RGB':t[k].tolist()}
   m['source_peak_position_preserved']=m['source']['peak_native_xy']==m['candidate']['peak_native_xy'];marks.append(m)
  row['source_faint_paintmark_peak_diagnostics']=marks
 else:row['review_note']='Original gray rounded field/curved narrowlightedge/separate right strip and lowerleft sourcepaint remain major fullUV positions; mildly stronger narrow rim is finalnative-review concern. No added stitching/buttons/marks.'
 rows.append(row)
(BASE/'pattern-metrics-equipment-twenty-second.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Private2source material/mark diagnostics saved.')

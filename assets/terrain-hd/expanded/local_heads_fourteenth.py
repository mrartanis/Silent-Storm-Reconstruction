from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='heads-fourteenth'
C=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));out=[]
for c in C:
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');n=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');a=np.asarray(o,dtype=float);g=np.asarray(n.resize(o.size,Image.Resampling.LANCZOS),dtype=float);regions={}
 bs={k:r['native_bbox']for k,r in c['region_diagnostics'].items()}
 if i==1126:bs['POSTCALL_right_existing_weak_warm_pigment_unclassified_before']=[37,32,41,37]
 for name,b in bs.items():
  x0,y0,x1,y1=b;s=a[y0:y1,x0:x1];v=g[y0:y1,x0:x1];sy,sx=np.unravel_index(s.mean(2).argmax(),s.shape[:2]);gy,gx=np.unravel_index(v.mean(2).argmax(),v.shape[:2]);regions[name]={'native_bbox':b,'source_mean_RGB':s.mean((0,1)).tolist(),'inverse_mean_RGB':v.mean((0,1)).tolist(),'source_peak_xy':[int(x0+sx),int(y0+sy)],'source_peak_RGB':s[sy,sx].tolist(),'inverse_peak_xy':[int(x0+gx),int(y0+gy)],'inverse_peak_RGB':v[gy,gx].tolist(),'source_exact_matrix':s.tolist(),'inverse_exact_matrix':v.tolist()}
 size=n.size;source4=np.asarray(o.resize(size,Image.Resampling.LANCZOS),dtype=float).mean(2);native=np.asarray(n,dtype=float).mean(2)
 def hp(v):return v[1:-1,1:-1]-(v[1:-1,:-2]+v[1:-1,2:]+v[:-2,1:-1]+v[2:,1:-1])/4
 hs=hp(source4);hg=hp(native)
 out.append({'id':i,'mode':'POSTCALL numeric diagnostics only, no RGB changes/regions registration/crops. Source-before proof remains immutable. Source per-native-pixel phase saved before prompt, physical newparts evaluated privately on full views, no universal numerical threshold.','whole_native4x_highpass_std_source':float(hs.std()),'whole_native4x_highpass_std_generated':float(hg.std()),'whole_native4x_highpass_correlation':float(np.corrcoef(hs.ravel(),hg.ravel())[0,1]),'regions':regions})
(B/f'local-metrics-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print([(r['id'],round(r['whole_native4x_highpass_std_source'],3),round(r['whole_native4x_highpass_std_generated'],3))for r in out])

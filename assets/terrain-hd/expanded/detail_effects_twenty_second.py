from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-twenty-second'
cons=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));out=[]
for c in cons:
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');s=np.asarray(o)[:,:,:3].astype(float)
 p=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');g=np.asarray(p.resize(o.size,Image.Resampling.LANCZOS)).astype(float)
 diff=g-s;l=s.mean(2);gg=g.mean(2);delta=gg-l;y,x=np.unravel_index(np.argmax(np.abs(delta)),delta.shape)
 domains=[]
 for name,r in c['regions'].items():
  x0,y0,x1,y1=r['native_bbox'];v=s[y0:y1,x0:x1];z=g[y0:y1,x0:x1]
  domains.append({'name':name,'bbox':r['native_bbox'],'source_mean_RGB':v.mean((0,1)).tolist(),'production_inverse_mean_RGB':z.mean((0,1)).tolist(),'source_peak_RGB':v.max((0,1)).tolist(),'production_inverse_peak_RGB':z.max((0,1)).tolist()})
 peaks=[]
 ys,xs=np.where(l==l.max())
 for py,px in zip(ys,xs):peaks.append({'native_xy':[int(px),int(py)],'source_RGB':s[py,px].astype(int).tolist(),'production_inverse_RGB':g[py,px].astype(int).tolist()})
 alpha=o.getchannel('A').resize(p.size,Image.Resampling.LANCZOS);actual=Image.open(B/f'private-{ST}/native4x/{i}-calibrated-native-alpha-private.png').getchannel('A')
 assert alpha.tobytes()==actual.tobytes()
 r={'id':i,'diagnostic_only':'Postcall entire production inverse-original-scale diagnostics, not thresholds/acceptance criteria. Full raw and native material review authoritative. No RGB patch or registration.','inverse_original_size':list(o.size),'whole_inverse_RGB_correlation':float(np.corrcoef(s.ravel(),g.ravel())[0,1]),'source_peak_native_samples':peaks,'maximum_local_meanRGB_difference':{'native_xy':[int(x),int(y)],'signed_delta':float(delta[y,x]),'source_RGB':s[y,x].astype(int).tolist(),'production_inverse_RGB':g[y,x].astype(int).tolist()},'source_domains':domains,'source_alpha_4x_byteexact':True,'native_target_POT':all(d&(d-1)==0 for d in p.size),'source_mean_RGB':s.mean((0,1)).tolist(),'production_inverse_mean_RGB':g.mean((0,1)).tolist(),'source_and_production_inverse_RGBA_matrix_file':f'assets/terrain-hd/expanded/private-{ST}/native4x/{i}-production-inverse-original.png'}
 p.resize(o.size,Image.Resampling.LANCZOS).save(B/f'private-{ST}/native4x/{i}-production-inverse-original.png');out.append(r)
(B/f'detail-metrics-{ST}.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
print(json.dumps([{k:r[k]for k in ['id','whole_inverse_RGB_correlation','source_peak_native_samples','maximum_local_meanRGB_difference','source_domains']}for r in out],indent=2))


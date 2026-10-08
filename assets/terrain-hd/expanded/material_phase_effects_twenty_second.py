from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-twenty-second';out=[]
for i in [1078,4739,4744]:
 o=Image.open(B/f'original/{i}.png').convert('RGBA');a=np.asarray(o)[:,:,:3];p=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');s=np.asarray(o.convert('RGB').resize(p.size,Image.Resampling.LANCZOS),dtype=float);g=np.asarray(p,dtype=float)
 def hp(v):return v[1:-1,1:-1]-(v[:-2,1:-1]+v[2:,1:-1]+v[1:-1,:-2]+v[1:-1,2:])/4
 hs=hp(s.mean(2));hg=hp(g.mean(2));inverse=np.asarray(p.resize(o.size,Image.Resampling.LANCZOS));srcmask=s.max(2)>0;zero=~srcmask
 source_l=a.mean(2);inverse_l=inverse.mean(2);py,px=np.unravel_index(source_l.argmax(),source_l.shape)
 out.append({'id':i,'mode':'Readonly postcall fullsource/raw/native4x material/shade phase diagnostics, no universal cutoff and no image patches. Full private source and raw visual review decides whether soft paint became coarse cells.','native4x_highpass_std_source':float(hs.std()),'native4x_highpass_std_candidate':float(hg.std()),'native4x_highpass_correlation':float(np.corrcoef(hs.ravel(),hg.ravel())[0,1]),'source_peak_native_xy':[int(px),int(py)],'entire_original_source_RGBA_matrix':np.asarray(o).tolist(),'entire_candidate_inverse_native_RGB_matrix':inverse.tolist(),'entire_row_luminance_source':source_l.tolist(),'entire_row_luminance_inverse':inverse_l.tolist(),'target_zero_source_RGB_pixel_count':int(zero.sum()),'candidate_RGB_at_zero_source_max':g[zero].max(0).tolist()if zero.any()else None,'candidate_positive_at_zero_source_count':int((g[zero].max(1)>0).sum())if zero.any()else 0,'zero_source_qualification':'Source whole4x RGB LANCZOS positive-mask numeric diagnostic only. Extra low-code color at originally0 locations is NOT automatically new physical particles or an acceptance threshold. No alpha0 exclusion forAdd/source originalA unchanged.'})
(B/f'material-phase-metrics-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print([{k:r[k]for k in ['id','native4x_highpass_std_source','native4x_highpass_std_candidate','candidate_RGB_at_zero_source_max','candidate_positive_at_zero_source_count']}for r in out])

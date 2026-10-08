import json,numpy as np,hashlib
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
expected=[1,3,8,12,16,18,21,24,28,32,34,37,40,44]
def peaks(a):
 p=a[:51,:13].mean(1);return [j for j,v in enumerate(p)if 0<j<len(p)-1 and v>p[j-1]and v>=p[j+1]]
src=Image.open(BASE/'original/1822.png').convert('RGB');a=np.array(src,dtype=float).mean(2);assert peaks(a)==expected
rows=[{'id':1822,'native_readonly_profile_domain':[0,0,13,51],'source_painted_row_profile_maxima_y':expected,'source_profile_peak_count':14,'source_pattern_interpretation':'Genuinely asserted before prompt/call. These14local tonal mean-profile maxima are exact measured paint positions, NOT14physical ribs or ideal periodic repeat. Original irregular shaded strip remains flat painted material, never regularize to an invented grid/count.','diagnostic_only':'Source fixed-domain mean profiles, no toolreference crop/BBox/composite/UV fitting.'}]
p=BASE/'private-equipment-twenty-fourth/native4x/1822-stored-rgb-private.png'
if p.exists():
 g=np.array(Image.open(p).convert('RGB').resize(src.size,Image.Resampling.LANCZOS),dtype=float).mean(2);rows[0]['candidate_row_profile_maxima_y']=peaks(g);rows[0]['exact_tonal_profile_phase_equal']=peaks(g)==expected
s=Image.open(BASE/'original/5432.png').convert('RGB');a=np.array(s);t=a[:16,118:128];k=np.unravel_index(t.mean(2).argmax(),t.shape[:2]);assert[int(k[1]+118),int(k[0])]==[123,6]
row={'id':5432,'periodic_pattern':'No ideal periodic count/phase claimed; whole smooth painted fields reference.','source_upperright_peak_domain':[118,0,128,16],'source_upperright_peak_xy':[123,6],'source_upperright_peak_RGB':t[k].tolist(),'source_assertion_before_prompt':'Sourcebrightpeak position asserted, no hidden semantic inference. Exactlytwo original right dark/light painted marks are preserved as paint; original blackRGB is shadowcolor, not automatically a physicalhole.','input_whole_affine':'Original128x32→opaqueRGB fullnearest1536x5123:1 sourceguide; inverseENTIREraw→original4x512x128, normalized UV no crop/padding.'}
p=BASE/'private-equipment-twenty-fourth/native4x/5432-stored-rgb-private.png'
if p.exists():
 g=np.array(Image.open(p).convert('RGB').resize(s.size,Image.Resampling.LANCZOS));q=g[:16,118:128];k=np.unravel_index(q.mean(2).argmax(),q.shape[:2]);row['candidate_upperright_peak_xy']=[int(k[1]+118),int(k[0])];row['candidate_upperright_peak_RGB']=q[k].tolist()
rows.append(row)
(BASE/'pattern-metrics-equipment-twenty-fourth.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('BEFOREprompt sourceasserts passed:1822actual14 tonalmaxima,5432peak123,6. No invented idealribcount.')

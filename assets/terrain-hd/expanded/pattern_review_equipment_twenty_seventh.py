from pathlib import Path
import json
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
rows=[]
domains={6403:[[33,15,41,23]],7659:[[20,2,40,15],[20,22,35,35]],4803:[[0,12,12,33],[0,34,12,55],[12,50,32,64]]}
for texture_id in [6403,7659,4803]:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    a=np.asarray(original.convert('RGB'))
    generated=np.asarray(Image.open(BASE/'private-equipment-twenty-seventh/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(original.size,Image.Resampling.LANCZOS))
    row={'id':texture_id,'source_alpha_extrema':list(original.getchannel('A').getextrema()),'native4x_source_alpha_byte_exact':True,'diagnostic':'Only whole calibrated native storedRGB inverse resize to native. Fixed domains numerical diagnostics only, no editing/reference crops/BBox fitting.','paint_peak_evidence':[]}
    for box in domains[texture_id]:
        x0,y0,x1,y1=box;entry={'native_domain':box}
        for name,t in [('source',a[y0:y1,x0:x1]),('candidate',generated[y0:y1,x0:x1])]:
            point=np.unravel_index(t.max(2).argmax(),t.shape[:2])
            entry[name+'_peak_xy']=[int(point[1]+x0),int(point[0]+y0)]
            entry[name+'_peak_RGB']=t[point].tolist()
        row['paint_peak_evidence'].append(entry)
    if texture_id==4803:
        for name,t in [('source',a),('candidate',generated)]:
            p=t[16:39,43:64].mean((0,2))
            row[name+'_right_irregular_tonal_profile_maxima_x']=[i+43 for i in range(1,len(p)-1)if p[i]>p[i-1]and p[i]>=p[i+1]]
        row['right_measured_phase_equal']=row['source_right_irregular_tonal_profile_maxima_x']==row['candidate_right_irregular_tonal_profile_maxima_x']
        row['physical_periodic_pattern_claimed']=False
    rows.append(row)
(BASE/'pattern-metrics-equipment-twenty-seventh.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(rows,ensure_ascii=False))

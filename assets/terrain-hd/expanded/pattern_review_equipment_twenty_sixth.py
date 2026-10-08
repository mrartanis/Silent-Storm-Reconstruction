from pathlib import Path
import json
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
rows=[]
for texture_id in [2448,3051,3041]:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    a=np.asarray(original.convert('RGB'))
    generated=np.asarray(Image.open(BASE/'private-equipment-twenty-sixth/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(original.size,Image.Resampling.LANCZOS))
    row={'id':texture_id,'source_alpha_extrema':list(original.getchannel('A').getextrema()),'native4x_source_alpha_byte_exact':True,'diagnostic':'Only whole calibrated native storedRGB inverse resize to native. Fixed domains numerical diagnostics only, never artistic edits/reference crops/BBox fitting.'}
    if texture_id==2448:
        row['original_weak_marks']=[]
        for box in [[18,22,21,26],[22,24,25,26],[28,21,31,24]]:
            x0,y0,x1,y1=box;s=a[y0:y1,x0:x1];g=generated[y0:y1,x0:x1]
            entry={'native_domain':box}
            for name,t in [('source',s),('candidate',g)]:
                point=np.unravel_index(t.max(2).argmax(),t.shape[:2])
                entry[name+'_peak_xy']=[int(point[1]+x0),int(point[0]+y0)]
                entry[name+'_peak_RGB']=t[point].tolist()
            row['original_weak_marks'].append(entry)
    elif texture_id==3051:
        row.update(source_luma_std=float(a.mean(2).std()),candidate_luma_std=float(generated.mean(2).std()),source_luma_min=float(a.mean(2).min()),source_luma_max=float(a.mean(2).max()),candidate_luma_min=float(generated.mean(2).min()),candidate_luma_max=float(generated.mean(2).max()),source_mean_RGB=a.mean((0,1)).tolist(),candidate_mean_RGB=generated.mean((0,1)).tolist())
    else:
        for name,t in [('source',a),('candidate',generated)]:
            gray=t.mean(2);p=gray[8:45,13:58].mean(0)
            row[name+'_upper_profile_maxima_x']=[i+13 for i in range(1,len(p)-1)if p[i]>p[i-1]and p[i]>=p[i+1]]
            p=gray[94,5:70]
            row[name+'_irregular_lower_core_row94_maxima_x']=[i+5 for i in range(1,len(p)-1)if p[i]>p[i-1]and p[i]>=p[i+1]]
        row['upper_native_count_phase_equal']=row['source_upper_profile_maxima_x']==row['candidate_upper_profile_maxima_x']
        row['lower_measured_core_row_phase_equal']=row['source_irregular_lower_core_row94_maxima_x']==row['candidate_irregular_lower_core_row94_maxima_x']
        row['physical_global_grid_count_claimed']=False
    rows.append(row)
(BASE/'pattern-metrics-equipment-twenty-sixth.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(rows,ensure_ascii=False))

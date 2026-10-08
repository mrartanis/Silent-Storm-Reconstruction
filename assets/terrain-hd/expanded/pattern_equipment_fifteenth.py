import json,hashlib
from pathlib import Path
import numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
CONSTRAINTS={3036:{'bbox':[81,13,105,56],'bright_rows':list(range(15,55,3)),'dark_rows':list(range(16,56,3)),'count':14,'transition':'Existing lower diffuse curved transition y56..64 is outside the periodic core; retain as source, not a new hard rib.'},3044:{'bbox':[5,8,31,66],'bright_rows':list(range(9,64,3)),'dark_rows':list(range(10,65,3)),'count':19,'transition':'Existing upper cap y0..8 and lower diffuse cap y66..80 lie outside the periodic core; retain their original tone/form, no hard extra ribs.'}}
source=[];qa=[]
for i,c in CONSTRAINTS.items():
    original=Image.open(BASE/f'original/{i}.png').convert('RGBA');s=np.asarray(original.convert('RGB'),dtype=float).mean(2)
    x0,y0,x1,y1=c['bbox'];v=s[:,x0:x1].mean(1)
    # All fully bounded peaks in the defined regular core are at these exact
    # native row positions. Tapered caps are deliberately outside this ROI.
    peaks=[y for y in range(y0+1,y1-1) if v[y]>v[y-1] and v[y]>=v[y+1]]
    assert peaks==c['bright_rows'],(i,peaks,c['bright_rows'])
    source.append({'id':i,**c,'source_RGBA_sha256':hashlib.sha256(original.tobytes()).hexdigest(),'coordinate_space':'Original native128x128 logical texels; bbox half-open; rows global Y. Pattern annotations constrain art only, never crop/compose/register output. Entire source UV canvas used for imagegen.','period':3,'phase_mod3':0,'source_gray_row_mean':[{'y':y,'meanRGB':float(v[y])} for y in range(y0,y1)],'proof':'Read-only exact source row-average local-maximum test within the periodic core equals explicitly enumerated rows. Separate cap/transition material not assigned a new ridge count.'})
    path=BASE/f'private-equipment-fifteenth/native4x/{i}-stored-rgb-private.png'
    if path.exists():
        g=np.asarray(Image.open(path).convert('RGB').resize(original.size,Image.Resampling.LANCZOS),dtype=float).mean(2);gv=g[:,x0:x1].mean(1)
        gp=[y for y in range(y0+1,y1-1) if gv[y]>gv[y-1] and gv[y]>=gv[y+1]]
        contrast=[float(gv[y]-0.5*(gv[y-1]+gv[y+1])) for y in c['bright_rows']]
        qa.append({'id':i,'source_count':c['count'],'source_peak_rows':c['bright_rows'],'candidate_peak_rows':gp,'candidate_peak_count':len(gp),'exact_logical_phase_and_count':gp==c['bright_rows'],'candidate_contrast_at_original_peak_rows':contrast,'source_contrast_at_original_peak_rows':[float(v[y]-0.5*(v[y-1]+v[y+1])) for y in c['bright_rows']],'candidate_gray_row_mean':[{'y':y,'meanRGB':float(gv[y])} for y in range(y0,y1)],'diagnostic_only':'Private wholecanvas scalar/native storedRGB inverse to source size; local peaks/count/phase aid visual review, are not acceptance alone. No image modification or source art RGB insertion.'})
(BASE/'pattern-constraints-equipment-fifteenth.json').write_text(json.dumps(source,indent=2)+'\n',encoding='utf-8')
if qa:(BASE/'pattern-metrics-equipment-fifteenth.json').write_text(json.dumps(qa,indent=2)+'\n',encoding='utf-8')
print('Source periodic cores verified:',[(r['id'],r['count']) for r in source]);print('Candidate patternQA',[(r['id'],r['candidate_peak_count'],r['exact_logical_phase_and_count']) for r in qa])

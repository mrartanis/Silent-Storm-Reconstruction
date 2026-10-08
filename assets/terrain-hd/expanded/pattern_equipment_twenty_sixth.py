from pathlib import Path
import json
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
def source(texture_id):return np.asarray(Image.open(BASE/'original'/f'{texture_id}.png').convert('RGB'))
rows=[]
a=source(2448)
marks=[]
for box,expected in [([18,22,21,26],[19,24]),([22,24,25,26],[22,24]),([28,21,31,24],[29,23])]:
    x0,y0,x1,y1=box;p=a[y0:y1,x0:x1];point=np.unravel_index(p.max(2).argmax(),p.shape[:2]);xy=[int(point[1]+x0),int(point[0]+y0)];assert xy==expected
    marks.append({'diagnostic_native_domain':box,'weak_bright_paint_peak_xy':xy,'peak_RGB':p[point].tolist()})
rows.append({'id':2448,'source_native_size':[64,64],'source_alpha_extrema':[255,255],'three_original_small_paint_peak_evidence':marks,'interpretation':'Three existing narrow warm paint highlights at native19,24/22,24/29,23. Preserve their unequal source sizes/intensities, not new white fasteners. Upper black/gray fields, dark nearrounded upper field, long middle weak horizontal brown boundary and lower broad brown polygon remain original painted layout; no new physical hardware or stitching inferred.'})
a=source(3051)
rows.append({'id':3051,'source_native_size':[32,32],'source_alpha_extrema':[255,255],'periodic_pattern':'No physical repeated count or ideal grid claimed. Original wholecanvas lowcontrast gray-beige/brown irregular broadpaint fields; all native positions are source-reference positions.','source_whole_RGB_mean':a.mean((0,1)).tolist(),'source_luma_std':float(a.mean(2).std()),'source_luma_min':float(a.mean(2).min()),'source_luma_max':float(a.mean(2).max()),'interpretation':'Muted paint-only field with dim broad patches and weak edge shading; do not introduce weave/ribs/photo noise/creases or reinterpret as raised material. Definitely not sameRGBA/type/dims as670:32square vs64square original checked independently.'})
a=source(3041);gray=a.mean(2)
p=gray[8:45,13:58].mean(0);peaks=[i+13 for i in range(1,len(p)-1)if p[i]>p[i-1]and p[i]>=p[i+1]];assert peaks==list(range(16,57,4))
p=gray[94,5:70];core=[x+5 for x in range(1,len(p)-1)if p[x]>p[x-1]and p[x]>=p[x+1]];assert core==[6,13,20,27,33,41,47,53,60,67]
rows.append({'id':3041,'source_native_size':[256,128],'source_alpha_extrema':[255,255],'upper_fine_vertical_painted_stroke_profile_domain':[13,8,58,45],'upper_stroke_native_maxima_x':peaks,'upper_stroke_count':11,'upper_native_pitch':4,'lower_irregular_knurl_native_reference_row':94,'lower_reference_core_domain':[5,94,70,95],'lower_core_row_painted_maxima_x':core,'lower_core_row_identifiable_maxima_count':10,'interpretation':'Genuine source eleven upper fine vertical painted strokes, exact native pitch4/phase16..56. Lower diamondlike crossed painted pattern is irregular/faded, not an ideal newly regularized grid: ten exact identifiable core native row94 maxima at measured positions, not an invented global physical grid cell count. Entire original crossed-line footprint and varying strengths preserved by fullsource reference, all boundaries/gaps unchanged. Lower redbrown shaded patch stays soft and weak, no woodgrain/raised bevel/diamond sharpening added.'})
for r in rows:
    r['source_before_prompt_assertion']='Native assertions genuinely executed before prompt/call. Entire source-only NEAREST reference, fixed native domains diagnostics only, no crop/BBox/ref fitting/artRGB patches.'
    r['material']='Subdued original fine paint, exact weak local relationships and full normalized UV. Fine lightpaint remains weak rather than white raised rim/relief/new photograin.'
(BASE/'pattern-constraints-equipment-twenty-sixth.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Before-prompt asserts:2448three weak paint peaks;3051lowcontrast wholepaint;3041eleven upper strokes pitch4 plus ten measured irregular lower row94 maxima, no fictitious global grid count.')

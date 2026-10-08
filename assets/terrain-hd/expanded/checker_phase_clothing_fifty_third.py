from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-fifty-third'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
c=read(B/f'pattern-constraints-{ST}.json')[0];i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);rnd=np.array(read(B/f'natural-RGB-roundtrip-{ST}.json')['sources'][str(i)]['source_RGB_LANCZOS4x_then_native_RGB_matrix'],dtype=float);p=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),dtype=float)
yy,xx=np.mgrid[36:56,80:128];phase=(xx+yy)%2==0;rows={}
for name,a in [('source',s),('natural_source_RGB_roundtrip',rnd),('production_RGB_inverse',p)]:
 r=a[36:56,80:128];warm=r[:,:,0]-r[:,:,2]>20;lum=r.mean(2)
 rows[name]={'fixed_native_bbox':[80,36,128,56],'source_defined_chroma_class_warm_pixel_count':int(warm.sum()),'source_defined_chroma_class_gray_pixel_count':int((~warm).sum()),'classified_warm_vs_original_even_parity_mismatches':int((warm!=phase).sum()),'warm_condition':'R-B>20 separates the original source warm vs gray classes exactly in this proven interior. Applied to diagnostics only; no native alteration/physicalpart counter or acceptance threshold.','RGB_mean_on_original_warm_locations':r[phase].mean(0).tolist(),'RGB_mean_on_original_gray_locations':r[~phase].mean(0).tolist(),'every_row_native_y_signed_original_even_minus_odd_meanRGB':[{'y':36+k,'difference':float(lum[k,phase[k]].mean()-lum[k,~phase[k]].mean())}for k in range(20)],'every_column_native_x_signed_original_even_minus_odd_meanRGB':[{'x':80+k,'difference':float(lum[:,k][phase[:,k]].mean()-lum[:,k][~phase[:,k]].mean())}for k in range(48)]}
out={'id':i,'before_exact_phase_ref':c['source_checker_exact_pixel_phase'],'qualification':'Complete BEFORE nativeRGBA/commonmatrix/sourcecount unchanged. Checker diagnostics refer to source pixelpainting phase, not physical textileholes or newcomponent claims. WholeRGB-FIRST native inverse and independentA, no localfit/gain/repair. Natural4x source roundtrip independently compared.','checks':rows}
(B/f'postcall-checker-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({n:{k:v for k,v in r.items()if not k.startswith('every_')}for n,r in rows.items()},indent=2))

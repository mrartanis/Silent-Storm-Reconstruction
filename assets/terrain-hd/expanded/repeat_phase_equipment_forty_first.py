from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='equipment-forty-first';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),dtype=float);sl=s.mean(2);gl=g.mean(2)
 d={'id':i,'qualification':'Postcall complete storedRGB inverse only independently of sourceA. Literal pixel maxima/selection/count/rowprofiles are descriptive diagnostics, NOT physical cells/hardware/acceptance thresholds. BEFORE proof unchanged. No cropping/registration/repair/localgain/retry.','checks':[]}
 if i==5431:
  raw=np.array(Image.open(B/f'generated/{i}-raw.png').convert('RGB'));rows=(raw.min(2)>=250).mean(1);top=0;bottom=0
  for v in rows:
   if v<.995:break
   top+=1
  for v in rows[::-1]:
   if v<.995:break
   bottom+=1
  d['raw_white_canvas_border_diagnostic']={'raw_size':[raw.shape[1],raw.shape[0]],'nearly_allwhite_row_recipe':'minRGB>=250 fraction>=.995; diagnostic only NOT BBox/crop/fitting authorization','consecutive_top_rows':top,'consecutive_bottom_rows':bottom,'normalized_top_fraction':top/raw.shape[0],'normalized_bottom_fraction':bottom/raw.shape[0]}
  for before in c['native_repeat_count_phase_evidence']:
   y=before['native_y'];x0,x1=before['native_x_interval'];r={'native_y':y,'native_x_interval':[x0,x1]}
   for n,a in [('source',sl),('production',gl)]:
    xs=[x for x in range(x0,x1)if a[y,x]>a[y,x-1]and a[y,x]>=a[y,x+1]];r[n]={'literal_row_maximum_count':len(xs),'native_maximum_x':xs,'successive_pitch':[b-a for a,b in zip(xs,xs[1:])]}
   d['checks'].append(r)
 else:
  for n,a in [('source',s),('production',g)]:
   m=(a[:64,:,2]>=a[:64,:,0])&(a[:64].min(2)>=25)&(a[:64].mean(2)>=45);d[f'{n}_literal_graypaint_selection']={'same_before_recipe':c['literal_source_weak_graypaint_evidence']['literal_cool_gray_source_selection_recipe'],'selected_pixel_count':int(m.sum()),'mean_RGB_of_selection':a[:64][m].mean(0).tolist(),'qualification':'No threshold physicalcomponent claim; before fullnativeRGBA authoritative including EVERY subthreshold pigment.'}
  for x,y in [(38,78),(124,56),(50,78),(64,25),(86,27),(109,91)]:d['checks'].append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist(),'qualification':'Actual original pigment samples only, no hardware/anatomy/materialhole inference.'})
 out.append(d)
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in out:
 print(r['id'],r.get('raw_white_canvas_border_diagnostic'),r.get('source_literal_graypaint_selection'),r.get('production_literal_graypaint_selection'))
 print([(x['native_y'],x['source']['literal_row_maximum_count'],x['production']['literal_row_maximum_count'])for x in r['checks']if 'native_y'in x])

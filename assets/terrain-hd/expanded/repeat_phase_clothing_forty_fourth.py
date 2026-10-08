from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-forty-fourth';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
R=read(B/f'natural-RGB-roundtrip-{ST}.json')['sources']
coords={6823:[(2,80),(4,80),(3,85),(113,117),(113,116),(114,117),(71,32),(71,35),(71,38),(72,33),(72,36),(72,39)],7607:[(106,66),(106,65),(105,66),(34,60),(34,61),(38,78),(3,72),(0,127)],7610:[(106,66),(107,66),(34,60),(34,61),(38,78),(10,87),(0,127)]}
regions={6823:[[69,29,78,42],[1,79,9,87],[112,115,117,120]],7607:[[0,66,26,96],[32,58,36,64],[36,75,44,85],[103,63,111,70]],7610:[[0,66,26,96],[32,58,36,64],[36,75,44,85],[103,63,111,70]]}
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);p4=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');g=np.array(p4.resize(o.size,Image.Resampling.LANCZOS),dtype=float);rnd=np.array(R[str(i)]['source_RGB_LANCZOS4x_then_native_RGB_matrix'],dtype=float);checks=[]
 for p in c['native_repeat_count_phase_evidence']:
  if 'axis'not in p:continue
  d={k:p[k]for k in ['axis','native_fixed_coordinate','native_axis_interval']};coord=p['native_fixed_coordinate'];z0,z1=p['native_axis_interval']
  for name,a in [('source',s.mean(2)),('source_RGB_natural_roundtrip',rnd.mean(2)),('production',g.mean(2))]:
   v=a[coord]if p['axis']=='row'else a[:,coord];zs=[z for z in range(max(1,z0),min(z1,len(v)-1))if v[z]>v[z-1]and v[z]>=v[z+1]];d[name]={'literal_paintshade_maximum_axis_coordinates':zs,'literal_count':len(zs),'native_successive_pitch':[b-a for a,b in zip(zs,zs[1:])],'meanRGB_profile':v[z0:z1].tolist()}
  d['qualification']='Literal shaded pigment maxima only, not inferred physical parts or any fidelity threshold.';checks.append(d)
 for x,y in coords[i]:checks.append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'source_RGB_natural_roundtrip':rnd[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist()})
 rr=[]
 for box in regions[i]:
  x0,y0,x1,y1=box;r={'native_bbox':box,'qualification':'Small literal RGB matrices for diagnostic private review ONLY; generated/production full canvas untouched, no cropped fitting, insertion or repair.'}
  for name,a in [('source',s),('source_RGB_natural_roundtrip',rnd),('production',g)]:
   z=a[y0:y1,x0:x1];r[name+'_RGB_matrix']=z.astype(int).tolist();m=z.mean(2);peaks=[]
   for yy in range(1,m.shape[0]-1):
    for xx in range(1,m.shape[1]-1):
     if m[yy,xx]>m[yy,xx-1]and m[yy,xx]>=m[yy,xx+1]and m[yy,xx]>m[yy-1,xx]and m[yy,xx]>=m[yy+1,xx]:peaks.append({'xy':[x0+xx,y0+yy],'RGB':z[yy,xx].astype(int).tolist()})
   r[name+'_literal_orthogonal_local_pigment_maxima']=peaks
  rr.append(r)
 for name,im in [('source',o),('production',p4.resize(o.size,Image.Resampling.LANCZOS))]:im.resize((512,512),Image.Resampling.NEAREST).save(B/f'private-{ST}/native4x/{i}-{name}-native-inverse-nearest512.png')
 out.append({'id':i,'qualification':'Fullcanvas RGB-FIRST independentlyA source4x/natural native roundtrip and actual production scalar inverse; all phase/weak source pigment before full matrix preserved. Literal profiles are not physical component counters or invented thresholds. Finepaint/one-nativepixel smoothing alone not held. No source RGB patch, crop, local gain or registration.','checks':checks,'region_diagnostics':rr})
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in out:
 print(r['id'],[(d['native_xy'],d['source_RGB'],d['source_RGB_natural_roundtrip'],d['production_inverse_RGB'])for d in r['checks']if'native_xy'in d])
 if r['id']==6823:
  z=r['region_diagnostics'][0];print('6823 small repeat ROI maxima',[(k,z[k])for k in z if k.endswith('pigment_maxima')])

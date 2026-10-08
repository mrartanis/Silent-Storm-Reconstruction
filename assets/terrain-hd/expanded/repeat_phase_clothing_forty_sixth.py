from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-forty-sixth';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
R=read(B/f'natural-RGB-roundtrip-{ST}.json')['sources']
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id']
 if not (B/f'generated/{i}-raw.png').exists():continue
 o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);p4=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');g=np.array(p4.resize(o.size,Image.Resampling.LANCZOS),dtype=float);rnd=np.array(R[str(i)]['source_RGB_LANCZOS4x_then_native_RGB_matrix'],dtype=float);checks=[]
 for p in c['native_repeat_count_phase_evidence']:
  d={k:p[k]for k in ['axis','native_fixed_coordinate','native_axis_interval']};coord=p['native_fixed_coordinate'];z0,z1=p['native_axis_interval']
  for name,a in [('source',s.mean(2)),('source_RGB_natural_roundtrip',rnd.mean(2)),('production',g.mean(2))]:
   v=a[coord]if p['axis']=='row'else a[:,coord];zs=[z for z in range(max(1,z0),min(z1,len(v)-1))if v[z]>v[z-1]and v[z]>=v[z+1]];d[name]={'literal_paintshade_maximum_axis_coordinates':zs,'literal_count':len(zs),'native_successive_pitch':[b-a for a,b in zip(zs,zs[1:])],'meanRGB_profile':v[z0:z1].tolist()}
  d['qualification']='Literal sourcepaint pigment maxima/nativephase only, not physicalbar/bolt/hardware counters or acceptance thresholds.';checks.append(d)
 for x,y in [(112,11),(67,48),(60,46),(48,72),(81,72),(64,58),(63,43),(24,12),(65,8),(108,8),(64,92),(0,127)]:checks.append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'source_RGB_natural_roundtrip':rnd[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist()})
 for name,im in [('source',o),('production',p4.resize(o.size,Image.Resampling.LANCZOS))]:im.resize((512,512),Image.Resampling.NEAREST).save(B/f'private-{ST}/native4x/{i}-{name}-native-inverse-nearest512.png')
 out.append({'id':i,'qualification':'Fullcanvas storedRGB FIRST separatelysourceA nativeinverse/naturalroundtrip. Onepixel smoothing/moderatepaintcontrast itself nothold; actualpaint/material/weakphase drift is qualitative fullsource/raw/native4x reason. No croppedfit/artrepair/gainthreshold/newphysicalpart claims.','checks':checks})
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in out:print(r['id'],[(d['native_xy'],d['source_RGB'],d['source_RGB_natural_roundtrip'],d['production_inverse_RGB'])for d in r['checks']if'native_xy'in d])

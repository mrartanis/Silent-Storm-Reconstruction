from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='equipment-forty-second';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),dtype=float);rnd=np.array(o.resize((256,256),Image.Resampling.LANCZOS).resize(o.size,Image.Resampling.LANCZOS),dtype=float)
 d={'id':i,'qualification':'Postcall ENTIRE storedRGB/native inverse separately sourceA, nativecol/row literalshade maxima are descriptive color-phase evidence not physicalhardware/cell counts nor autoacceptthreshold. SourceRGB LANCZOS4x roundtrip operator included; no RGBrepair/registration/crop/retry.','checks':[]}
 for p in c['native_repeat_count_phase_evidence']:
  r={k:v for k,v in p.items()if k in ['axis','native_x','native_y','native_x_interval','native_y_interval']}
  for n,a in [('source',s.mean(2)),('source_LANCZOS_roundtrip',rnd.mean(2)),('production',g.mean(2))]:
   if p['axis']=='column':x=p['native_x'];v=a[:,x];z0,z1=p['native_y_interval']
   else:y=p['native_y'];v=a[y,:];z0,z1=p['native_x_interval']
   zs=[z for z in range(z0,z1)if v[z]>v[z-1]and v[z]>=v[z+1]];r[n]={'literal_paintshade_maximum_count':len(zs),'native_maximum_axis_coordinates':zs,'native_successive_pitch':[b-a for a,b in zip(zs,zs[1:])],'same_native_axis_meanRGB_profile':v[z0:z1].tolist()}
  d['checks'].append(r)
 xy={1915:[(11,13),(47,51),(0,63),(37,31)],1919:[(9,53),(52,15),(35,54),(0,63)],1920:[(23,19),(6,30),(0,63),(32,63),(63,63),(44,45)]}[i]
 for x,y in xy:d['checks'].append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'source_RGB_LANCZOS_roundtrip':rnd[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist(),'qualification':'Exact originalsource colorpaint/blackfield samples, allactualsourceA255. No hardware or hole semantic inference.'})
 out.append(d)
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for d in out:
 print(d['id'],[(r.get('native_x',r.get('native_y')),r['source']['literal_paintshade_maximum_count'],r['source_LANCZOS_roundtrip']['literal_paintshade_maximum_count'],r['production']['literal_paintshade_maximum_count'])for r in d['checks']if'axis'in r]);print([(r['native_xy'],r['source_RGB'],r['source_RGB_LANCZOS_roundtrip'],r['production_inverse_RGB'])for r in d['checks']if'native_xy'in r])

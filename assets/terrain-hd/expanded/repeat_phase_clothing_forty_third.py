from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='clothing-forty-third';out=[]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
R=read(B/f'natural-RGB-roundtrip-{ST}.json')['sources']
for c in read(B/f'pattern-constraints-{ST}.json'):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');s=np.array(o,dtype=float);g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(o.size,Image.Resampling.LANCZOS),dtype=float);rnd=np.array(R[str(i)]['source_RGB_LANCZOS4x_then_native_RGB_matrix'],dtype=float);checks=[]
 for p in c['native_repeat_count_phase_evidence']:
  d={k:p[k]for k in ['axis','native_fixed_coordinate','native_axis_interval']};coord=p['native_fixed_coordinate'];z0,z1=p['native_axis_interval']
  for name,a in [('source',s.mean(2)),('source_RGB_natural_roundtrip',rnd.mean(2)),('production',g.mean(2))]:
   v=a[coord]if p['axis']=='row'else a[:,coord];zs=[z for z in range(z0,z1)if v[z]>v[z-1]and v[z]>=v[z+1]];d[name]={'literal_paintshade_maximum_axis_coordinates':zs,'literal_count':len(zs),'native_successive_pitch':[b-a for a,b in zip(zs,zs[1:])],'meanRGB_profile':v[z0:z1].tolist()}
  d['qualification']='Literal nativepaint shadedmaxima/phase only, NOT physicalribs/hardware counts or threshold. NaturalRGB full4x→native operatorroundtrip separateA.';checks.append(d)
 for x,y in {6813:[(244,15),(248,33),(99,109),(102,101),(65,117),(0,127),(134,32),(134,33)],6814:[(202,93),(202,94),(99,103),(102,101),(65,117),(0,127),(134,32),(134,33)],6817:[(122,16),(245,15),(97,101),(102,101),(65,117),(0,127),(134,32),(134,33),(134,34)]}[i]:checks.append({'native_xy':[x,y],'source_RGB':s[y,x].tolist(),'source_RGB_natural_roundtrip':rnd[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist(),'qualification':'Exact authoredsourcepigment/opaque blackUV nativeRGB; no sourceRGBartistrepair or newphysicalpart inference.'})
 out.append({'id':i,'qualification':'Nativewhole RGB-FIRST independentA, allsourceweakpaint/blur/countphase compared, no cropped fit/perregion gain/repair. Observed moderateone-pixeloperatorchange alone NOTautomaticheld. Actualmaterial/newrelief/weakpigment reinterpretation qualitative source fullview evidence.','checks':checks})
(B/f'repeat-phase-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in out:print(r['id'],[(d['axis'],d['native_fixed_coordinate'],d['source']['literal_count'],d['source_RGB_natural_roundtrip']['literal_count'],d['production']['literal_count'])for d in r['checks']if'axis'in d]);print([(d['native_xy'],d['source_RGB'],d['source_RGB_natural_roundtrip'],d['production_inverse_RGB'])for d in r['checks']if'native_xy'in d])

from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageFilter
BASE=Path(__file__).resolve().parent
STEM='equipment-thirty-first'
out=[]
domains={1821:[[18,25,41,62],[18,75,40,100]],1827:[[11,35,22,85],[34,38,42,90]],1921:[[6,21,20,30],[44,25,54,38],[3,55,59,60]]}
for i in [1821,1827,1921]:
 original=Image.open(BASE/f'original/{i}.png').convert('RGB')
 candidate=Image.open(BASE/f'private-{STEM}/native4x/{i}-stored-rgb-private.png').resize(original.size,Image.Resampling.LANCZOS).convert('RGB')
 s=np.asarray(original,dtype=float);g=np.asarray(candidate,dtype=float)
 sl=s.mean(2);gl=g.mean(2)
 sh=sl-np.asarray(original.filter(ImageFilter.GaussianBlur(2))).mean(2)
 gh=gl-np.asarray(candidate.filter(ImageFilter.GaussianBlur(2))).mean(2)
 r={'id':i,'timing':'Postcall private readonly whole native inverse diagnostics. Precall original native count/phase assertions immutable. No artistic RGB patch/fitting/crop.','material_domains':[]}
 for box in domains[i]:
  x0,y0,x1,y1=box;ss=sl[y0:y1,x0:x1];gg=gl[y0:y1,x0:x1]
  r['material_domains'].append({'native_diagnostic_domain':box,'source_luma_mean':float(ss.mean()),'candidate_luma_mean':float(gg.mean()),'source_luma_std':float(ss.std()),'candidate_luma_std':float(gg.std()),'source_highpass_std':float(sh[y0:y1,x0:x1].std()),'candidate_highpass_std':float(gh[y0:y1,x0:x1].std()),'highpass_correlation':float(np.corrcoef(sh[y0:y1,x0:x1].ravel(),gh[y0:y1,x0:x1].ravel())[0,1])})
 if i==1821:
  checks=[]
  for y,expected in {20:[9,11,13],21:[8,10,12],22:[9,11,13],23:[9,11,13],25:[9,11,13]}.items():
   d={'native_row':y}
   for name,a in [('source',s),('candidate',g)]:
    p=a[y,7:15].mean(1);xs=[x+7 for x in range(1,len(p)-1)if p[x]>p[x-1]and p[x]>=p[x+1]];assert xs==expected
    d[name]={'native_tonal_maxima_x':xs,'count':3,'profile':p.tolist()}
   checks.append(d)
  r['exact_faint_left_pattern_phase']=checks
  r['dark_marker_diagnostic']={'source_core_mean':float(sl[66:68,34:41].mean()),'candidate_core_mean':float(gl[66:68,34:41].mean()),'source_core_RGBmean_lt40_pixels':int((sl[66:68,34:41]<40).sum()),'candidate_core_RGBmean_lt40_pixels':int((gl[66:68,34:41]<40).sum())}
 elif i==1827:
  checks=[]
  for name,a in [('source',s),('candidate',g)]:
   p=a[47:103,24:31].mean((1,2));ys=[j+47 for j in range(1,len(p)-1)if p[j]>p[j-1]and p[j]>=p[j+1]and p[j]>50]
   assert ys==[51,56,63,69,75,81,87,93,99]
   checks.append({'name':name,'source_warm_band_count':9,'native_warm_band_y':ys,'profile':p.tolist()})
  r['exact_nine_warm_tonal_bands']=checks
 else:
  checks=[]
  for y in range(25,30):
   d={'native_row':y}
   for name,a in [('source',s),('candidate',g)]:
    p=a[y,43:56].mean(1);xs=[x+43 for x in range(1,len(p)-1)if p[x]>p[x-1]and p[x]>=p[x+1]]
    d[name]={'native_maxima_x':xs,'count':len(xs),'profile':p.tolist()}
   checks.append(d)
  r['checker_native_phase']=checks
  r['source_measured_checker_maxima']=sum(c['source']['count']for c in checks)
  r['candidate_measured_checker_maxima']=sum(c['candidate']['count']for c in checks)
  r['original_pale_point']={'native_xy':[50,34],'source_RGB':s[34,50].tolist(),'candidate_RGB':g[34,50].tolist()}
  r['note']='Row25 weak native44 maximum is not distinct after candidate inverse (source28→candidate27 total maxima). Four other rows/2px pitch/alternating phase retained. This weak tonal threshold/1px issue is not sole hold; fullraw material/edge strength concern independently recorded. No ideal physicalknurl count asserted.'
 out.append(r)
 print(i,[(round(d['source_luma_mean'],2),round(d['candidate_luma_mean'],2),round(d['highpass_correlation'],3))for d in r['material_domains']])
for name in ['material-pattern-metrics','tiny-marks-review']:(BASE/f'{name}-{STEM}.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')

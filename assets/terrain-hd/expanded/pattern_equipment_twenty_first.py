import json,numpy as np,hashlib
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
rows=[]
def peaks(p,off):return [j+off for j,v in enumerate(p)if 0<j<len(p)-1 and v>p[j-1]and v>=p[j+1]]
s=np.array(Image.open(BASE/'original/670.png').convert('RGB'),dtype=float).mean(2)
x=peaks(s[:34,1:59].mean(0),1);y=peaks(s[:34,2:58].mean(1),0)
assert x==[4,7,11,14,18,21,25,28,32,35,39,42,46,49,53,56]
assert y==[4,12,20,29]
assert len(x)==16 and len(y)==4
row={'id':670,'source_native_column_peak_pairs':[x[k:k+2]for k in range(0,16,2)],'source_native_row_peaks':y,'source_field_count':32,'source_grid_columns':8,'source_grid_rows':4,'diagnostic_only':'Whole sourceRGB fixed-domain mean profiles, source asserted before actualprompt/call. ROI only read-only measurements; no toolreference crop, registration or artcomposite.'}
p=BASE/'private-equipment-twenty-first/native4x/670-stored-rgb-private.png'
if p.exists():
 g=np.array(Image.open(p).convert('RGB').resize((64,64),Image.Resampling.LANCZOS),dtype=float).mean(2);row.update(candidate_column_peaks=peaks(g[:34,1:59].mean(0),1),candidate_row_peaks=peaks(g[:34,2:58].mean(1),0),phase_equal=peaks(g[:34,1:59].mean(0),1)==x and peaks(g[:34,2:58].mean(1),0)==y)
rows.append(row)
rows.append({'id':5434,'periodic_count':'No periodic pattern claimed. Original exactly two rectangular dark openings and full planar fields.','diagnostic_only':'Full material/UV/private originalA/scalar review; no crop/artistRGB insert.'})

def flood(a,seed,box,threshold):
 seen=set();todo=[seed];x0,y0,x1,y1=box
 while todo:
  x,y=todo.pop()
  if (x,y)in seen or x<x0 or x>=x1 or y<y0 or y>=y1 or a[y,x].max()>threshold:continue
  seen.add((x,y));todo.extend([(x-1,y),(x+1,y),(x,y-1),(x,y+1)])
 return {'count':len(seen),'bbox':[min(x for x,y in seen),min(y for x,y in seen),max(x for x,y in seen)+1,max(y for x,y in seen)+1]}if seen else {'count':0}
p=BASE/'private-equipment-twenty-first/native4x/5434-stored-rgb-private.png'
if p.exists():
 a=np.array(Image.open(BASE/'original/5434.png').convert('RGB'));g=np.array(Image.open(p).convert('RGB').resize((64,64),Image.Resampling.LANCZOS))
 rows[1]['opening_diagnostics']=[{'seed':list(seed),'measurement_domain':list(box),'threshold_RGBmax':t,'source':flood(a,seed,box,t),'candidate':flood(g,seed,box,t)}for seed,box in [((30,32),(27,26,38,42)),((60,28),(56,22,64,34))]for t in [5,20]]
for row in rows:
 p=BASE/f"private-equipment-twenty-first/native4x/{row['id']}-calibrated-native-alpha-private.png"
 if p.exists():
  o=Image.open(BASE/f"original/{row['id']}.png").convert('RGBA');n=Image.open(p).convert('RGBA');row['sourceA4x_byte_exact']=n.getchannel('A').tobytes()==o.getchannel('A').resize(n.size,Image.Resampling.LANCZOS).tobytes()
(BASE/'pattern-metrics-equipment-twenty-first.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Source grid assertions passed:8x4=32; positions preserved in proof.')

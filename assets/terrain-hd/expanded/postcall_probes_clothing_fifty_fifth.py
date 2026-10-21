from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-fifth';i=6806
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ref(p):return {'path':p.relative_to(ROOT).as_posix(),'file_SHA256':sha(p)}
s=Image.open(B/f'original/{i}.png').convert('RGB');sz=(s.width*4,s.height*4);s4=s.resize(sz,Image.Resampling.LANCZOS);rnd=s4.resize(s.size,Image.Resampling.LANCZOS)
p4=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');p=p4.resize(s.size,Image.Resampling.LANCZOS)
u=Image.open(B/f'private-{ST}/before-aliases/{i}-resource-{i|0x01000000}-native-opaque-storedRGB.png').convert('RGB');u4=u.resize(sz,Image.Resampling.LANCZOS);un=u4.resize(u.size,Image.Resampling.LANCZOS)
u4.save(B/f'private-{ST}/native4x/{i}-highbit-source-RGB-LANCZOS4x-private.png')
coords=[(53,10),(29,58),(74,58),(26,41),(26,48),(26,54),(26,59),(77,41),(77,48),(77,54),(77,58),(53,7),(135,76),(210,75),(0,127)]
rows=[{'xy':list(xy),'compressed_original_RGB':list(s.getpixel(xy)),'uncompressed_alias_RGB':list(u.getpixel(xy)),'compressed_natural_RGB_LANCZOS4x_inverse_RGB':list(rnd.getpixel(xy)),'uncompressed_natural_RGB_LANCZOS4x_inverse_RGB':list(un.getpixel(xy)),'production_whole_RGB_inverse_RGB':list(p.getpixel(xy))}for xy in coords]
source=np.array(s);alt=np.array(u);natural=np.array(rnd);prod=np.array(p);c=read(B/f'pattern-constraints-{ST}.json')[0];profiles=[]
for f in c['native_repeat_count_phase_evidence']:
 x0,y0,x1,y1=f['native_bbox'];vals={name:a[y0:y1,x0:x1].mean((1,2))for name,a in [('canonical_original',source),('highbit_original',alt),('canonical_natural_roundtrip',natural),('production_whole_inverse',prod)]}
 centers={}
 for name,v in vals.items():
  centers[name]=[lo+int(np.argmax(v[lo-y0:hi-y0]))for lo,hi in f['four_existing_pale_paint_band_intervals']]
 profiles.append({'name':f['name'],'native_bbox':f['native_bbox'],'source_bands':f['four_existing_pale_paint_band_intervals'],'peak_rows_in_same_original_band_intervals':centers,'full_row_means':{name:[{'y':y0+j,'meanRGB':float(z)}for j,z in enumerate(v)]for name,v in vals.items()},'qualification':'Original intervals intentionally fixed BEFORE. Position/strength diagnostics forpaint, not countphysicalbuckles. A1px shift may reflect natural filtering and is not automatic hold/newparts proof.'})
o={'id':i,'phase':'POSTCALL diagnostic; immutable BEFORE literal nativecount/phase+bothalias sourceproof. Numeric probes describepigment positions, not physicalobjects.','source_before_ref':ref(B/f'source-check-{ST}-before-call.json'),'alias_before_ref':ref(B/f'before-alias-proof-{ST}.json'),'literal_source_probes':rows,'graypaint_phase_profiles':profiles,'production_calibration_source':'Canonicalcompressed source_i unchanged. Highbit sourceRGB comparison separatelydiagnostic, not an alteredproductiongain. BothfullA255 byteequal; raw/globalgain restoredcanonical sourceA.','no_artist_RGB_repair':True}
(B/f'postcall-source-specific-probes-{ST}.json').write_text(json.dumps(o,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'probes':len(rows),'phase':[{k:v for k,v in f.items()if k in ['name','peak_rows_in_same_original_band_intervals']}for f in profiles]},indent=2))

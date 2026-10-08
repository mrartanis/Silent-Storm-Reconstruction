from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-twenty-fourth';CALL={4093,4095,4098};out=[]
for r in json.loads((B/f'source-check-{ST}-before-call.json').read_text(encoding='utf-8')):
 if r['id']in CALL:continue
 i=r['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');a=np.array(o);rgb=a[:,:,:3];l=rgb.mean(2);y,x=np.unravel_index(l.argmax(),l.shape)
 reason='Genuine dark soft sourcepaint fragment; this bounded scope did not establish every localshade/weakedge detail for solecall. Pending0calls, not a generic technical/family exclusion.'
 if i==5298:reason='Individually reviewed full storedRGB: one smooth centered green glow; no independently visible brush texture. Native fullRGBA matrix, center row/column intensities and sourceA0 document symmetric near-radial monotone falloff. Source-only analytical shading0calls, no newglowgrain.'
 out.append({'id':i,'native_size':list(o.size),'source_RGBA_extrema':[list(z)for z in o.getextrema()],'whole_native_RGBA_matrix':a.tolist(),'source_max_mean_RGBA_xy':[int(x),int(y)],'source_max_mean_RGBA':a[y,x].tolist(),'center_row_mean_RGB':l[o.height//2].tolist(),'center_column_mean_RGB':l[:,o.width//2].tolist(),'reason':reason,'imagegen_call_count':0,'genuine_pending':i!=5298,'not_accepted':True,'source_RGBA_sha256':r['source_rgba_sha256']})
(B/f'source-only-detail-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Source-only complete native matrices',len(out))

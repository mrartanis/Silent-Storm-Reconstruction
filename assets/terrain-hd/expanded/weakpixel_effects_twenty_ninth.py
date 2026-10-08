from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-twenty-ninth';out=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for i in [4086,2626]:
 o=np.array(Image.open(B/f'original/{i}.png').convert('RGBA'))
 g=np.array(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize((o.shape[1],o.shape[0]),Image.Resampling.LANCZOS))
 z=o[:,:,:3].max(2)==0;changed=z&(g.max(2)>5);locations=[]
 for y,x in zip(*np.where(changed)):
  neighbor=o[max(0,y-1):y+2,max(0,x-1):x+2,:3]
  locations.append({'xy':[int(x),int(y)],'source_RGBA':o[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist(),'adjacent_original_nonzero_RGB':bool(neighbor.max()>0)})
 weak=(o[:,:,:3].max(2)>0)&(o[:,:,:3].max(2)<=5)
 out.append({'id':i,'native_matrix_ref':{'path':f'assets/terrain-hd/expanded/native-matrices-{ST}.json','file_SHA256':sha(B/f'native-matrices-{ST}.json'),'source_key':str(i)},'production_matrix_ref':{'path':f'assets/terrain-hd/expanded/production-matrices-{ST}.json','file_SHA256':sha(B/f'production-matrices-{ST}.json'),'source_key':str(i)},'original_zero_RGB_native_locations_production_gt5':locations,'original_RGB1through5_native_pixel_count':int(weak.sum()),'weak_pixel_checks':[{'xy':[int(x),int(y)],'source_RGBA':o[y,x].tolist(),'production_inverse_RGB':g[y,x].tolist()}for y,x in zip(*np.where(weak))],'qualification':'Native inverse diagnostic, not a threshold acceptance rule. Whole storedRGB/A privately reviewed. Original RGB-zero with nonzero A can have source adjacent pigment; complete original alpha restored, no RGBpatch/threshold/crop or generated newfragment count asserted.'})
(B/f'weakpixel-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(out,ensure_ascii=False,indent=2))

from pathlib import Path
import json
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='equipment-thirty-fifth';i=7653
ns={'__file__':str(B/'detail_equipment_thirty_fifth.py')};exec((B/'detail_equipment_thirty_fifth.py').read_text(encoding='utf-8').split('for id_ in IDS:')[0],ns)
s=np.asarray(Image.open(B/f'original/{i}.png').convert('RGB'))[8:24,30:46].astype(int)
g=np.asarray(Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').resize((64,64),Image.Resampling.LANCZOS).convert('RGB'))[8:24,30:46].astype(int)
def mask(z):return(z.max(2)>50)&(abs(z[:,:,0]-z[:,:,1])<=7)&(abs(z[:,:,1]-z[:,:,2])<=7)
def result(z):
 m=mask(z);return{'components':ns['components'](m),'native_absolute_xy':[[int(x+30),int(y+8)]for y,x in zip(*np.where(m))],'exact_native_RGB_samples':[z[y,x].tolist()for y,x in zip(*np.where(m))]}
r={'id':i,'stage':'Supplemental POSTCALL tonal-class analysis, not retrospectively claimed count-beforeprompt. Whole originalRGBA/roundfield native matrix was already immutable beforeprompt; this additional color-class diagnostic is newly computed after call.','domain':[30,8,46,24],'algorithm':'Native source vs ENTIRE pure-native-production inverse-original: RGBmax>50, abs(R-G)<=7,abs(G-B)<=7; 8connected. Diagnostic nearlyneutral-gray class only, not acceptance threshold, not physical dot/part count, not crop/composite/artRGB source insertion.','source':result(s),'production_inverse':result(g),'qualification':'5 source ->1 generated nearlygray connected regions because warmer mixed shade and smoothing exclude peripheral flecks from grayclass. Do not claim disappearance of4 physical dots; corresponding weaker warm tonal remnants visible in fullraw/native, root may review palette/phase material fidelity.'}
(B/f'tiny-flecks-review-{ST}.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
print('Postcall grayclass diagnostic saved separately, no art edits')

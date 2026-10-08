import json,hashlib
from pathlib import Path
import numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
rows=[]
for i in [5403,5411]:
 src=Image.open(BASE/f'original/{i}.png').convert('RGBA');can=Image.open(BASE/f'private-equipment-twentieth/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA');stored=Image.open(BASE/f'private-equipment-twentieth/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(src.size,Image.Resampling.LANCZOS)
 exact=can.getchannel('A').tobytes()==src.getchannel('A').resize(can.size,Image.Resampling.LANCZOS).tobytes()
 row={'id':i,'full_sourceA4x_byte_exact':exact,'sourceA_extrema':list(src.getchannel('A').getextrema()),'diagnostic_only':'Whole storedRGB inverse to original native dimensions. Read-only profiles; no crop/BBox/composition/registration or artistic sourceRGB insert. OriginalA restoration constrains final cuts/outer silhouette independently, not a claim of full material fidelity.'}
 if i==5411:
  def peaks(im):
   p=np.array(im.convert('RGB'),dtype=float).mean(2)[:40,4:24].mean(1)
   return [j for j,v in enumerate(p)if 0<j<len(p)-1 and v>p[j-1] and v>=p[j+1]]
  row.update(profile_native_domain=[4,0,24,40],source_profile_maxima_y=peaks(src),candidate_profile_maxima_y=peaks(stored),phase_and_count_equal=peaks(src)==peaks(stored),reason='13 source local paint-profile maxima become12, several shifts1pixel. No ideal periodic rib count inferred; irregular source profile is authoritative. Original soft spots also interpreted as geometric metal marks; pending.')
 else:row['reason']='Source complete outer cuts and single opening remain constrained by byte-exact native sourceA; source straight internal black cuts/stem/right field retain major positions. Ready only for coordinator review; longer lightstripe strengthens and slight finepaint grain persists, maxlocal+40 in detail metrics.'
 rows.append(row)
(BASE/'pattern-metrics-equipment-twentieth.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Saved2 private pattern diagnostics; no image edits.')

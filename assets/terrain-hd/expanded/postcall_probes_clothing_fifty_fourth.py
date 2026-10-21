from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-fourth';i=4556
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
s=Image.open(B/f'original/{i}.png').convert('RGB');s4=s.resize((512,512),Image.Resampling.LANCZOS);rnd=s4.resize(s.size,Image.Resampling.LANCZOS)
p=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB').resize(s.size,Image.Resampling.LANCZOS)
u=Image.open(B/f'private-{ST}/postcall-aliases/{i}-resource-{i|0x01000000}-native-opaque-storedRGB.png').convert('RGB')
d=read(B/f'detail-metrics-{ST}.json')[0]
coords=[(61,94),(64,107),(56,99),(69,102),(67,92),(71,96),(71,97),(35,73),(90,73),(66,82),(11,65),(36,74),(0,127)]
rows=[{'xy':list(xy),'compressed_original_RGB':list(s.getpixel(xy)),'uncompressed_alias_RGB':list(u.getpixel(xy)),'compressed_natural_RGB_LANCZOS4x_inverse_RGB':list(rnd.getpixel(xy)),'production_whole_RGB_inverse_RGB':list(p.getpixel(xy))}for xy in coords]
o={'id':i,'phase':'POSTCALL diagnostic; BEFORE coordinates and sourceclass definitions unchanged. Numeric probes are pigment positions, not inferred physical parts. Alternative alias has same A but different RGB; production scalar diagnostic remains calibrated against compressed source only.','source_before_file_SHA256':sha(B/f'source-check-{ST}-before-call.json'),'alias_proof_SHA256':sha(B/f'postcall-alias-proof-{ST}.json'),'literal_source_probes':rows,'local_strength_detail_ref':{'path':(B/f'detail-metrics-{ST}.json').relative_to(ROOT).as_posix(),'file_SHA256':sha(B/f'detail-metrics-{ST}.json')},'no_artist_RGB_repair':True}
(B/f'postcall-source-specific-probes-{ST}.json').write_text(json.dumps(o,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print({'probes':len(rows),'source_peak':d['source_peak_xy']})

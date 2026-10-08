from PIL import Image,ImageDraw
from pathlib import Path
import json,numpy as np
B=Path(__file__).resolve().parent
rows=json.loads((B/'source-check-heads-sixteenth.json').read_text(encoding='utf-8'))['source_checks']
out=B/'private-heads-sixteenth/source-alpha';out.mkdir(parents=True,exist_ok=True)
c=Image.new('RGB',(4*260,3*280));d=ImageDraw.Draw(c)
for k,r in enumerate(rows):
 i=r['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');a=o.getchannel('A').resize((256,256),Image.Resampling.NEAREST)
 a.save(out/f'{i}-source-alpha-nearest.png');c.paste(a.convert('RGB'),((k%4)*260,(k//4)*280+20));d.text(((k%4)*260,(k//4)*280),str(i),fill='white')
c.save(out/'whole-source-alpha-survey.png')
print('Native lowerleft RGBA',[(i,np.array(Image.open(B/f'original/{i}.png').convert('RGBA'))[127,0].tolist())for i in [6698,6719,6720]])

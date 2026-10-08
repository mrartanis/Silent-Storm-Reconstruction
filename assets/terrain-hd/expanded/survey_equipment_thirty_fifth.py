from pathlib import Path
import json
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent
rows=json.loads((B/'remaining-equipment-thirty-fifth.json').read_text(encoding='utf-8'))
out=B/'private-equipment-thirty-fifth/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(rows),15):
 part=rows[start:start+15];canvas=Image.new('RGB',(5*270,3*290),(30,30,30));draw=ImageDraw.Draw(canvas)
 for k,r in enumerate(part):
  o=Image.open(B/f'original/{r["id"]}.png').convert('RGBA');im=o.convert('RGB').resize((256,256),Image.Resampling.NEAREST)
  im.save(out/f'{r["id"]}-whole-source-rgb-nearest.png')
  x=(k%5)*270;y=(k//5)*290;canvas.paste(im,(x,y+25));draw.text((x+2,y+3),f'{r["id"]} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 canvas.save(out/f'whole-sources-{start}.png')
print('Private complete source RGB survey sheets',len(rows))


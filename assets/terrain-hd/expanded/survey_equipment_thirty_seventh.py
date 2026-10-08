from pathlib import Path
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;IDS=[5238,5239,5240,5241,5242,5245,5246,5247,5250,5251,5252,6668]
out=B/'private-equipment-thirty-seventh/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(IDS),6):
 c=Image.new('RGB',(3*530,2*300),(30,30,30));d=ImageDraw.Draw(c)
 for k,i in enumerate(IDS[start:start+6]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');scale=256//max(o.size);im=o.convert('RGB').resize((o.width*scale,o.height*scale),Image.Resampling.NEAREST);im.save(out/f'{i}-whole-source-rgb-nearest.png');x=(k%3)*530;y=(k//3)*300;c.paste(im,(x,y+24));d.text((x,y),f'{i} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 c.save(out/f'whole-sources-{start}.png')

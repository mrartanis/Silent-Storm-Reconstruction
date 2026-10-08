from pathlib import Path
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;IDS=[675, 1914, 1918, 1977, 2213, 3035, 3039, 3042, 3384, 3783, 3914, 5677];ST='equipment-fiftieth'
out=B/f'private-{ST}/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(IDS),6):
 c=Image.new('RGB',(3*290,2*300),(30,30,30));d=ImageDraw.Draw(c)
 for k,i in enumerate(IDS[start:start+6]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');scale=256/max(o.size);im=o.convert('RGB').resize((int(o.width*scale),int(o.height*scale)),Image.Resampling.NEAREST);im.save(out/f'{i}-whole-source-rgb-nearest.png');x=(k%3)*290;y=(k//3)*300;c.paste(im,(x,y+30));d.text((x,y),f'{i} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 c.save(out/f'whole-sources-{start}.png')

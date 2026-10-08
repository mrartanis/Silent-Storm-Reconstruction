from pathlib import Path
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;IDS=[4086, 4089, 4092, 4096, 4097, 4101, 2626, 2627, 2628, 2629, 5391, 5392]
out=B/'private-effects-twenty-seventh/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(IDS),6):
 c=Image.new('RGB',(3*270,2*290),(30,30,30));d=ImageDraw.Draw(c)
 for k,i in enumerate(IDS[start:start+6]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');im=o.convert('RGB').resize((256,256),Image.Resampling.NEAREST);im.save(out/f'{i}-whole-source-rgb-nearest.png');x=(k%3)*270;y=(k//3)*290;c.paste(im,(x,y+24));d.text((x,y),f'{i} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 c.save(out/f'whole-sources-{start}.png')

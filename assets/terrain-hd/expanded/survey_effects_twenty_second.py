from pathlib import Path
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;IDS=[1078,2485,2118,2575,2577,4739,4744,5261,5391,5392,2523,4780,4782,4783,4804,4805,4915,5298,5304,5699,5935,6676,7682,3095]
out=B/'private-effects-twenty-second/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(IDS),8):
 canvas=Image.new('RGB',(4*270,2*290),(30,30,30));d=ImageDraw.Draw(canvas)
 for k,i in enumerate(IDS[start:start+8]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');im=o.convert('RGB').resize((256,256),Image.Resampling.NEAREST);im.save(out/f'{i}-whole-source-rgb-nearest.png');x=(k%4)*270;y=(k//4)*290;canvas.paste(im,(x,y+25));d.text((x+2,y+3),f'{i} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 canvas.save(out/f'whole-sources-{start}.png')
print('Private complete source RGB survey; no calls')

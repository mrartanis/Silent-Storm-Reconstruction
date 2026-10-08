from pathlib import Path
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;IDS=[3039,3040,3914,5331,5333,5372,5379,5383,5386,5390,5402,5407]
out=B/'private-equipment-thirty-sixth/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(IDS),8):
 canvas=Image.new('RGB',(4*270,2*290),(30,30,30));d=ImageDraw.Draw(canvas)
 for k,i in enumerate(IDS[start:start+8]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');im=o.convert('RGB').resize((256,256),Image.Resampling.NEAREST);im.save(out/f'{i}-whole-source-rgb-survey-affine-square.png');x=(k%4)*270;y=(k//4)*290;canvas.paste(im,(x,y+25));d.text((x+2,y+3),f'{i} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 canvas.save(out/f'extra-whole-sources-{start}.png')
print('Private entire-canvas survey only, no call references or artchanges')

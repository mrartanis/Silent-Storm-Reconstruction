from pathlib import Path
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent
IDS=[6692,6695,6697,6702,6703,6704,6709,6710,6711,6712,6713,6714,6717,6718,6719,6720,6721,6722,6723,6780]
out=B/'private-heads-fifteenth/survey';out.mkdir(parents=True,exist_ok=True)
for start in range(0,len(IDS),10):
 canvas=Image.new('RGB',(5*270,2*290),(30,30,30));d=ImageDraw.Draw(canvas)
 for k,i in enumerate(IDS[start:start+10]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');im=o.convert('RGB').resize((256,256),Image.Resampling.NEAREST);im.save(out/f'{i}-whole-source-rgb-nearest.png');x=(k%5)*270;y=(k//5)*290;canvas.paste(im,(x,y+25));d.text((x+2,y+3),f'{i} {o.size} A{o.getchannel("A").getextrema()}',fill='white')
 canvas.save(out/f'whole-sources-{start}.png')
print('Private full-canvas originals survey only')

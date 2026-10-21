from pathlib import Path
import subprocess,json,sys
B=Path(__file__).resolve().parent;ROOT=B.parents[2]
ids=[916,1879,1883,1887,1891,2006]
desc=('This is an UPSIDE-DOWN STORED FACE UV diffuse texture, not a portrait. Preserve the exact current orientation and whole square canvas. The single existing male face stays upside down in the lower-center region, with the very same identity, hair color, age, expression, pale eye marks, nose, short closed mouth mark, ear outlines and diffuse cheek shading. Preserve the existing jagged three-part brown hair silhouette along the bottom edge. Preserve the separate ear and FOUR-band hand paint islands along the top edge and the exact black gaps; never assemble them into a new person or rearrange the atlas. Keep the original restrained old game PAINTED shading. Add only delicate painted definition to existing features, without photographic pores, eyelashes, new teeth, beard hairs, bright eye whites, new expression or anatomy. Keep weak facial details weak and the mouth closed; do not mirror, rotate or beautify the face. The source tiny islands and clipped borders remain clipped.')
for i in ids:
 subprocess.run([sys.executable,str(B/'work_clothing_complete_20261008.py'),'prepare',str(i),'v1',desc],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
(B/'face-generation-clothing-complete-20261008-batch1.json').write_text(json.dumps(ids)+'\n',encoding='utf-8')
print('Prepared six separately inspected selected-native face edit requests',ids)

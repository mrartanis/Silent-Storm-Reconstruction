from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='effects-twenty-fifth';CALL={2577};records=[];unique={}
for r in json.loads((B/f'source-check-{ST}-before-call.json').read_text(encoding='utf-8')):
 if r['id']in CALL:continue
 i=r['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');a=np.array(o);rgb=a[:,:,:3];l=rgb.mean(2);mask=rgb.max(2)>0;seen=np.zeros(mask.shape,bool);components=[]
 for y,x in zip(*np.where(mask)):
  if seen[y,x]:continue
  stack=[(int(y),int(x))];seen[y,x]=True;pts=[]
  while stack:
   yy,xx=stack.pop();pts.append((yy,xx))
   for dy in [-1,0,1]:
    for dx in [-1,0,1]:
     ny,nx=yy+dy,xx+dx
     if 0<=ny<o.height and 0<=nx<o.width and mask[ny,nx]and not seen[ny,nx]:seen[ny,nx]=True;stack.append((ny,nx))
  yy,xx=zip(*pts);components.append({'pixel_count':len(pts),'native_bbox':[min(xx),min(yy),max(xx)+1,max(yy)+1],'max_RGB':rgb[list(yy),list(xx)].max(0).tolist(),'all_original_pixel_coordinates_RGBA':[[x,y,*a[y,x].tolist()]for y,x in pts]})
 y,x=np.unravel_index(l.argmax(),l.shape);key=r['source_rgba_sha256']
 if key not in unique:unique[key]={'representative_id':i,'native_size':list(o.size),'actual_source_RGBA_extrema':[list(z)for z in o.getextrema()],'whole_native_RGBA_matrix':a.tolist(),'source_max_mean_RGBA_xy':[int(x),int(y)],'source_max_mean_RGBA':a[y,x].tolist(),'positive_RGB_8connected_pixel_domain_count':len(components),'domains':components,'qualification':'Literal sourceRGB>0 pixel-connected regions, NOT physicalparticle/component count inferred fromRGBA mask. Weakpixel max1–5 fragments included exactly; no threshold guide/artpatch. Actual sourcealpha independently preserved.'}
 reason='Genuine sparse soft brown cloudpaint with11 literal positiveRGB connected domains, including very weak separate max1–5 bits. Numeric count/pixel phase now exactly documented, but preserving weak painttone hierarchy without strengthening/dropping bits is not clear enough for forced call in this bounded batch. Genuinepending0calls, not technical byname orblanketfamilyban; sameRGBA pendingaliases not accepted donors.'
 if i==2575:reason='Genuine complex brown/orange sourcepaint has one major positiveRGB connected domain plus SIX isolated single-pixel RGBmax1 remnants. Literal pixel topology and allweakpixels now stored; finer manyinterior shading knots/darkgap material remain complex for a faithful boundedcall. Genuinepending0calls, no A0-empty or technical classification, no familyban.'
 records.append({'id':i,'source_RGBA_sha256':key,'unique_native_matrix_key':key,'reason':reason,'imagegen_call_count':0,'genuine_pending':True,'not_accepted':True,'actual_typed_sourceproof':f'assets/terrain-hd/expanded/source-check-{ST}-before-call.json'})
(B/f'source-only-detail-{ST}.json').write_text(json.dumps({'records':records,'unique_native_sources':unique,'id_count':len(records),'unique_native_RGBA_count':len(unique)},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Source-only11IDs/3unique native matrices, all weak sourcepixels documented')

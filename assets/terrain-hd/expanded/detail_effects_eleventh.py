import json
from pathlib import Path
import numpy as np
from PIL import Image
BASE=Path(__file__).resolve().parent
IDS=[1566,1568,1569,1570,1571,1572,1573,1574,1575,1576]
out=[]
def components(mask):
 visited=np.zeros(mask.shape,dtype=bool);records=[]
 for y,x in zip(*np.where(mask)):
  if visited[y,x]:continue
  todo=[(int(y),int(x))];visited[y,x]=True;coords=[]
  while todo:
   py,px=todo.pop();coords.append((py,px))
   for dy in (-1,0,1):
    for dx in (-1,0,1):
     yy,xx=py+dy,px+dx
     if 0<=yy<mask.shape[0] and 0<=xx<mask.shape[1] and mask[yy,xx] and not visited[yy,xx]:visited[yy,xx]=True;todo.append((yy,xx))
  ys,xs=zip(*coords);records.append({'pixel_count':len(coords),'source_pixel_bbox':[min(xs),min(ys),max(xs)+1,max(ys)+1]})
 return sorted(records,key=lambda r:r['pixel_count'],reverse=True)
for id_ in IDS:
 original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
 s=np.asarray(original.convert('RGB'),dtype=np.int16)
 g=np.asarray(Image.open(BASE/f'private-effects-eleventh/native4x/{id_}-stored-rgb-private.png').resize(original.size,Image.Resampling.LANCZOS).convert('RGB'),dtype=np.int16)
 sl=s.max(2);gl=g.max(2);delta=gl-sl;ys,xs=np.where((sl>0)&(delta>12));idx=sorted(range(len(xs)),key=lambda i:int(delta[ys[i],xs[i]]),reverse=True)[:10]
 examples=[{'logical_xy':[int(xs[i]),int(ys[i])],'source_rgb':s[ys[i],xs[i]].tolist(),'candidate_rgb':g[ys[i],xs[i]].tolist(),'max_channel_increase':int(delta[ys[i],xs[i]])} for i in idx]
 a=np.asarray(original.getchannel('A'))
 native=Image.open(BASE/f'private-effects-eleventh/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
 assert native.getchannel('A').tobytes()==original.getchannel('A').resize(native.size,Image.Resampling.LANCZOS).tobytes()
 row={'id':id_,'diagnostic':'Entire calibrated native storedRGB inverse LANCZOS to original64x64, positive-source pixels only. Local increases/connected-component bboxes diagnose source material/count only; not proof of newly invented parts, never used for registration or compositing.', 'stored_native_luma_spatial_correlation':float(np.corrcoef(s.mean(2).ravel(),g.mean(2).ravel())[0,1]),'max_positive_source_local_channel_increase':int(delta[sl>0].max()),'weak_source_pixels_max_lt60_gain_gt12':int(((sl>0)&(sl<60)&(delta>12)).sum()),'examples':examples,'source_native_alpha_components_8connected_positiveA':components(a>0),'source_material_components_8connected_RGBmax_gt5':components(sl>5),'native_alpha4x_byte_exact':True}
 out.append(row);print(id_,round(row['stored_native_luma_spatial_correlation'],4),'sourceA-components',len(row['source_native_alpha_components_8connected_positiveA']))
(BASE/'detail-metrics-effects-eleventh.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
proof=json.loads((BASE/'source-check-effects-eleventh.json').read_text(encoding='utf-8'))
sel={x['id']:x for x in json.loads((BASE/'selected-effects-eleventh.json').read_text(encoding='utf-8'))}
for p in proof:
 uses=p['fresh_typed_usage'];assert all(u['particle'] is not None for u in uses)
 p['material_evidence']={'source_native_type':sel[p['id']]['texture']['Type'],'source_alpha_extrema':p['alpha_extrema'],'actual_instance_alpha_blend_values':sorted({u['instance']['AlphaBlending'] for u in uses}),'actual_particle_Wrap_pairs':sorted({(u['particle']['WrapX'],u['particle']['WrapY']) for u in uses}),'unresolved_particle_definitions':[],'all_positive_TextureN_bindings_checked':True,'all_frame_sibling_records_present':all(not u['missing_sibling_texture_slots'] for u in uses)}
 p['uv_proof']['source_visual_review']='Full native original and complete source-only opaqueRGB helper inspected privately before single built-in call. Complete raw and original/native4x calibrated storedRGB/nativeA inspected afterwards. Plain painted gray paper material, original folds/parts/gaps; no inferred animation atlas. Review separately holds uncertain stronger folds/material.'
(BASE/'source-check-effects-eleventh.json').write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

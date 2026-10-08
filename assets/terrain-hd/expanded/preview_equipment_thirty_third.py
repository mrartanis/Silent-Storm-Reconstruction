import json,sys
from pathlib import Path
import numpy as np
from PIL import Image,ImageChops
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
sys.path.insert(0,str(BASE.parent))
from match_brightness import match_cell,stored_luma,pad_rgb_under_source_mask
OUT=BASE/'private-equipment-thirty-third/native4x'
OUT.mkdir(parents=True,exist_ok=True)
IDS=[5387, 5408, 5438]
metrics=[]
types={x['id']:x['texture']['Type'] for x in json.loads((BASE/'selected-equipment-thirty-third.json').read_text(encoding='utf-8'))}
for id_ in IDS:
    if not (BASE/f'generated/{id_}-raw.png').exists():continue
    premultiply=types[id_]=='Transparent'
    original=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    raw=Image.open(BASE/f'generated/{id_}-raw.png').convert('RGBA')
    size=(original.width*4,original.height*4)
    imported=raw.resize(size,Image.Resampling.LANCZOS)
    rgb=imported.convert('RGB')
    alpha=original.getchannel('A').resize(size,Image.Resampling.LANCZOS)
    rgba=rgb.convert('RGBA');rgba.putalpha(alpha)
    rgba.save(OUT/f'{id_}-native-alpha-normalized.png')
    rgb.save(OUT/f'{id_}-rgb-normalized.png')
    # The root packer premultiplies Transparent textures only; source RGB is
    # already premultiplied. Preview the stored RGB using the same pure scalar
    # calibration function, without invoking metadata/pack writes.
    padded,padded_count,cleared_count=pad_rgb_under_source_mask(imported,alpha)
    corrected,gain_native,actual,rounding=match_cell(padded,stored_luma(original),premultiply,alpha)
    corrected.putalpha(alpha)
    corrected.save(OUT/f'{id_}-calibrated-native-alpha-private.png')
    rr,gg,bb,aa=corrected.split()
    if premultiply:rr,gg,bb=[ImageChops.multiply(c,aa) for c in (rr,gg,bb)]
    Image.merge('RGB',(rr,gg,bb)).save(OUT/f'{id_}-stored-rgb-private.png')
    src=original.convert('RGB').resize(size,Image.Resampling.LANCZOS)
    src.save(OUT/f'{id_}-source-rgb-native4x-private.png')
    original.resize(size,Image.Resampling.LANCZOS).save(OUT/f'{id_}-source-rgba-native4x-private.png')
    s=np.asarray(src,dtype=float);g=np.asarray(rgb,dtype=float)
    weight=np.asarray(alpha,dtype=float)/255
    if not weight.any():weight=np.ones(size[::-1])
    sw=(s.mean(2)*weight).sum();gw=(g.mean(2)*weight).sum()
    gain=float(sw/max(gw,1))
    adjusted=np.clip(g*gain,0,255).round().astype('uint8')
    adjusted_image=Image.fromarray(adjusted,'RGB')
    if weight.min()==0 and weight.max()>0:
        adjusted_image=adjusted_image.convert('RGBA');adjusted_image.putalpha(alpha)
    adjusted_image.save(OUT/f'{id_}-gain-preview.png')
    l1=s.mean(2).ravel();l2=g.mean(2).ravel()
    metrics.append({'id':id_,'raw_size':list(raw.size),'target_4x_size':list(size),
       'preview_luma_gain':gain,'luma_spatial_correlation':float(np.corrcoef(l1,l2)[0,1]),
       'native_alpha_restored':True,'bbox_registration':False,
       'native_packer_premultiplies':premultiply,'native_preview_luma_gain':gain_native,'native_preview_stored_luma':actual,'native_preview_rounding':rounding,
       'root_padding_texels':padded_count,'root_hidden_clear_texels':cleared_count,
       'normalization_recipe':'Full raw RGBA Lanczos to exact original4x dimensions, matching importer; original alpha Lanczos4x, matching packer. Existing pure pad_rgb_under_source_mask + match_cell scalar calibration, then original native premultiplication in PRIVATE previews only. No crop/reposition/rotate/source artistic RGB insertion. Root calibrates final.',
       'normalized_preview':f'assets/terrain-hd/expanded/private-equipment-thirty-third/native4x/{id_}-native-alpha-normalized.png',
       'rgb_preview':f'assets/terrain-hd/expanded/private-equipment-thirty-third/native4x/{id_}-rgb-normalized.png',
       'private_gain_preview':f'assets/terrain-hd/expanded/private-equipment-thirty-third/native4x/{id_}-gain-preview.png',
       'stored_native_rgb_preview':f'assets/terrain-hd/expanded/private-equipment-thirty-third/native4x/{id_}-stored-rgb-private.png'})
(BASE/'metrics-equipment-thirty-third.json').write_text(json.dumps(metrics,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print([(x['id'],round(x['luma_spatial_correlation'],4),round(x['preview_luma_gain'],3)) for x in metrics])

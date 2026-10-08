from pathlib import Path
B=Path(__file__).resolve().parent
old='heads-nineteenth';new='heads-twentieth'
ids='[6692,6697,6699,6700,6718,6721,6722,6772,6773,6774,6775,6776]'
s=(B/'audit_heads_nineteenth.py').read_text(encoding='utf-8').replace(old,new).replace('[6691,6695,6697,6699,6700,6701,6717,6718,6721,6722,6780,6781]',ids)
s=s.replace("scale=512//max(o.size);hp=B/f'generated/{i}-{ST}-source-rgb-support.png';assert not hp.exists();o.convert('RGB').resize((o.width*scale,o.height*scale),Image.Resampling.NEAREST).save(hp)","scale=512//max(o.size);hp=B/f'generated/{i}-{ST}-source-rgb-native.png';assert not hp.exists() or Image.open(hp).convert('RGB').tobytes()==o.convert('RGB').tobytes();o.convert('RGB').save(hp);guidep=B/f'private-{ST}/survey/{i}-source-rgb-nearest512.png';o.convert('RGB').resize((o.width*scale,o.height*scale),Image.Resampling.NEAREST).save(guidep)")
s=s.replace("f'ENTIRE original storedRGBA→opaqueRGB NEAREST{scale}x only, no crop/padding/rotate/fit/BBox/unpremultiply/artistRGB; entire fullsourceA separately restored.'","'ENTIRE original native storedRGBA→opaqueRGB only, NO enlargement for tool target. Private NEAREST512 view proves native coordinates only, never tool reference. No crop/padding/rotate/fit/BBox/unpremultiply/artistRGB; entire fullsourceA separately restored.'")
s=s.replace("'helper_size':[o.width*scale,o.height*scale],'helper_bbox':[0,0,o.width*scale,o.height*scale]","'helper_size':list(o.size),'helper_bbox':[0,0,o.width,o.height],'private_NN_coordinate_view':rel(guidep)")
(B/'audit_heads_twentieth.py').write_text(s,encoding='utf-8')
for name in ['save_heads_nineteenth_call.py','precall_guard_heads_nineteenth.py','preview_heads_nineteenth.py','material_phase_heads_nineteenth.py','detail_heads_nineteenth.py','postcall_material_heads_nineteenth.py']:
 s=(B/name).read_text(encoding='utf-8').replace(old,new).replace('CALL=[6695,6717]','CALL=[6697,6700,6722]')
 (B/name.replace('nineteenth','twentieth')).write_text(s,encoding='utf-8')
print('Own Heads20 cloned compact audit/native RGB tool target and private NN only; old scopes untouched')

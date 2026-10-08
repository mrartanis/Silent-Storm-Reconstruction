from pathlib import Path
B=Path(__file__).resolve().parent
for n in ['save_effects_thirtieth_call.py','precall_guard_effects_thirtieth.py','preview_effects_thirtieth.py','material_phase_effects_thirtieth.py','detail_effects_thirtieth.py','weakpixel_effects_thirtieth.py']:
 s=(B/n).read_text(encoding='utf-8').replace('effects-thirtieth','heads-nineteenth').replace('[4101,2575]','[6695,6717]')
 if n=='preview_effects_thirtieth.py':
  s=s.replace('imported=raw.resize(size,Image.Resampling.LANCZOS)',"assert raw.getchannel('A').getextrema()==(255,255), 'Opaque RGB guide edit unexpectedly nonopaque; do not bypass default padding'\n    imported=raw.convert('RGB').resize(size,Image.Resampling.LANCZOS).convert('RGBA')")
  s=s.replace("original.resize(size,Image.Resampling.LANCZOS).save(OUT/f'{id_}-source-rgba-native4x-private.png')","source_rgba=src.convert('RGBA');source_rgba.putalpha(alpha);source_rgba.save(OUT/f'{id_}-source-rgba-native4x-private.png')")
  s=s.replace('Full raw RGBA Lanczos to exact original4x dimensions, matching importer','Full opaque raw RGB-FIRST Lanczos original4x, equivalent importer opaqueRGBA; sourceRGB and sourceA resample separately, no implicit Pillow premul')
 (B/n.replace('effects_thirtieth','heads_nineteenth')).write_text(s,encoding='utf-8')
print('Own Heads19 scripts; no frozen scopes changed')

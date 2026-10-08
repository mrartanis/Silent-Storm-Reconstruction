from pathlib import Path
B=Path(__file__).resolve().parent;old='equipment-fortieth';new='equipment-forty-first'
s=(B/'audit_equipment_fortieth.py').read_text(encoding='utf-8').replace(old,new).replace('[5237,5382,5383,5235,5236,5239,5243,5244,5372,5431,675,6902]','[5431,5329,5333,5235,5236,5239,5243,5244,3384,6901,6902,2213]');(B/'audit_equipment_forty_first.py').write_text(s,encoding='utf-8')
for name in ['save_equipment_fortieth_call.py','precall_guard_equipment_fortieth.py','preview_equipment_fortieth.py','material_phase_equipment_fortieth.py','detail_equipment_fortieth.py','postcall_material_equipment_fortieth.py']:
 s=(B/name).read_text(encoding='utf-8').replace(old,new).replace('[5237,5383]','[5431,5333]');(B/name.replace('fortieth','forty_first')).write_text(s,encoding='utf-8')
print('OwnEq41 compactactualMat/nativeopaqueRGB tool,oldstableunchanged')

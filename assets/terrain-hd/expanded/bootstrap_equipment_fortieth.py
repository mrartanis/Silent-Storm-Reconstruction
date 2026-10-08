from pathlib import Path
B=Path(__file__).resolve().parent;old='heads-twenty-first';new='equipment-fortieth';IDS='[5237,5382,5383,5235,5236,5239,5243,5244,5372,5431,675,6902]'
s=(B/'audit_heads_twenty_first.py').read_text(encoding='utf-8').replace(old,new).replace('[3322,5259,6691,6699,6701,6718,6721,6772,6773,6781,6782,6921]',IDS)
s=s.replace('Heads21','Equipment40').replace('Heads19','Equipment40').replace('Dynamic THMID/Heads actual bound; static flag/meshUV/runtime/weights not invented. Root owns actual runtime/native acceptance.','Actual Material source/template/model roles proven whenpresent; no meshUV/runtime/semantic hardware interpretation inferred. Root native/runtime finalacceptance.')
s=s.replace('Actual Materials.TextureID diffuse/template consumers; static hair/head only when exact linked records prove it. Runtime/model meshUV unknown, no fake model or dynamic layer inference.','Actual Materials.TextureID diffuse/TemplateID/Models.MaterialN roles proven from freshDB; geometries/modeltemplates exact. Runtime/modelmeshUV/physicalhardware interpretation unknown, no invented dynamicHead proof.')
s=s.replace('Entire originalcanvas UV retained for source layer. Dynamic actual LSHead blending consumes whole native source; no false staticHead/hair proof or inferred anatomical correspondence.','Entire originalcanvas registration/count/nativepaintscale retained as image replacement. ActualMatTemplates/Models diffuse roles recorded; mesh/runtime UV unknown, no automaticBBox/crop/fitting or inferred hardware.')
needle="assert dynamic or static"
extra="""assert dynamic or static
 queued_mats={u['id']for u in r['usage']['direct_texture_uses']if u['table']=='Materials'};actual_mats={v['ID']for v in T['Materials'].values()if v['TextureID']==i};assert queued_mats==actual_mats
 for u in r['usage']['direct_texture_uses']:
  assert u['table']=='Materials', ('unexpected_equipment_direct_consumer',i,u['table'])
  v=T['Materials'][u['id']];assert all(v[k]==u[k]for k in ['Alpha','AddressMode','TemplateID'])
  actual_models={(v['ID'],f)for v in T['Models'].values()for f,x in v.items()if re.fullmatch(r'Material\\d+',f)and x==u['TemplateID']};assert actual_models=={(c['id'],c['field'])for c in u['consumers']}
  for c in u['consumers']:
   v=T['Models'][c['id']];g=T['Geometries'][v['GeometryID']];assert v[c['field']]==u['TemplateID']==c['material_template_id'];assert v['GeometryID']==c['geometry_id']and v['TemplateID']==c['model_template_id']and g['SrcName']==c['geometry']and v['Flags']==c['flags']
"""
s=s.replace(needle,extra)
(B/'audit_equipment_fortieth.py').write_text(s,encoding='utf-8')
for name in ['save_heads_twenty_first_call.py','precall_guard_heads_twenty_first.py','preview_heads_twenty_first.py','material_phase_heads_twenty_first.py','detail_heads_twenty_first.py','postcall_material_heads_twenty_first.py']:
 s=(B/name).read_text(encoding='utf-8').replace(old,new).replace('[3322,5259,6691]','[5237,5382,5383]')
 (B/name.replace('heads_twenty_first','equipment_fortieth')).write_text(s,encoding='utf-8')
print('Own Equipment40 compactactualmat/model/nativewholeRGB reference,oldscopesuntouched')

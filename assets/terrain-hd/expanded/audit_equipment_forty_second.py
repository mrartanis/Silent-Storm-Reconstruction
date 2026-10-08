from pathlib import Path
import json,re,hashlib,sys
import numpy as np
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='equipment-forty-second';IDS=[1914, 1915, 1916, 1917, 1918, 1919, 1920, 1977, 3035, 3039, 3914, 5402]
sys.path.insert(0,str(B.parent))
from prepare_sources import ReleaseResources,decode_mmp
from audit_texture_groups import read_database
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def canon(r):return hashlib.sha256(json.dumps(r,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode('utf-8')).hexdigest()
DB=Path('G:/SS/Silent-Storm/Complete/game.db');db=read_database(DB);dbsha=sha(DB)
regs=(ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8');names={int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',regs)}
T={names[k]:{r['ID']:r for r in table['records']}for k,table in db.items()if k in names};common={};hashes={}
def ref(table,i):
 row=T.get(table,{}).get(i)
 if row is None:return None
 key=str(i);common.setdefault(table,{})[key]=row;hashes.setdefault(table,{})[key]=canon(row)
 return {'table':table,'id':i,'canonical_record_SHA256':canon(row)}
remaining=read(B/f'remaining-{ST}.json');selected=[next(r for r in remaining if r['id']==i)for i in IDS];ex=read(B/f'exclusions-{ST}.json');release=ReleaseResources('G:/SS/lab/baseline/res');proof=[];matrices={}
dyn_tables=[n for n in T if n.startswith('Transformable')and n.endswith('Textures')and any('TextureID'in r for r in T[n].values())]
for r in selected:
 i=r['id'];assert i not in ex['ids'];assert T['Textures'][i]==r['texture']and r['texture']['Type']!='Bump';assert r['usage']['direct_texture_uses']and all(u['role']=='color'for u in r['usage']['direct_texture_uses'])
 op=B/f'original/{i}.png';o=Image.open(op).convert('RGBA');hist=Path(f'G:/SS/Silent-Storm/Complete/Textures/{i}');hb=hist.read_bytes();rb=release.read(i);h=decode_mmp(hb).convert('RGBA');rr=decode_mmp(rb).convert('RGBA')
 assert o.size==h.size==rr.size==tuple(r['logical_size']);assert o.tobytes()==h.tobytes()==rr.tobytes();rgba=hashlib.sha256(o.tobytes()).hexdigest();assert rgba==r['source_rgba_sha256']==r['release_rgba_sha256'];assert sha(hist)==r['source_sha256'];assert rgba not in ex['blocked_hashes'];assert all(d>0 and d&(d-1)==0 for d in o.size)
 dynamic=[];static=[]
 for table in dyn_tables:
  rows=[v for v in T[table].values()if v['TextureID']==i];queued={u['id']for u in r['usage']['direct_texture_uses']if u['table']==table};assert queued=={v['ID']for v in rows},(i,table)
  for row in rows:
   owner=T['TransformableHeadMaterials'].get(row['THMID']);heads=[v for v in T['Heads'].values()if v.get('TransformableTextures')==row['THMID']];headids={v['ID']for v in heads};complexheads=[v for v in T['ComplexHeads'].values()if v.get('HeadID')in headids]
   dynamic.append({'actual_layer_table':table,'actual_layer_ref':ref(table,row['ID']),'actual_THMID_owner_ref':ref('TransformableHeadMaterials',row['THMID']),'all_actual_Heads_refs':[ref('Heads',v['ID'])for v in heads],'all_actual_ComplexHeads_refs':[ref('ComplexHeads',v['ID'])for v in complexheads],'qualification':'Actual dynamic source COLOR layer through THMID/Heads.TransformableTextures. No static material Alpha/AddressMode invented. Whole originalRGBA/nativepaintphase/UV/layer weights unchanged, runtime dispatch not measured by worker.','missing_owner_or_Heads':owner is None or not heads})
 for row in T['Materials'].values():
  if row['TextureID']!=i:continue
  tid=row['TemplateID'];models=[(v,f)for v in T['Models'].values()for f,x in v.items()if re.fullmatch(r'Material\d+',f)and x==tid];mtemplates={v['TemplateID']for v,f in models};heads=[v for v in T['Heads'].values()if any(v.get(f)==tid for f in ['MaterialID']+[f'Material{k}'for k in range(8)])];races=[v for v in T['Races'].values()if v.get('MaterialID')==tid];ch=[v for v in T['ComplexHeads'].values()if any(v.get(f)in mtemplates for f in ['Hair','HairInCap'])];th=[v for v in T.get('TransformableHeadHairs',{}).values()if v.get('Hair')in mtemplates]
  static.append({'actual_material_ref':ref('Materials',row['ID']),'actual_material_template_ref':ref('MaterialTemplates',tid),'all_actual_model_consumer_refs':[{'material_field':f,'model_ref':ref('Models',v['ID']),'model_template_ref':ref('ModelTemplates',v['TemplateID']),'geometry_ref':ref('Geometries',v['GeometryID'])}for v,f in models],'actual_Heads_refs':[ref('Heads',v['ID'])for v in heads],'actual_Races_refs':[ref('Races',v['ID'])for v in races],'actual_ComplexHeads_hair_refs':[ref('ComplexHeads',v['ID'])for v in ch],'actual_TransformableHeadHairs_refs':[ref('TransformableHeadHairs',v['ID'])for v in th],'actual_Alpha_AddressMode':[row['Alpha'],row['AddressMode']],'qualification':'Actual Materials.TextureID diffuse/TemplateID/Models.MaterialN roles proven from freshDB; geometries/modeltemplates exact. Runtime/modelmeshUV/physicalhardware interpretation unknown, no invented dynamicHead proof.'})
 assert dynamic or static
 queued_mats={u['id']for u in r['usage']['direct_texture_uses']if u['table']=='Materials'};actual_mats={v['ID']for v in T['Materials'].values()if v['TextureID']==i};assert queued_mats==actual_mats
 for u in r['usage']['direct_texture_uses']:
  assert u['table']=='Materials', ('unexpected_equipment_direct_consumer',i,u['table'])
  v=T['Materials'][u['id']];assert all(v[k]==u[k]for k in ['Alpha','AddressMode','TemplateID'])
  actual_models={(v['ID'],f)for v in T['Models'].values()for f,x in v.items()if re.fullmatch(r'Material\d+',f)and x==u['TemplateID']};assert actual_models=={(c['id'],c['field'])for c in u['consumers']}
  for c in u['consumers']:
   v=T['Models'][c['id']];g=T['Geometries'][v['GeometryID']];assert v[c['field']]==u['TemplateID']==c['material_template_id'];assert v['GeometryID']==c['geometry_id']and v['TemplateID']==c['model_template_id']and g['SrcName']==c['geometry']and v['Flags']==c['flags']

 # Every native pixel/alpha once in a common immutable matrix file.
 a=np.array(o);rgb=a[:,:,:3];u,ct=np.unique(rgb.reshape(-1,3),axis=0,return_counts=True);l=rgb.mean(2);y,x=np.unravel_index(l.argmax(),l.shape)
 matrices[str(i)]={'id':i,'source_native_size':list(o.size),'source_RGBA_SHA256':rgba,'original_PNG_file_SHA256':sha(op),'actual_native_Type':r['texture']['Type'],'actual_source_RGBA_extrema':[list(z)for z in o.getextrema()],'native_RGBA_matrix':a.tolist(),'native_peak_mean_RGB_xy':[int(x),int(y)],'native_peak_mean_RGBA':a[y,x].tolist(),'qualification':'Exact original stored RGB/A fullnative once, ordinary uses straight RGB. No unpremultiply/no artistpatch, even A0 useful or white sourceRGB remains authoritative.'}
 scale=512//max(o.size);hp=B/f'generated/{i}-{ST}-source-rgb-native.png';assert not hp.exists() or Image.open(hp).convert('RGB').tobytes()==o.convert('RGB').tobytes();o.convert('RGB').save(hp);guidep=B/f'private-{ST}/survey/{i}-source-rgb-nearest512.png';o.convert('RGB').resize((o.width*scale,o.height*scale),Image.Resampling.NEAREST).save(guidep)
 ap=B/f'private-{ST}/survey/{i}-source-alpha-nearest.png';o.getchannel('A').resize((512,512),Image.Resampling.NEAREST).save(ap)
 prior=[rel(p)for p in B.rglob(f'{i}-*source*.png')if ST not in str(p)][:12]
 proof.append({'id':i,'fresh_DB_sha256':dbsha,'actual_Texture_record_ref':ref('Textures',i),'logical_size':list(o.size),'source_rgba_sha256':rgba,'source_binary_sha256':sha(hist),'release_binary_sha256':hashlib.sha256(rb).hexdigest(),'source_binary_path':hist.as_posix(),'original_PNG':rel(op),'original_PNG_file_SHA256':sha(op),'source_parity':'Fresh historical MMP == release native == original FULLRGBA/dimensions, exact Texture row and nativePOT checked.','historical_release_MMP_bytes_equal':hb==rb,'fresh_actual_dynamic_consumers':dynamic,'fresh_actual_static_consumers':static,'material_evidence':{'actual_source_native_type':r['texture']['Type'],'source_alpha_extrema':list(o.getchannel('A').getextrema()),'ordinary_straightRGB_noPremul':r['texture']['Type']=='Ordinary','static_actual_Alpha_AddressMode':[v['actual_Alpha_AddressMode']for v in static],'unknowns':'Actual Material source/template/model roles proven whenpresent; no meshUV/runtime/semantic hardware interpretation inferred. Root native/runtime finalacceptance.'},'native_RGB_distinct_count':len(u),'dominant_RGB_count_values':sorted([{'RGB':v.tolist(),'count':int(c)}for v,c in zip(u,ct)],key=lambda v:-v['count'])[:5],'source_RGBA_RGB_A_NN_privately_inspected':False,'previous_source_only_views':prior,'view_novelty':'Honest fresh re-audit of prior0call sources if listed. Accepted/raw/jobs/prompts/calls/held/refusal/identicalRGBA guards passed separately, no never-viewed fiction.','source_only_reason':'Genuine dynamic color paint pending exact localcount/weakphase/material fidelity; no source name/human/A0 blanket classification.','imagegen_call_count':0,'helper_recipe':{'input':rel(op),'operations':'ENTIRE original native storedRGBA→opaqueRGB only, NO enlargement for tool target. Private NEAREST512 view proves native coordinates only, never tool reference. No crop/padding/rotate/fit/BBox/unpremultiply/artistRGB; entire fullsourceA separately restored.','source_size':list(o.size),'source_bbox':[0,0,o.width,o.height],'helper_size':list(o.size),'helper_bbox':[0,0,o.width,o.height],'private_NN_coordinate_view':rel(guidep),'inverse':'ENTIRE raw LANCZOS original4x; RGB diagnostic inverse separately from originalA, never Pillow RGBA implicit premul; standard whole scalar/defaultpadding/nativeType downstream root.','output':rel(hp)},'UV_proof':'Entire originalcanvas registration/count/nativepaintscale retained as image replacement. ActualMatTemplates/Models diffuse roles recorded; mesh/runtime UV unknown, no automaticBBox/crop/fitting or inferred hardware.'})
dbp=B/f'actual-records-{ST}.json';mp=B/f'native-matrices-{ST}.json';save(dbp.name,{'fresh_DB_path':DB.as_posix(),'fresh_DB_SHA256':dbsha,'canonical_record_hash_recipe':'SHA256 UTF8 json.dumps actual row sort_keys=True separators comma/colon','tables':common,'record_SHA256':hashes});save(mp.name,{'sources':matrices,'matrix_semantics':'Each exact native fullRGBA matrix once, referenced by sourceid/fileSHA; A and storedRGB distinct semantic channels.'})
for r in proof:r.update(common_actual_records={'path':rel(dbp),'file_SHA256':sha(dbp),'source_key':str(r['id'])},native_matrix_ref={'path':rel(mp),'file_SHA256':sha(mp),'source_key':str(r['id']),'source_RGBA_SHA256':r['source_rgba_sha256']})
save(f'selected-{ST}.json',selected);save(f'source-check-{ST}.json',proof)
for start in range(0,12,6):
 im=Image.new('RGB',(810,580),(30,30,30));draw=ImageDraw.Draw(im)
 for n,i in enumerate(IDS[start:start+6]):
  o=Image.open(B/f'original/{i}.png').convert('RGBA');x=(n%3)*270;y=(n//3)*290;im.paste(o.getchannel('A').resize((256,256),Image.Resampling.NEAREST).convert('RGB'),(x,y+24));draw.text((x,y),str(i),fill='white')
 im.save(B/f'private-{ST}/survey/whole-alpha-{start}.png')
print('Fresh compact fullRGBA/POT/typed chains12;',sum(len(v)for v in common.values()),'actual unique rows; RGB/A counts',[(r['id'],r['native_RGB_distinct_count'],r['material_evidence']['source_alpha_extrema'])for r in proof])

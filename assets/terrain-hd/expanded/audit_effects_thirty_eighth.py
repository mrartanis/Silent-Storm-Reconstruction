from pathlib import Path
import sys,json,re,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='effects-thirty-eighth';IDS=[717,901,2373,2375,2628,2629,3095,3732,5391,7410]
sys.path.insert(0,str(B.parent))
from prepare_sources import ReleaseResources,decode_mmp
from audit_texture_groups import read_database
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def canon(r):return hashlib.sha256(json.dumps(r,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode('utf-8')).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
db=Path('G:/SS/Silent-Storm/Complete/game.db');dbsha=sha(db);tables=read_database(db)
reg=(ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8');names={int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',reg)}
def named(n):
 ids=[i for i,s in names.items()if s==n and i in tables];assert len(ids)==1;return {r['ID']:r for r in tables[ids[0]]['records']}
T={n:named(n)for n in ['Textures','ParticleInstances','Particles','Materials','FinalElements','ContainerModels','Models','Geometries','MaterialTemplates','ModelTemplates']}
common={n:{}for n in T};hashes={n:{}for n in T}
def ref(n,i):
 r=T[n].get(i)
 if r is None:return None
 k=str(i);common[n][k]=r;hashes[n][k]=canon(r);return {'table':n,'id':i,'canonical_record_SHA256':hashes[n][k]}
queue=read(B/'groups/source-queues/effects.json');selected=[next(r for r in queue if r['id']==i)for i in IDS];ex=read(B/f'exclusions-{ST}.json');assert not set(IDS).intersection(ex['ids']);release=ReleaseResources('G:/SS/lab/baseline/res');checks=[];matrices={}
fields={'Materials':['TextureID'],'FinalElements':['LightFlareTexture'],'ContainerModels':['PLightFlareTexture']}
for r in selected:
 i=r['id'];assert T['Textures'][i]==r['texture'];assert r['usage']['direct_texture_uses'] and all(u['role']=='color'for u in r['usage']['direct_texture_uses']);pu=[u for u in r['usage']['direct_texture_uses']if u['table']=='ParticleInstances'];actual={(z['ID'],f)for z in T['ParticleInstances'].values()for f,v in z.items()if re.fullmatch(r'Texture\d+',f)and v==i};assert actual=={(u['id'],u['field'])for u in pu}
 usages=[];blend=set();crown=set();wrap=set();unresolved=[];missing=[]
 for u in pu:
  row=T['ParticleInstances'][u['id']];p=T['Particles'].get(row['ParticleID']);assert row[u['field']]==i;assert (p==u['particle_definition'])if p is not None else u['particle_definition']in ({},None)
  for k in ('AlphaBlending','Static','PivotX','PivotY','Scale','Speed','CycleCount'):assert row[k]==u[k]
  frames={k:v for k,v in row.items()if re.fullmatch(r'Texture\d+',k)and v>0};assert frames==u['frame_bindings'];absent={k:v for k,v in frames.items()if v not in T['Textures']}
  for v in set(frames.values()):ref('Textures',v)
  if p is None:unresolved.append({'instance_id':row['ID'],'particle_id':row['ParticleID'],'texture_field':u['field']})
  else:wrap.add((p['WrapX'],p['WrapY']))
  if absent:missing.append({'instance_id':row['ID'],'slots':absent})
  blend.add(row['AlphaBlending']);crown.add(row['IsCrown']);usages.append({'instance_ref':ref('ParticleInstances',row['ID']),'particle_definition_ref':ref('Particles',row['ParticleID']),'particle_definition_status':'resolved'if p is not None else'missing-actual-definition','texture_field':u['field'],'all_TextureN_frame_matrix':'All actual TextureN slots incl0/negative are in referenced exact ParticleInstances record. Positive slots resolve by common Textures ID keys; every present sibling row saved once.','positive_frame_count':len(frames),'missing_sibling_texture_slots':absent})
 non=[]
 for table,fs in fields.items():
  actual={(z['ID'],f)for z in T[table].values()for f in fs if z.get(f)==i};assert actual=={(u['id'],u['field'])for u in r['usage']['direct_texture_uses']if u['table']==table}
 for u in r['usage']['direct_texture_uses']:
  if u['table']=='ParticleInstances':continue
  n=u['table'];row=T[n][u['id']];assert row[u['field']]==i;entry={'table':n,'texture_field':u['field'],'actual_record_ref':ref(n,row['ID']),'runtime_unknowns':'Actual typed direct binding only; no inferred meshUV/dynamicdispatch or absent definitions.'}
  if n=='Materials':
   for k in ('Alpha','AddressMode','TemplateID'):assert row[k]==u[k]
   actual={(m['ID'],f)for m in T['Models'].values()for f,v in m.items()if re.fullmatch(r'Material\d+',f)and v==row['TemplateID']};assert actual=={(c['id'],c['field'])for c in u['consumers']};cons=[]
   for c in u['consumers']:
    m=T['Models'][c['id']];g=T['Geometries'][m['GeometryID']];assert m[c['field']]==row['TemplateID']==c['material_template_id'];assert m['GeometryID']==c['geometry_id']and m['TemplateID']==c['model_template_id']and g['SrcName']==c['geometry']and m['Flags']==c['flags'];cons.append({'texture_reference_field':c['field'],'model_ref':ref('Models',m['ID']),'geometry_ref':ref('Geometries',g['ID']),'model_template_ref':ref('ModelTemplates',m['TemplateID'])})
   entry.update(actual_material_template_ref=ref('MaterialTemplates',row['TemplateID']),all_actual_model_consumer_refs=cons)
  non.append(entry)
 hist=Path('G:/SS/Silent-Storm/Complete/Textures')/str(i);data=hist.read_bytes();h=decode_mmp(data);rr=decode_mmp(release.read(i));op=B.parent/r['original_png'];o=Image.open(op).convert('RGBA');assert h.size==rr.size==o.size==tuple(r['logical_size']);assert h.tobytes()==rr.tobytes()==o.tobytes();rgba=hashlib.sha256(o.tobytes()).hexdigest();assert rgba==r['source_rgba_sha256']==r['release_rgba_sha256'];assert hashlib.sha256(data).hexdigest()==r['source_sha256'];assert rgba not in ex['blocked_hashes']
 a=np.array(o);rgb=a[:,:,:3];l=rgb.mean(2);y,x=np.unravel_index(l.argmax(),l.shape)
 matrices[str(i)]={'id':i,'source_native_size':list(o.size),'source_RGBA_SHA256':rgba,'original_PNG_file_SHA256':sha(op),'actual_native_Type':r['texture']['Type'],'actual_source_RGBA_extrema':[list(z)for z in o.getextrema()],'native_RGBA_matrix':a.tolist(),'native_peak_mean_RGB_xy':[int(x),int(y)],'native_peak_mean_RGBA':a[y,x].tolist(),'qualification':'Stored original RGB/A exactly asdecoded, no unpremultiply/alpha0-empty inference/pixelrepair. One matrix per source common file, not repeated per consumer.'}
 scale=1;helper=o.convert('RGB');hp=B/f'generated/{i}-{ST}-source-rgb-support.png';assert not hp.exists();helper.save(hp)
 checks.append({'id':i,'fresh_DB_sha256':dbsha,'actual_Texture_record_ref':ref('Textures',i),'logical_size':list(o.size),'source_rgba_sha256':rgba,'source_binary_sha256':hashlib.sha256(data).hexdigest(),'source_binary_path':str(hist).replace('\\','/'),'original_PNG':rel(op),'original_PNG_file_SHA256':sha(op),'source_parity':'Fresh historical MMP == release resource == existing original PNG entire RGBA bytes/dims/hash; actual Texture row exact sourcequeue row.','fresh_typed_usage':usages,'fresh_nonparticle_usage':non,'material_evidence':{'actual_source_native_type':r['texture']['Type'],'source_alpha_extrema':list(o.getchannel('A').getextrema()),'actual_instance_alpha_blend_values':sorted(blend),'resolved_only_particle_Wrap_pairs':[list(p)for p in sorted(wrap)],'all_instance_IsCrown_values':sorted(crown),'unresolved_particle_definitions':unresolved,'missing_sibling_Texture_rows':missing,'actual_material_Alpha_AddressMode':[{'id':u['id'],'Alpha':u['Alpha'],'AddressMode':u['AddressMode']}for u in r['usage']['direct_texture_uses']if u['table']=='Materials'],'unknowns':'Missing Particles/siblingTextures and actualmeshUV/runtime dispatch explicitunknown. Enumvalues areactual, not inferred numericalrenderer factors. EveryDB binding unchanged.'},'source_RGB_whole_survey_privately_viewed':False,'imagegen_call_count':0,'source_only_reason':'Fresh fullsource colorart survey, needs exactnative count/weakphase/material proof before anysolecall. No family/Type blanketclassification.','helper_recipe':{'input':rel(op),'operations':f'ENTIRE original stored RGBA→opaqueRGB, whole NEAREST{scale}x; no crop/padding/rotate/fit/artinsert/unpremultiply. Full originalA separately restored root. Native opaque RGB directly changes no original pigment pixels; nearest guides reserved private QA.','source_size':list(o.size),'source_bbox':[0,0,o.width,o.height],'helper_size':list(helper.size),'helper_bbox':[0,0,helper.width,helper.height],'inverse':'ENTIRE raw LANCZOS to original4x; originalA independently LANCZOS4x; standardwhole scalar/defaultpadding and nativeType downstream byroot.','output':rel(hp)},'UV_proof':'One ENTIRE originalcanvas replacement/imagecalibration[1,1]. Actual ParticleInstances full TextureN matrix referenced. No inferred atlasgrid/meshUV; resolved IsCrown explicit. Main/GParticleInfo.cpp non-grass branch usescomplete frame rectangle, WrapParticlePosition wraps worldpositions; missing definitions unknown.'})
dbp=B/f'actual-records-{ST}.json';mp=B/f'native-matrices-{ST}.json';save(dbp.name,{'fresh_DB_path':str(db).replace('\\','/'),'fresh_DB_SHA256':dbsha,'canonical_record_hash_recipe':'SHA256(UTF8(json.dumps(actual_decoded_record,ensure_ascii=False,sort_keys=True,separators=(comma,colon))))','tables':common,'record_SHA256':hashes,'binding_semantics':'Exact actual records unique once; everyreferenced TextureN row/definition/frame/sampler ifpresent, missinglookup null explicitly unknown. No sourceRGBA repeated inconsumerrecords.'});save(mp.name,{'sources':matrices,'matrix_semantics':'Each native fullRGBA matrix stored once. All source/consumer proofs reference same immutable source id/file SHA. No generated guides/crops/artistpatches.'})
for r in checks:r.update(common_actual_records={'path':rel(dbp),'file_SHA256':sha(dbp),'source_key':str(r['id'])},native_matrix_ref={'path':rel(mp),'file_SHA256':sha(mp),'source_key':str(r['id']),'source_RGBA_SHA256':r['source_rgba_sha256']})
save(f'selected-{ST}.json',selected);save(f'source-check-{ST}.json',checks);print('Fresh compact actualDB12/fullnative12 unique matrices; common record count',sum(len(v)for v in common.values()))

import pathlib,json,hashlib,sys,collections,datetime,struct,re
from PIL import Image
ROOT=pathlib.Path(__file__).resolve().parents[3]
B=ROOT/'assets/terrain-hd';E=B/'expanded'
sys.path.insert(0,str(B))
from prepare_sources import ReleaseResources,decode_mmp
from audit_texture_groups import read_database
read=lambda p:json.loads(pathlib.Path(p).read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
hs=lambda a:hashlib.sha256(a).hexdigest()
def save(n,d):(E/n).write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def portable(p):
 try:return p.relative_to(ROOT).as_posix()
 except ValueError:return p.as_posix()
def resolve(r):
 p=pathlib.Path(r)
 if p.is_absolute():return p
 if p.parts[0]=='assets':return ROOT/p
 return B/p
books={3795,3796,3797,3953,6206,6207}
metadata_files=[B/'sources.json',E/'queue.json']
metadata_before={portable(p):sha(p)for p in metadata_files}
accepted=read(B/'sources.json')['textures'];q=read(E/'queue.json')
assert len(accepted)==2309 and len(q)==2734
ab={d['id']:d for d in accepted};excluded={d['id']for d in accepted+q}
for d in accepted:excluded.update(d.get('aliases',[]))
accepted_files=set()
for d in accepted:
 for k in ('png','uncalibrated_png','generated_png','reference_png','prompt_file'):
  if d.get(k) and resolve(d[k]).exists():accepted_files.add(resolve(d[k]))
 for k in ('reference_png','before_png'):
  v=d.get('brightness_calibration',{}).get(k)
  if v and resolve(v).exists():accepted_files.add(resolve(v))
accepted_before={portable(p):sha(p)for p in sorted(accepted_files)}
snapshot_digest=hs(json.dumps(accepted_before,sort_keys=True,separators=(',',':')).encode('utf-8'))
DB=pathlib.Path('G:/SS/Silent-Storm/Complete/game.db');dbsha=sha(DB)
tables=read_database(DB);textures={r['ID']:r for r in tables[3]['records']}
reg=(ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8')
names={int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',reg)}
def named(n):
 ids=[i for i,name in names.items()if name==n and i in tables];assert len(ids)==1
 return {r['ID']:r for r in tables[ids[0]]['records']}
# All tables retained in memory only; detailed typed chains emitted only for exact potential matches.
actual={n:named(n)for n in ['Materials','FinalElements','ContainerModels','ParticleInstances','Particles','Models','Geometries','MaterialTemplates','ModelTemplates']}
pool={};inputs=[]
for p in list((E/'groups/source-queues').glob('*.json'))+list(E.glob('*prepared*.json')):
 rows=read(p)
 if not isinstance(rows,list):continue
 inputs.append({'file':portable(p),'sha256':sha(p)})
 for r in rows:
  if not isinstance(r,dict):continue
  i=r.get('id',r.get('texture',{}).get('ID'))
  if i is None or i in excluded or i in books:continue
  pool.setdefault(i,{'record':r,'input_files':[]})['input_files'].append(portable(p))
assert len(pool)==1433,len(pool)
donors=collections.defaultdict(list);donor_errors=[];indexed=0
for d in accepted:
 i=d['id']
 if i in books:continue
 ref=d.get('reference_png',d.get('brightness_calibration',{}).get('reference_png'))
 try:
  assert ref and i in textures
  im=Image.open(resolve(ref)).convert('RGBA');rgba=hs(im.tobytes())
  if d.get('source_rgba_sha256'):assert rgba==d['source_rgba_sha256']
  donors[(rgba,im.size,textures[i]['Type'])].append(i);indexed+=1
 except Exception as exc:donor_errors.append({'id':i,'error':repr(exc)})
res=ReleaseResources('G:/SS/lab/baseline/res');checked=[];matches=[];errors=[]
def parity(i,reference=None):
 hp=pathlib.Path(f'G:/SS/Silent-Storm/Complete/Textures/{i}');hb=hp.read_bytes();rb=res.read(i)
 hi=decode_mmp(hb).convert('RGBA');ri=decode_mmp(rb).convert('RGBA');assert hi.size==ri.size and hi.tobytes()==ri.tobytes()
 orig=resolve(reference)if reference else E/f'original/{i}.png';exists=orig.exists()
 if exists:
  oi=Image.open(orig).convert('RGBA');assert oi.size==hi.size and oi.tobytes()==hi.tobytes()
 t=textures[i]
 return {'id':i,'RGBA_sha256':hs(hi.tobytes()),'dimensions':list(hi.size),'type':t['Type'],'alpha_extrema':list(hi.getchannel('A').getextrema()),'actual_source':{'texture_id':i,'SrcName':t['SrcName'],'native_db_dimensions':[t['Width'],t['Height']],'Format':t['Format'],'Type':t['Type'],'AddrType':t['AddrType'],'historical_file':hp.as_posix(),'release_resource':'G:/SS/lab/baseline/res/Textures.res/'+str(i)},'historical_MMP_sha256':hs(hb),'release_MMP_sha256':hs(rb),'historical_release_exact_native_bytes':hb==rb,'historical_release_exact_RGBA':True,'original_png':portable(orig)if exists else None,'original_png_sha256':sha(orig)if exists else None,'original_exact_RGBA':exists,'native_format':struct.unpack_from('<I',hb,4)[0]}
def bindings(i):
 out=[]
 for name,fields in [('Materials',['TextureID']),('FinalElements',['LightFlareTexture']),('ContainerModels',['PLightFlareTexture'])]:
  for row in actual[name].values():
   for field in fields:
    if row.get(field)!=i:continue
    entry={'table':name,'field':field,'actual_row':row}
    if name=='Materials':
     template=row['TemplateID'];entry['actual_material_template']=actual['MaterialTemplates'].get(template);entry['all_actual_models']=[]
     for m in actual['Models'].values():
      for mf,v in m.items():
       if re.fullmatch(r'Material\d+',mf)and v==template:entry['all_actual_models'].append({'field':mf,'model':m,'geometry':actual['Geometries'].get(m['GeometryID']),'model_template':actual['ModelTemplates'].get(m['TemplateID'])})
    out.append(entry)
 for row in actual['ParticleInstances'].values():
  slots={f:v for f,v in row.items()if re.fullmatch(r'Texture\d+',f)and v>0}
  for f,v in slots.items():
   if v==i:out.append({'table':'ParticleInstances','field':f,'actual_row':row,'actual_particle':actual['Particles'].get(row['ParticleID']),'all_frame_bindings':slots,'all_frame_texture_rows':{f:textures.get(v)for f,v in slots.items()},'unknowns':'Absent actual Particles row remains unknown; no inferred Wrap/blend. Resource replacement leaves DB/allframes unchanged.'})
 return out
potential=[]
for i,item in sorted(pool.items()):
 try:
  r=parity(i);r['donor_ids']=donors.get((r['RGBA_sha256'],tuple(r['dimensions']),r['type']),[])
  if r['donor_ids']:potential.append(r)
  # Per-ID compact parity/hash/Type/source locator only, not duplicate full DB chains.
  checked.append(r)
 except Exception as exc:errors.append({'id':i,'error':repr(exc)})
for r in potential:
 i=r['id'];choices=[]
 for di in r['donor_ids']:
  d=ab[di];dp=parity(di,d.get('reference_png',d.get('brightness_calibration',{}).get('reference_png')))
  assert (dp['RGBA_sha256'],dp['dimensions'],dp['type'])==(r['RGBA_sha256'],r['dimensions'],r['type'])
  grid=d.get('calibration_grid',d.get('brightness_calibration',{}).get('grid'))
  image=d.get('uncalibrated_png');whole=grid==[1,1]and image is not None
  if whole:
   im=Image.open(resolve(image));whole=im.size==tuple(n*4 for n in r['dimensions'])
  choices.append({'donor_id':di,'accepted':True,'accepted_source_parity':dp,'calibration_grid':grid,'whole_source_UV_1x1':whole,'layout_annotation':d.get('layout'),'accepted_donor_input':portable(resolve(image))if image else None,'accepted_donor_input_sha256':sha(resolve(image))if image else None,'accepted_balanced_png':portable(resolve(d['png'])),'accepted_balanced_png_sha256':sha(resolve(d['png'])),'native_type_equal':True,'complete_RGBA_dimensions_equal':True,'donor_actual_bindings':bindings(di)})
 matches.append({'id':i,'candidate_source':r,'actual_texture_row':textures[i],'candidate_actual_bindings':bindings(i),'accepted_donor_choices':choices,'wholecanvas_reuse_proof':'Exact native fullRGBA+dimensions+Type equal. Use accepted normalized uncalibrated entire4x canvas only if donor grid[1,1]; no crop/registration/alpha/UV/DB changes. 1x1 denotes image/calibration footprint, not a claim about unknown runtime semantics or absent particle definitions.','status':'potential-exact-accepted-donor-awaiting-coordinator' if any(d['whole_source_UV_1x1']for d in choices)else'exact-source-match-but-no-confirmed-1x1-donor'})
metadata_after={portable(p):sha(p)for p in metadata_files}
accepted_after={portable(p):sha(p)for p in sorted(accepted_files)}
assert metadata_after==metadata_before,'Common metadata changed externally during readonly audit; snapshot invalid'
assert accepted_after==accepted_before,'Accepted files changed externally during readonly audit; snapshot invalid'
assert sha(DB)==dbsha
scope={'scope':'Fresh exact-donors-eighteenth accepted-only source audit; no imagegen or live game. Per-ID compact hash/dimensions/Type/source locator, detailed actual bindings only potential matches.','checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'accepted_count':len(accepted),'main_queue_count':len(q),'remaining_count':len(pool),'excluded_accepted_alias_queue_ids_count':len(excluded),'excluded_authorized_bookcovers':sorted(books),'fresh_database_sha256':dbsha,'inputs':inputs,'accepted_original_indexed_count':indexed,'donor_index_errors':donor_errors,'checked_count':len(checked),'source_check_errors':errors,'matches_count':len(matches),'matches_ids':[r['id']for r in matches],'remaining_source_checks':checked,'immutability':{'shared_metadata_before':metadata_before,'shared_metadata_after':metadata_after,'accepted_file_count':len(accepted_files),'accepted_original_HD_raw_prompt_snapshot_sha256_before':snapshot_digest,'accepted_original_HD_raw_prompt_snapshot_sha256_after':hs(json.dumps(accepted_after,sort_keys=True,separators=(',',':')).encode('utf-8')),'all_accepted_files_unchanged':True,'sources_and_queue_unchanged':True},'imagegen_calls':0,'shared_metadata_modified':False,'note':'Only accepted2309 are eligible donors. SameRGBA pending/selected/raw is not a donor. Seven new root19 inputs are excluded when present in queue and never treated as accepted.'}
save('source-check-exact-donors-eighteenth-scope.json',scope);save('source-check-exact-donors-eighteenth.json',matches)
jobs=[]
for m in matches:
 choices=[d for d in m['accepted_donor_choices']if d['whole_source_UV_1x1']]
 if choices:
  d=choices[0];jobs.append({'id':m['id'],'generated':d['accepted_donor_input'],'reuse_source_id':d['donor_id'],'reuse_input_stage':'Accepted normalized uncalibrated wholecanvas4x donor only, source grid[1,1]. Exact full originalRGBA/dimensions/nativeType. Await coordinator validation/import, no calls or shared changes.','proof':'assets/terrain-hd/expanded/source-check-exact-donors-eighteenth.json'})
save('candidatejobs-exact-donors-eighteenth.json',jobs)
save('tinyreport-exact-donors-eighteenth.json',{k:scope[k]for k in ['scope','accepted_count','main_queue_count','remaining_count','checked_count','fresh_database_sha256','matches_count','matches_ids','source_check_errors','donor_index_errors','immutability','imagegen_calls']})
print(json.dumps({'accepted':len(accepted),'queue':len(q),'remaining':len(pool),'checked':len(checked),'errors':errors,'donor_errors':donor_errors,'matches':[(m['id'],[d['donor_id']for d in m['accepted_donor_choices']])for m in matches],'candidatejobs':len(jobs),'immutablefiles':len(accepted_files)},ensure_ascii=True))

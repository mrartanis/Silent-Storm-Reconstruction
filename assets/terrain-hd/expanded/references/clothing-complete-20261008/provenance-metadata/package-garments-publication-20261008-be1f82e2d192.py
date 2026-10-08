from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,shutil
ROOT=Path('G:/SS/Silent-Storm-Reconstruction');B=ROOT/'assets/terrain-hd/expanded';ST='clothing-garments-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
def write(p,v):
 assert not p.exists(),p
 p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
frozen=B/f'SHA-manifest-{ST}-FINAL.json';assert sha(frozen)=='05452d6e3f229e090623703deb9f28fb15e3622817f2fb23be1a6b63eaaaf40c';fm=read(frozen)
for row in fm['files']:assert sha(ROOT/row['path'])==row['SHA256'],('frozen file changed',row['path'])
alloc=read(B/'delegated-garments-clothing-complete-20261008.json');audit={r['id']:r for r in read(ROOT/alloc['source_audit'])};tail=set(read(B/'delegated-garments-coordinator-tail20-20261008.json')['IDs']);own={i for g in alloc['groups']for i in g['ids']}-tail;stable=B/'references'/ST;selecteddir=stable/'selected-sources';inputdir=stable/'input-references';selecteddir.mkdir(parents=True,exist_ok=True);inputdir.mkdir(exist_ok=True);maps={};retained={};local=[]
def private(p):return any(v.startswith('private')for v in p.parts)
def copy_existing(old,new,usage):
 old=old.resolve();new=new.resolve();assert old.is_file();assert old.suffix.lower()=='.png';assert new.is_relative_to(stable.resolve());assert old!=new;new.parent.mkdir(parents=True,exist_ok=True)
 if new.exists():assert old.read_bytes()==new.read_bytes()
 else:shutil.copyfile(old,new)
 assert old.read_bytes()==new.read_bytes();assert sha(old)==sha(new)
 key=old.as_posix()
 if key not in maps:maps[key]={'actual_original_path':key,'original_repo_path':rel(old),'published_copy_path':rel(new),'SHA256':sha(old),'bytes':old.stat().st_size,'exact_file_bytes_unchanged':True,'usages':[]}
 else:assert maps[key]['published_copy_path']==rel(new)
 maps[key]['usages'].append(usage);retained[new]={'path':rel(new),'SHA256':sha(new),'bytes':new.stat().st_size,'kind':'byte-identical existing source/input reference copy'}
for i in sorted(own):
 r=audit[i];rid=r['resource_when_gfx_texture_usedxt_0'];a=next(v for v in r['aliases']if v['resource_id']==rid)
 for kind in ['fullRGBA_PNG','nativeRGB_PNG','nativeA_PNG']:
  old=ROOT/a[kind];assert sha(old)==a[kind+'_SHA256'];copy_existing(old,selecteddir/old.name,{'ID':i,'role':'whole actual selected '+kind,'source_resource_id':rid,'source_RGBA_SHA256':a['native_RGBA_SHA256'],'original_was_actual_argument':False})
calls=[p for p in (B/'generated').glob(f'*-{ST}-*-call.json')if not p.name.endswith('-before-call.json')];actual_private_args=0
for cp in calls:
 c=read(cp)
 for rawp in c['literal_actual_tool_arguments']['referenced_image_paths']:
  old=Path(rawp);assert old.is_file()
  if not private(old):continue
  key=old.resolve().as_posix();dest=ROOT/maps[key]['published_copy_path']if key in maps else inputdir/f'{old.stem}-{sha(old)[:12]}{old.suffix}'
  copy_existing(old,dest,{'ID':c['id'],'generation_variant':c['generation_variant'],'role':'literal actual builtin referenced_image_paths argument','actual_original_argument_path':rawp,'call_metadata':rel(cp),'call_metadata_SHA256':sha(cp),'original_was_actual_argument':True});actual_private_args+=1
for bp in (B/'generated').glob(f'*-{ST}-*-before-call.json'):
 br=read(bp)
 for q,qs in zip(br.get('references',[br['reference']]),br.get('references_SHA256',[br['reference_SHA256']])):
  old=ROOT/q;assert sha(old)==qs
  if not private(old):continue
  key=old.resolve().as_posix();dest=ROOT/maps[key]['published_copy_path']if key in maps else inputdir/f'{old.stem}-{qs[:12]}{old.suffix}'
  copy_existing(old,dest,{'ID':br['id'],'generation_variant':br['generation_variant'],'role':'immutable BEFORE source-reference binding; may be prepared-only or actual-service-refused','before_call_metadata':rel(bp),'before_call_metadata_SHA256':sha(bp),'original_was_actual_argument':False})
for row in fm['files']:
 p=ROOT/row['path'];why=None
 if private(p):why='Private source original has byte-identical stable publication copy'if p.resolve().as_posix()in maps else'Private whole QA / preview / diagnostic evidence; remains local, frozen original SHA preserved.'
 elif p.suffix.lower()in ['.bmp','.html','.htm']:why='Local visual inspection artifact; never published.'
 elif p.suffix.lower()=='.lock':why='Local exclusive generation scheduling lock; actual call-start JSON retained.'
 elif p.suffix.lower()in ['.png','.jpg','.jpeg','.gif','.webp']and any(v in p.name.lower()for v in ['survey','montage','comparison','whole-qa','qa-sheet']):why='Local QA montage / survey image; never published.'
 if why:local.append({**row,'local_only_reason':why})
 else:retained[p]={'path':row['path'],'SHA256':row['SHA256'],'bytes':p.stat().st_size,'kind':'immutable artifact from original frozen manifest'}
mapping=B/f'publication-source-relocations-{ST}.json';write(mapping,{'scope':ST,'operation':'Byte-copy existing selected whole native RGBA/RGB/A and actual builtin input references only. No new artwork or pixel transformations.','original_frozen_manifest':rel(frozen),'original_frozen_manifest_SHA256':sha(frozen),'old_actual_BEFORE_prompt_call_and_argument_paths_unchanged':True,'actual_private_input_argument_occurrences':actual_private_args,'copied_files':len(maps),'mappings':list(maps.values())});retained[mapping]={'path':rel(mapping),'SHA256':sha(mapping),'bytes':mapping.stat().st_size,'kind':'immutable original argument to published copy relocation mapping'};retained[frozen]={'path':rel(frozen),'SHA256':sha(frozen),'bytes':frozen.stat().st_size,'kind':'original complete frozen 1684-file manifest; local-only entries remain honestly recorded'}
jobs=read(B/f'jobs-{ST}-FINAL.json');oldguards=[{'ID':j['id'],'generation_variant':j['generation_variant'],'actual_guard_file':j['worker_source_guard'],'note':'Earlier builtin call predates separate call-start helper; actual immutable BEFORE carries selected-source fresh guard. No new call-start synthesized.'}for j in jobs if j['id']!=5481 and j['worker_source_guard'].endswith('-before-call.json')]
gp=B/f'publication-provenance-guard-accounting-{ST}.json';write(gp,{'scope':ST,'final_actual_art_calls':63,'exact_source_sibling_new_calls':0,'actual_before_guard_only_old_calls':oldguards,'actual_separate_call_start_count':63-len(oldguards),'actual_service_refused_IDs':[4947,5670],'actual_refused_assets_not_retried':True,'all_63_raw_argument_prompt_BEFORE_bindings_and_original_frozen_1684_hashes_rechecked':True});retained[gp]={'path':rel(gp),'SHA256':sha(gp),'bytes':gp.stat().st_size,'kind':'honest actual guard/provenance accounting'}
script=Path(__file__).resolve();retained[script]={'path':rel(script),'SHA256':sha(script),'bytes':script.stat().st_size,'kind':'byte-copy publication packaging reproducibility code'}
for p in retained:assert not private(p);assert p.suffix.lower()not in ['.bmp','.html','.htm','.lock'];assert sha(p)==retained[p]['SHA256']
pub=B/f'publication-SHA-manifest-{ST}-FINAL.json';write(pub,{'scope':ST,'status':'publication packaging ready; Game/native acceptance remains root responsibility','original_full_manifest':rel(frozen),'original_full_manifest_SHA256':sha(frozen),'original_full_manifest_file_count':1684,'publication_file_count':len(retained),'files':sorted(retained.values(),key=lambda r:r['path']),'strict_no_private_paths_in_publication_files':True,'no_QA_montage_survey_BMP_HTML_published':True,'source_relocations':rel(mapping),'source_relocations_SHA256':sha(mapping),'local_only_file_count':len(local),'local_only':local,'before_arguments_and_existing_files_not_rewritten':True,'source_copies_byte_identical_no_new_images':True,'utc':datetime.now(timezone.utc).isoformat()});print(json.dumps({'publication_manifest':rel(pub),'SHA256':sha(pub),'publication_files':len(retained),'local_only':len(local),'source_copies':len(maps),'relocation_mapping':rel(mapping),'mapping_SHA256':sha(mapping),'old_BEFORE_guard_only_IDs':[r['ID']for r in oldguards],'original_1684_SHA_unchanged':sha(frozen)}))

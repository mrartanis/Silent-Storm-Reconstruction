"""Prepare stable byte-exact provenance copies; never alter frozen worker evidence."""
from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,shutil,gzip,re
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def rp(s):
 p=Path(s.replace('\\','/'));return p if p.is_absolute() else ROOT/p
def rel(p):return p.resolve().relative_to(ROOT).as_posix()
def ref(p,kind):return {'path':rel(p),'SHA256':sha(p),'bytes':p.stat().st_size,'kind':kind}
def save(p,v):
 assert not p.exists(),p
 p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
origp=B/f'SHA-manifest-{ST}-final.json';origsha=sha(origp);orig=read(origp)
helperp=B/'publication-SHA-manifest-clothing-garments-complete-20261008-FINAL.json'
helpermaps=B/'publication-source-relocations-clothing-garments-complete-20261008.json'
assert sha(helperp)=='61ca2092d0ab1f5665d961d2775b268b59ea87ca0df4d1cf470912f2abcdda31'
assert sha(helpermaps)=='e9b450773e0f3b0aca54a0a220293373dbdf546d6b25797b5e7a440c30cbfa3a'
pub={};local=[];mappings={};input_images=set();callbind=[];large=[]
for f in read(helperp)['files']:
 p=rp(f['path']);assert sha(p)==f['SHA256'];pub[rel(p)]=ref(p,'delegated garment worker strict stable publication artifact')
for m in read(helpermaps)['mappings']:
 assert sha(rp(m['published_copy_path']))==m['SHA256'];mappings[m['original_repo_path']]=dict(m)
pub[rel(helperp)]=ref(helperp,'delegated strict publication manifest')
pub[rel(helpermaps)]=ref(helpermaps,'delegated immutable original-argument-path relocation map')
guardp=B/'publication-provenance-guard-accounting-clothing-garments-complete-20261008.json'
pub[rel(guardp)]=ref(guardp,'delegated actual guards/call accounting')

def path_exists(v):
 if not isinstance(v,str) or '\n' in v or len(v)>500:return None
 try:
  p=rp(v)
  return p if p.is_file() and p.resolve().is_relative_to(ROOT) else None
 except (OSError,ValueError):return None
def raw_paths(v):
 out=[]
 if isinstance(v,dict):
  for k,x in v.items():
   if k in ['generated','raw','raw_saved','saved_raw','saved_raw_png','generated_png','raw_path']:
    p=path_exists(x)
    if p and p.suffix.lower()=='.png':out.append(p)
   if isinstance(x,(dict,list)):out+=raw_paths(x)
 elif isinstance(v,list):
  for x in v:out+=raw_paths(x)
 return out
def references(v):
 out=[]
 if isinstance(v,dict):
  for k,x in v.items():
   if k in ['referenced_image_paths','reference_pngs','references'] and isinstance(x,list):
    for s in x:
     p=path_exists(s)
     if p and p.suffix.lower()=='.png':out.append(p)
   if isinstance(x,(dict,list)):out+=references(x)
 elif isinstance(v,list):
  for x in v:out+=references(x)
 return out
call_data=[]
for f in orig['files']:
 p=rp(f['path']);assert sha(p)==f['SHA256'],('frozen file changed',f['path'])
 if p.suffix.lower()!='.json':continue
 d=read(p)
 if not isinstance(d,dict):continue
 iscall=('call' in p.name or 'refusal' in p.name or 'tool-error' in p.name) and ('image' in str(d.get('tool','')).lower() or d.get('builtin') or 'literal_actual_tool_arguments' in d or 'exact_service_args' in d or 'reproducible_tool_arguments' in d or 'actual_tool_arguments' in d)
 if not iscall:continue
 raws=raw_paths(d);refs=references(d);input_images.update(refs)
 call_data.append((p,d,raws,refs))
 for q in raws:pub[rel(q)]=ref(q,'saved output of an actual historical built-in call, including earlier rejected attempts')
 callbind.append({'call_file':rel(p),'call_SHA256':sha(p),'raw_files':[rel(q)for q in raws],'actual_input_reference_files':[rel(q)for q in refs],'new_call_created':False})

def private(p):return any(part.lower().startswith('private') for part in p.relative_to(ROOT).parts)
def copy_reference(p,kind,usage):
 original=rel(p);h=sha(p)
 if original in mappings:
  m=mappings[original];assert m['SHA256']==h
  m.setdefault('whole_clothing_additional_usages',[]).append(usage)
  pub[m['published_copy_path']]=ref(rp(m['published_copy_path']),'byte-identical required source/reference copy')
  return
 sub={'source':'selected-and-alternate-sources','input':'actual-inputs','metadata':'provenance-metadata','raw':'historical-raws'}[kind]
 dest=B/f'references/{ST}/{sub}/{p.stem}-{h[:12]}{p.suffix.lower()}'
 dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists():assert sha(dest)==h
 else:shutil.copyfile(p,dest)
 assert dest.read_bytes()==p.read_bytes()
 mappings[original]={'actual_original_path':p.resolve().as_posix(),'original_repo_path':original,'published_copy_path':rel(dest),'SHA256':h,'bytes':p.stat().st_size,'exact_file_bytes_unchanged':True,'usages':[usage]}
 pub[rel(dest)]=ref(dest,'byte-identical required full source or actual builtin input reference')
for r in read(B/f'source-check-{ST}.json'):
 for a in r.get('aliases',[]):
  if not a.get('present'):continue
  for field in ['fullRGBA_PNG','nativeRGB_PNG','nativeA_PNG']:
   p=rp(a[field]);copy_reference(p,'source',{'ID':r['id'],'source_resource_id':a['resource_id'],'role':field,'actual_selected':a['resource_id']==r['resource_when_gfx_texture_usedxt_0'],'original_was_actual_input_argument':p in input_images})
for p in sorted(input_images):
 if private(p) or rel(p).startswith('build/'):
  copy_reference(p,'input',{'role':'actual historical builtin input reference','original_was_actual_input_argument':True})
 else:pub[rel(p)]=ref(p,'stable exact actual historical builtin input reference')

def compress_large_json(p):
 h=sha(p);existing=Path(str(p)+'.gz')
 if existing.exists() and hashlib.sha256(gzip.decompress(existing.read_bytes())).hexdigest()==h:dest=existing
 else:
  dest=B/f'references/{ST}/provenance-json/{p.name}-{h[:12]}.gz';dest.parent.mkdir(parents=True,exist_ok=True)
  payload=gzip.compress(p.read_bytes(),compresslevel=9,mtime=0)
  if dest.exists():assert dest.read_bytes()==payload
  else:dest.write_bytes(payload)
 assert hashlib.sha256(gzip.decompress(dest.read_bytes())).hexdigest()==h
 large.append({'original_json_path':rel(p),'original_JSON_SHA256':h,'original_bytes':p.stat().st_size,'published_gzip_path':rel(dest),'gzip_SHA256':sha(dest),'gzip_bytes':dest.stat().st_size,'lossless_restoration_byte_exact':True,'original_local_file_unchanged':True})
 pub[rel(dest)]=ref(dest,'lossless portable large historical JSON proof; original byte SHA preserved')

for f in orig['files']:
 p=rp(f['path']);name=p.name.lower();parts=[x.lower()for x in p.relative_to(ROOT).parts]
 reason=None
 if private(p):reason='Private source/projection/diagnostic/survey artifact; required full source or actual tool input has byte-identical stable copy when needed.'
 elif rel(p).startswith('build/'):reason='Ignored historical build reference path; actual inputs have byte-identical stable copies.'
 elif p.suffix.lower() in ['.bmp','.html','.htm','.lock']:reason='Local-only visual/HTML/scheduling artifact.'
 elif p.suffix.lower()=='.png' and rel(p) not in pub:
  if any(x in name for x in ['survey','preview','contact','montage','comparison','whole-qa','pure-native','rgbinverse','roundtrip','diagnostic','gain-preview','normalized']):reason='Local-only visual QA/projection; actual input references separately retained.'
  elif '/generated/' in '/'+rel(p) and not re.search(r'(^|[-_])raw([-. _]|$)',name):reason='Historical unselected derived image; actual-call raws/input references and accepted production snapshots retained.'
 if reason:
  local.append({'path':f['path'],'SHA256':f['SHA256'],'bytes':f['bytes'],'local_only_reason':reason});continue
 if p.suffix.lower()=='.json' and p.stat().st_size>10*1024*1024:
  compress_large_json(p);continue
 pub[rel(p)]=ref(p,'immutable stable provenance/source/raw/arguments/BEFORE/check from full frozen manifest')
# Keep accepted clothing production PNGs even where generic historical-derived-image filtering applies.
category=read(B/f'category-final-{ST}.json')
for r in category['accepted_preservation']:
 for f in r['files']:
  p=rp(f['path']);assert sha(p)==f['SHA256'];pub[rel(p)]=ref(p,'previously accepted clothing PNG preserved exactly, no regeneration')
for p in [Path(__file__),origp]:pub[rel(p)]=ref(p,'strict publication preparer / original full immutable worker evidence manifest')
# Delegated strict package includes its historical packaging script in build/. Preserve
# that byte-exact tooling evidence through an explicit stable metadata copy as well.
for path in list(pub):
 p=rp(path)
 if path.startswith('build/'):
  copy_reference(p,'input' if p in input_images else 'metadata',{'role':'actual builtin input' if p in input_images else 'historical delegated packaging/proof artifact','original_was_actual_input_argument':p in input_images})
  del pub[path]
 elif private(p):
  # A genuine old saved output, if a historic call used a private save location,
  # remains an ordinary byte-identical PNG under stable references/historical-raws.
  if 'saved output of an actual' in pub[path]['kind']:
   copy_reference(p,'raw',{'role':'genuine historical saved builtin output','original_was_actual_input_argument':False});del pub[path]
  else:raise AssertionError(('unexpected private publication artifact',path))
assert sha(origp)==origsha
for path in pub:
 assert not private(rp(path)) and not path.startswith('build/'),('nonstable publication entry',path)
 assert rp(path).suffix.lower() not in ['.bmp','.html','.htm','.lock']
mapfile=B/f'publication-source-relocations-{ST}.json'
save(mapfile,{'scope':'whole characters-clothing','original_frozen_manifest':rel(origp),'original_frozen_manifest_SHA256':origsha,
 'helper_relocations':ref(helpermaps,'immutable delegated mapping'),'old_actual_arguments_BEFORE_calls_and_paths_unchanged':True,
 'operation':'Only byte-copy already existing selected/alternate full source RGB/A and actual imagegen input references into stable expanded/references. No imagegen calls or pixel transformations.',
 'mappings':[mappings[k]for k in sorted(mappings)]})
largefile=B/f'publication-large-JSON-portability-{ST}.json'
save(largefile,{'scope':'whole clothing historical JSON proofs','operation':'Lossless gzip only for metadata JSON over10MiB, never PNG. Original files and full frozen SHA unchanged. Missing originals may be restored from published archive and verified by original byte SHA.','mappings':large})
callfile=B/f'publication-actual-call-accounting-{ST}.json'
save(callfile,{'scope':'whole clothing source audit/history','actual_existing_call_metadata_files':callbind,
 'calls_or_BEFORE_synthesized':False,'actual_refusal_IDs':[1760,4947,5670],'related_NO_own_call_IDs':[801,1761,1762,2446],
 'selected_source_copies_complete':True,'literal_actual_reference_paths_are_preserved_in_old_calls':True,
 '2011_v5_is_source_only_cancelled_no_call':True,'root7641_v1_interrupted_before_result_is_NOT_refusal':True})
for p in [mapfile,largefile,callfile]:pub[rel(p)]=ref(p,'strict immutable publication mapping/accounting')
out=B/f'publication-SHA-manifest-{ST}-final.json'
files=[pub[k]for k in sorted(pub)]
save(out,{'scope':'whole characters-clothing stable publication','status':'complete-stable-publication-package-awaiting-coordinator-unified-build',
 'original_full_manifest':rel(origp),'original_full_manifest_SHA256':origsha,'original_manifest_unchanged':True,
 'helper_strict_publication':ref(helperp,'delegated manifest'),'source_relocations':ref(mapfile,'original-argument-path to byte-identical published copy'),
 'large_JSON_portability':ref(largefile,'lossless portable source proof metadata'),'actual_call_accounting':ref(callfile,'actual immutable generation history'),
 'publication_file_count':len(files),'publication_total_bytes':sum(f['bytes']for f in files),'files':files,
 'local_only_file_count':len(local),'local_only':local,'strict_no_private_build_paths_in_publication_files':True,
 'no_survey_QA_visual_BMP_HTML_locks_published':True,'no_PNG_archives':True,'old_calls_raws_prompts_arguments_BEFORE_unchanged':True,
 'new_generation_calls':0,'new_artwork_or_source_pixel_transformations':0,'utc':datetime.now(timezone.utc).isoformat()})
print(json.dumps({'publication':ref(out,'whole strict publication'),'files':len(files),'MiB':sum(f['bytes']for f in files)/1048576,'mappings':len(mappings),'call_files':len(callbind),'large_JSON_archives':len(large),'local_only':len(local),'original6199_manifest_unchanged':True}))

"""Explicit portable menu publication scope; preserve frozen bytes, exclude user work."""
from pathlib import Path
import json,hashlib,subprocess,sys
R=Path(__file__).resolve().parents[3];H=R/'assets/terrain-hd';W=H/'expanded/mainmenu-complete-20261015'
rd=lambda p:json.loads(Path(p).read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def rel(p):return p.resolve().relative_to(R).as_posix()
def save(p,v):
 data=(json.dumps(v,ensure_ascii=False,indent=2)+'\n').encode()
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
assert subprocess.check_output(['git','branch','--show-current'],cwd=R).strip()==b'develop'
mutable=[R/'HD-TEXTURES.md',H/'CONTINUE-HD-TEXTURES.md',H/'README.md',H/'sources.json',H/'brightness-report.json',H/'expanded/queue.json',H/'expanded/coverage.json']
if sys.argv[1]=='prepare':
 assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=R).strip()
 assert rd(W/'category-final.json')['status']=='complete'
 paths={p for p in W.rglob('*') if p.is_file()}
 tools=['audit_mainmenu_complete_20261015.py','work_mainmenu_complete_20261015.py','finalize_mainmenu_complete_20261015.py','verify_mainmenu_normal_launch_20261015.py','record_mainmenu_complete_20261015.py','publish_mainmenu_complete_20261015.py']
 paths.update(H/'expanded'/n for n in tools)
 ids={j['id'] for j in rd(W/'jobs-final.json')}
 for r in rd(H/'sources.json')['textures']:
  if r['id'] in ids:
   for k in ['png','uncalibrated_png','generated_png','reference_png','native_alpha_reference','prompt_file']:
    if r.get(k):paths.add(H/r[k])
 for r in rd(W/'coordinator-prepared.json'):paths.add(H/r['original_png'])
 paths.update(mutable)
 assert all(p.is_file() and p.suffix.lower() not in ['.bmp','.html','.res','.zip','.lock'] and 'private' not in rel(p).lower() for p in paths)
 assert max(p.stat().st_size for p in paths)<100000000
 tracked={s.decode('utf-8') for s in subprocess.check_output(['git','ls-files','-z'],cwd=R).split(b'\0') if s}
 manifest=W/'publication-SHA-manifest.json';pathmanifest=W/'publication-paths.json'
 attrs=R/'.gitattributes';old=attrs.read_bytes();newtext=[p for p in sorted(paths|{manifest,pathmanifest}) if rel(p) not in tracked and p.suffix.lower() in ['.json','.txt','.py','.log']]
 addition=b'\n# Frozen mainmenu20261015 actual imagegen inputs, raw provenance and final proofs\n'
 for p in newtext:
  line=rel(p)+(' -text -diff' if p.suffix.lower()=='.log' else ' -text')
  if line.encode() not in old:addition+=line.encode()+b'\n'
 attrs.write_bytes(old+addition);paths.add(attrs)
 stable=sorted(paths-set(mutable)-{attrs})
 save(manifest,{'category':'mainmenu-complete-20261015','new_HD':20,'old_global_accepted_preserved':2817,'whole_menu_scope':33,'stable_files':[{'path':rel(p),'bytes':p.stat().st_size,'SHA256':sha(p)} for p in stable],'mutable_publication_snapshots':[{'path':rel(p),'bytes':p.stat().st_size,'SHA256_at_publication':sha(p)} for p in mutable],'private_LOCAL_ONLY_policy':'All private QA/montages/preview PNG, actual Game BMP/ordinary desktop captures and HTML remain local; numeric SHA/review records are published. Stable actual imagegen references/full sourceRGBA/guide PNG and all genuine raw/BEFORE/exact args are included. Built RES stay ignored and are assembled with Game.','portable_build_runtime_copy_mapping':'portable-evidence-relocations.json','private_native_reproduction':'Artist-reviewed native forecasts and actual native exported PNG are private QA. Their fullRGBA hashes and actual bothalias/allmip proof are published; reproduce exact actualnative from balancedPNG/sourceA using tracked build_pack.mmp and compiled pack.'})
 paths.add(manifest);paths.add(pathmanifest);save(pathmanifest,{'paths':[rel(p) for p in sorted(paths)],'bytes':sum(p.stat().st_size for p in paths if p.is_file()),'staged_only_menu_scope':True,'preexisting_dirty_files_excluded':rd(W/'checkout-before.json')['dirty_tracked_files']})
 total=sum(p.stat().st_size for p in paths);print('Prepared explicit menu publication',len(paths),'paths',round(total/1024**2,2),'MiB; no private/BMP/HTML/RES/ZIP; rootREADME and38dirtyfiles excluded.')
elif sys.argv[1]=='stage':
 paths=[R/p for p in rd(W/'publication-paths.json')['paths']];assert sum(p.stat().st_size for p in paths)<500*1024**2
 for r in rd(W/'publication-SHA-manifest.json')['stable_files']:assert sha(R/r['path'])==r['SHA256']
 subprocess.run(['git','add','--pathspec-from-file=-','--pathspec-file-nul'],cwd=R,input=b'\0'.join(rel(p).encode() for p in paths)+b'\0',check=True,capture_output=True)
 names=[p.decode() for p in subprocess.check_output(['git','diff','--cached','--name-only','-z'],cwd=R).split(b'\0') if p]
 forbidden=set(rd(W/'checkout-before.json')['dirty_tracked_files']);assert not forbidden.intersection(names)
 data=subprocess.check_output(['git','cat-file','--batch'],cwd=R,input=''.join(':'+p+'\n' for p in names).encode());offset=0;errors=[];eol=[];sizes=[];png=[]
 allowed={rel(p) for p in mutable}|{'.gitattributes'}
 for n in names:
  end=data.index(b'\n',offset);size=int(data[offset:end].split()[-1]);blob=data[end+1:end+1+size];offset=end+size+2;sizes.append(size);disk=(R/n).read_bytes()
  if blob!=disk:
   if n in allowed and blob==disk.replace(b'\r\n',b'\n'):eol.append(n)
   else:errors.append(n)
  if Path(n).suffix.lower()=='.png':assert blob.startswith(b'\x89PNG\r\n\x1a\n');png.append(n)
 assert not errors,errors;assert max(sizes)<100000000 and sum(sizes)<500*1024**2
 attrs=subprocess.check_output(['git','check-attr','-z','--stdin','filter'],cwd=R,input=b'\0'.join(p.encode() for p in png)+b'\0');assert b'\0filter\0lfs\0' not in attrs
 assert all('private' not in p.lower() and Path(p).suffix.lower() not in ['.bmp','.html','.res','.zip','.lock'] for p in names)
 save(R/'build/hd-mainmenu-index-proof.json',{'staged_files':len(names),'ordinary_PNGs':len(png),'bytes':sum(sizes),'MiB':sum(sizes)/1024**2,'max_file_bytes':max(sizes),'frozen_byte_differences':errors,'mutable_EOL_only':eol,'preexisting38dirtyfiles_excluded':True,'fonts_code_excluded':True})
 print('Staged',len(names),'files',len(png),'ordinaryPNG',round(sum(sizes)/1024**2,2),'MiB; frozen-byteerrors0; original38dirtyfiles excluded.')

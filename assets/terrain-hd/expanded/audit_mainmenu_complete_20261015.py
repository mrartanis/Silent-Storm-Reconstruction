"""Immutable main-menu scope and actual original source qualification."""
from pathlib import Path
import hashlib,json,sys,subprocess,struct
from PIL import Image
R=Path(__file__).resolve().parents[3]; H=R/'assets/terrain-hd'; W=H/'expanded/mainmenu-complete-20261015'
sys.path.insert(0,str(H))
from prepare_sources import ReleaseResources,decode_mmp
read=lambda p:json.loads(Path(p).read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,v):
 p=Path(p)
 p.parent.mkdir(parents=True,exist_ok=True); data=(json.dumps(v,ensure_ascii=False,indent=2)+'\n').encode()
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
def png(p,im):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert Image.open(p).convert(im.mode).tobytes()==im.tobytes(),p
 else:im.save(p)
def rel(p):return p.relative_to(R).as_posix()
assert subprocess.check_output(['git','branch','--show-current'],cwd=R).strip()==b'develop'
S=read(H/'sources.json'); accepted={r['id']:r for r in S['textures']}
pack=Path('G:/SS/lab/build-x64-stage4/RelWithDebInfo/res-hd'); N=read(pack/'manifest.json')
assert len(S['textures'])==len(N['textures'])==2817
base=R/'build/hd-before-mainmenu-complete-20261015-groups'
save(str(base)+'-sources.json',S);save(str(base)+'-native.json',N)
save(str(base)+'-png-hashes.json',{str(H/r['png']):sha(H/r['png']) for r in S['textures']})
inv=read(H/'expanded/full-texture-inventory.json'); group={r['id']:r for r in read(H/'expanded/groups/interface-v5.json')}
release=ReleaseResources('G:/SS/lab/baseline/res'); historical=Path('G:/SS/Silent-Storm/Complete/Textures')
rows=[]; candidates=[]
for t in inv:
 if 'interface v5.0/mainmenu/' not in t.get('SrcName','').replace('\\','/').lower():continue
 i=t['ID']; rid=i|0x1000000 if t['Format'].startswith('dxt') else i
 uses=group.get(i,{}).get('direct_texture_uses',[])
 status='accepted-preserved' if i in accepted else ('technical-original' if i in [4664,4665,5600,5601] else 'artwork-pending')
 aliases=[]
 for a in [i,i|0x1000000]:
  if not (historical/str(a)).is_file():
   assert a!=rid,(i,'selected source absent');aliases.append({'resource_id':a,'present':False,'selected':False});continue
  d=(historical/str(a)).read_bytes(); native=release.read(a); im=decode_mmp(d); ni=decode_mmp(native)
  assert im.size==ni.size and im.tobytes()==ni.tobytes(),(i,a)
  p=W/'source'/f'{i}-resource-{a}-RGBA.png';png(p,im)
  aliases.append({'resource_id':a,'historical_MMP_SHA256':hashlib.sha256(d).hexdigest(),'release_MMP_SHA256':hashlib.sha256(native).hexdigest(),'RGBA_SHA256':hashlib.sha256(im.tobytes()).hexdigest(),'exact_historical_release_RGBA':True,'full_RGBA':rel(p),'native_format':struct.unpack_from('<I',d,4)[0],'size':list(im.size),'alpha_extrema':list(im.getchannel('A').getextrema())})
 selected=next(x for x in aliases if x['resource_id']==rid); source=Image.open(R/selected['full_RGBA']).convert('RGBA')
 assert source.size==(t['Width'],t['Height'])
 row={'id':i,'texture':t,'status':status,'actual_selected_resource':rid,'selection_proof':'DBFormat/DataFormat.cpp CTexture::Import bIsDXT=(Format prefix dxt); Main/GTexture.cpp:GetRealTextureID under gfx_texture_usedxt0 selects UC alias for bIsDXT, otherwise primary. Whole native RGBA is verified, not primary alpha reused.','uses':uses,'aliases':aliases}
 if status!='accepted-preserved':
  g=W/'guide'/f'{i}-whole-RGB.png';png(g,source.convert('RGB').resize(tuple(n*max(1,1024//max(source.size)) for n in source.size),Image.Resampling.NEAREST));row['whole_RGB_guide']=rel(g)
  a=W/'guide'/f'{i}-whole-alpha.png';png(a,source.getchannel('A').resize(Image.open(g).size,Image.Resampling.NEAREST));row['whole_alpha_guide']=rel(a)
 if status=='artwork-pending':
  candidates.append({'texture':t,'category':'interface-v5','role':'typed-db-color-candidate','source_resource_id':rid,'source_resource_selection_reason':row['selection_proof'],'usage':{'direct_texture_uses':uses,'ui_links':group.get(i,{}).get('ui_links',[])},'review_requirements':['Whole original RGB/A, layout, material and exact source print fidelity; one imagegen call per asset; full source alpha restored.']})
 rows.append(row)
assert len(rows)==33 and len(candidates)==20
save(W/'source-audit.json',{'scope':'All artwork in exact Interface v5.0/MainMenu folder; visibility of every asset in current camera is not claimed. Fonts and caption rendering excluded.','accepted_preserved':9,'artwork_pending':20,'technical_originals':4,'records':rows})
save(W/'candidates.json',candidates)
save(W/'checkout-before.json',{'HEAD':subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip(),'dirty_tracked_files':subprocess.check_output(['git','diff','--name-only'],cwd=R).decode().splitlines(),'Game_SHA256':sha(Path('G:/SS/lab/build-x64-stage4/RelWithDebInfo/Game.exe')),'font_files_SHA256':{p.as_posix():sha(R/p) for p in [Path('Main/VectorFontRasterizer.cpp'),Path('Main/VectorFonts.cpp'),Path('Main/GLocale.cpp'),Path('Main/UIML.cpp'),Path('Main/iMainMenu.cpp')]}})
print('MainMenu33:9 accepted preserved,20 artwork pending,4 technical. All actual selected RGBA historical/release exact; old2817 baseline frozen.')

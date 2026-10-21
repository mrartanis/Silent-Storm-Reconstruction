from pathlib import Path
import json,re,hashlib,sys,struct
import numpy as np
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-complete-20261008';sys.path.insert(0,str(B.parent))
from prepare_sources import ReleaseResources,decode_mmp
from audit_texture_groups import read_database
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def hs(data):return hashlib.sha256(data).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def canon(r):return hs(json.dumps(r,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode('utf-8'))
release=ReleaseResources('G:/SS/lab/baseline/res');DB=Path('G:/SS/Silent-Storm/Complete/game.db');db=read_database(DB);regs=(ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8');names={int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',regs)};T={names[k]:{r['ID']:r for r in t['records']}for k,t in db.items()if k in names};common={};hashes={}
def ref(t,i):
 row=T.get(t,{}).get(i)
 if row is None:return {'table':t,'id':i,'missing':True}
 common.setdefault(t,{})[str(i)]=row;hashes.setdefault(t,{})[str(i)]=canon(row)
 return {'table':t,'id':i,'canonical_record_SHA256':canon(row)}
S=read(B.parent/'sources.json')['textures'];Q=read(B/'queue.json');sq=read(B/'groups/source-queues/characters-clothing.json');sids={r['id']for r in S};aliasids={a for r in S for a in r.get('aliases',[])if isinstance(a,int)};qids={r['id']:r for r in Q};pending=[r for r in sq if r['id']not in sids|aliasids];wanted={r['id']for r in pending}
files=list(B.rglob('*'));history={i:{'raws':[],'calls':[],'prompts':[],'reviews':[]}for i in wanted}
for p in files:
 if not p.is_file()or ST in str(p):continue
 m=re.match(r'(\d+)-',p.name)
 if m and int(m[1])in wanted:
  i=int(m[1]);k='raws'if 'raw'in p.name and p.suffix=='.png'else'calls'if 'call'in p.name and p.suffix=='.json'else'prompts'if 'prompt'in p.name and p.suffix=='.txt'else None
  if k:history[i][k].append({'path':rel(p),'SHA256':sha(p)})
for p in B.glob('*review*.json'):
 if ST in p.name:continue
 try:d=read(p)
 except Exception:continue
 if not isinstance(d,dict):continue
 for r in d.get('records',[]):
  if isinstance(r,dict)and r.get('id')in wanted:history[r['id']]['reviews'].append({'path':rel(p),'file_SHA256':sha(p),'record':r})
# Accepted native aliases constitute immutable donors. Previous held/called sources are repair eligible, never accepted.
sizes={tuple(r['logical_size'])for r in pending if r.get('logical_size')};donors={};donorrows=[]
for s in S:
 i=s['id']
 for rid in [i,i|0x1000000]:
  try:data=release.read(rid)
  except KeyError:continue
  sig,fmt,avg,w,h,mips=struct.unpack_from('<6I',data)
  if (w,h)not in sizes:continue
  im=decode_mmp(data).convert('RGBA');key=str(im.size)+hs(im.tobytes());donors.setdefault(key,[]).append({'id':i,'resource_id':rid,'source_status':s.get('status')});donorrows.append({'id':i,'resource_id':rid,'native_size':list(im.size),'RGBA_SHA256':hs(im.tobytes()),'payload_SHA256':hs(data)})
proof=[];selected=[];out=B/f'private-{ST}';out.mkdir(exist_ok=True);(out/'sources').mkdir(exist_ok=True);(out/'native4x').mkdir(exist_ok=True)
for r in pending:
 i=r['id'];op=B/f'original/{i}.png';entry={'id':i,'source_queue_status':r.get('status'),'original_available':op.exists(),'prior_history':history[i],'actual_Texture_ref':ref('Textures',i),'shared_queue_record':qids.get(i),'initial_status':'pending-audit'}
 if not op.exists()or r.get('status')!='pending':entry.update(initial_status='unavailable-source',reason=r.get('reason'));proof.append(entry);continue
 o=Image.open(op).convert('RGBA');hb=Path(f'G:/SS/Silent-Storm/Complete/Textures/{i}').read_bytes();rb=release.read(i);assert o.tobytes()==decode_mmp(hb).convert('RGBA').tobytes()==decode_mmp(rb).convert('RGBA').tobytes();assert T['Textures'][i]==r['texture'];aliases=[];ims={}
 for rid in [i,i|0x1000000]:
  try:data=release.read(rid)
  except KeyError:aliases.append({'resource_id':rid,'present':False,'unknown':True});continue
  a=decode_mmp(data).convert('RGBA');v=np.array(a);hist=Path(f'G:/SS/Silent-Storm/Complete/Textures/{rid}');hd=hist.read_bytes()if hist.exists()else None;assert hd is None or hd==data;ims[rid]=a
  png=out/'sources'/f'{i}-resource-{rid}-fullRGBA.png';rgb=out/'sources'/f'{i}-resource-{rid}-nativeRGB.png';ap=out/'sources'/f'{i}-resource-{rid}-nativeA.png';a.save(png);a.convert('RGB').save(rgb);a.getchannel('A').save(ap)
  aliases.append({'resource_id':rid,'present':True,'native_size':list(a.size),'native_RGBA_SHA256':hs(a.tobytes()),'payload_SHA256':hs(data),'historical_payload_SHA256':hs(hd)if hd is not None else None,'historical_release_bytes_exact':hd==data if hd is not None else None,'historical_RGBA_exact':decode_mmp(hd).convert('RGBA').tobytes()==a.tobytes()if hd is not None else None,'fullRGBA_PNG':rel(png),'fullRGBA_PNG_SHA256':sha(png),'nativeRGB_PNG':rel(rgb),'nativeRGB_PNG_SHA256':sha(rgb),'nativeA_PNG':rel(ap),'nativeA_PNG_SHA256':sha(ap),'source_RGBA_extrema':[list(z)for z in a.getextrema()],'fullRGBA_constant':bool(np.all(v==v[0,0])),'A0_useful_RGB_count':int(((v[:,:,3]==0)&(v[:,:,:3].max(2)>0)).sum()),'native_RGB_distinct_count':len(np.unique(v[:,:,:3].reshape(-1,3),axis=0)),'exact_accepted_donors':donors.get(str(a.size)+hs(a.tobytes()),[])})
 actual=[]
 for m in T['Materials'].values():
  if m['TextureID']!=i:continue
  tid=m['TemplateID'];cons=[]
  for model in T['Models'].values():
   for f,x in model.items():
    if re.fullmatch(r'Material\d+',f)and x==tid:cons.append({'field':f,'model':ref('Models',model['ID']),'model_template':ref('ModelTemplates',model['TemplateID']),'geometry':ref('Geometries',model['GeometryID'])})
  actual.append({'material':ref('Materials',m['ID']),'material_template':ref('MaterialTemplates',tid),'Alpha':m['Alpha'],'AddressMode':m['AddressMode'],'Models':cons,'Races':[ref('Races',v['ID'])for v in T['Races'].values()if v.get('MaterialID')==tid],'Heads':[ref('Heads',v['ID'])for v in T['Heads'].values()if v.get('MaterialID')==tid]})
 assert actual,(i,'missingactualMaterial')
 fmt=T['Textures'][i]['Format'];isDXT=fmt[:3]=='dxt';runtime=i|0x1000000 if isDXT else i;alt=ims.get(runtime);compare={}
 if len(ims)==2 and ims[i].size==ims[i|0x1000000].size:
  aa=np.array(ims[i]);bb=np.array(ims[i|0x1000000]);d=np.abs(aa.astype(int)-bb.astype(int));compare={'different_RGB_pixels':int(np.any(aa[:,:,:3]!=bb[:,:,:3],2).sum()),'different_A_pixels':int((aa[:,:,3]!=bb[:,:,3]).sum()),'max_absolute_RGBA_delta':d.max((0,1)).tolist()}
 donor=any(a.get('exact_accepted_donors')for a in aliases);service=qids.get(i,{}).get('status')in ['generation-unavailable','imagegen-unavailable','generation-refused'];status='exact-accepted-donor-pending-root-reuse'if donor else'actual-service-unavailable'if service else'pending-art-review'
 entry.update(initial_status=status,logical_size=list(o.size),source_RGBA_SHA256=hs(o.tobytes()),original_PNG=rel(op),original_PNG_SHA256=sha(op),aliases=aliases,cross_alias_comparison=compare,actual_native_Type=T['Textures'][i]['Type'],actual_Format=fmt,CTexture_Import_bIsDXT=isDXT,resource_when_gfx_texture_usedxt_0=runtime,actual_material_consumers=actual,source_history_release_original_exact=True,both_aliases_privately_viewed=False,source_weakcount_phase_proof='Pending private whole nativeRGB/A/NN inspection per ID, not inferred hardware from source name.',runtime_mesh_UV_unknown=True)
 proof.append(entry);selected.append(r)
# Compact unique DB records and full source-code hashes; no raster arrays embedded in JSON.
save(f'actual-records-{ST}.json',{'fresh_DB_path':DB.as_posix(),'fresh_DB_SHA256':sha(DB),'tables':common,'record_SHA256':hashes,'canonical_recipe':'UTF8 sorted JSON separators comma/colon'})
code=[]
for n in ['DBFormat/DataFormat.cpp','Main/GTexture.cpp','Main/LSHead.cpp','assets/terrain-hd/prepare_sources.py','assets/terrain-hd/match_brightness.py']:
 p=ROOT/n;ls=p.read_text(encoding='utf-8').splitlines();code.append({'path':n,'complete_file_SHA256':sha(p),'relevant_exact_lines':[{'line':k+1,'text':s}for k,s in enumerate(ls)if any(t in s for t in ['bIsDXT =','GetRealTextureID','gfx_texture_usedxt','pad_rgb_under_source_mask','def match_cell','ImportField( "TextureID"','ImportField( "TemplateID"'])]})
save(f'consumer-code-proof-{ST}.json',code);save(f'selected-{ST}.json',selected);save(f'source-check-{ST}.json',proof);save(f'accepted-donor-audit-{ST}.json',donorrows);save(f'guards-{ST}.json',{'source_entry_count':len(S),'queue_count':len(Q),'source_SHA256':sha(B.parent/'sources.json'),'queue_SHA256':sha(B/'queue.json'),'source_entry_status_counts':{s:sum(r.get('status')==s for r in S)for s in sorted({str(r.get('status'))for r in S})},'sourcequeue_total':len(sq),'sourcequeue_already_sources':len(sq)-len(pending),'remaining_scope_count':len(pending),'policy':'Whole clothing category. Accepted/donors never generated; previous artistic held/raw available for versioned repair, frozen old proofs never edited. Actual service refusals/unavailable retained. Current root state not automatically published.', 'shared_writes':False})
# Survey all native source paint in private groups. Native reference is not enlarged for tool.
for start in range(0,len(proof),12):
 canvas=Image.new('RGB',(4*280,3*300),(28,28,28));draw=ImageDraw.Draw(canvas)
 for k,e in enumerate(proof[start:start+12]):
  if not e.get('aliases'):continue
  a=e['aliases'][0];im=Image.open(ROOT/a['nativeRGB_PNG']);scale=min(1,256/max(im.size));im=im.resize((int(im.width*scale),int(im.height*scale)),Image.Resampling.NEAREST);x=k%4*280;y=k//4*300;canvas.paste(im,(x,y+28));draw.text((x,y),f"{e['id']} {e['logical_size']} oldraw{len(e['prior_history']['raws'])}",fill='white')
 canvas.save(out/f'survey-{start:03}.png')
print(json.dumps({'source_entries':len(S),'queue':len(Q),'pending_scope':len(proof),'status_counts':{s:sum(e['initial_status']==s for e in proof)for s in sorted({e['initial_status']for e in proof})},'old_raw_IDs':sum(bool(e['prior_history']['raws'])for e in proof),'uniqueDBrows':sum(len(v)for v in common.values()),'no_calls_yet':True},indent=2))

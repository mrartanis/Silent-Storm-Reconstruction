from pathlib import Path
import sys,json,hashlib,struct
import numpy as np
from PIL import Image,ImageDraw
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-fifty-fifth';sys.path.insert(0,str(B.parent))
from prepare_sources import ReleaseResources,decode_mmp
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def payloadsha(data):return hashlib.sha256(data).hexdigest()
def rel(p):return p.relative_to(ROOT).as_posix()
def save(n,v,compact=False):(B/n).write_text(json.dumps(v,ensure_ascii=False,**({'separators':(',',':')}if compact else {'indent':2}))+'\n',encoding='utf-8')
out=B/f'private-{ST}/before-aliases';out.mkdir(parents=True,exist_ok=True);before=B/f'source-check-{ST}.json';beforebytes=before.read_bytes();release=ReleaseResources('G:/SS/lab/baseline/res');sources=read(before);rows=[];matrices={};actual=read(B/f'actual-records-{ST}.json')
for r in sources:
 i=r['id'];aliases=[];ims={}
 for rid in [i,i|0x01000000]:
  try:data=release.read(rid)
  except KeyError:
   aliases.append({'resource_id':rid,'present_in_baseline':False,'unresolved':True});continue
  a=decode_mmp(data).convert('RGBA');v=np.array(a);hist=Path(f'G:/SS/Silent-Storm/Complete/Textures/{rid}');hd=hist.read_bytes()if hist.exists()else None;loc=release.index.get(rid);loose=release.root/'Textures'/str(rid)
  nativeRGBA=payloadsha(a.tobytes());fullRGBAconstant=len(np.unique(v.reshape(-1,4),axis=0))==1
  entry={'logical_id':i,'resource_id':rid,'alias_kind':'compressed/resource_i'if rid==i else 'uncompressed/resource_i_OR_0x01000000','baseline_locator':{'loose':str(loose).replace('\\','/')if loose.exists()else None,'archive':str(loc[0]).replace('\\','/')if loc else None,'offset':loc[1]if loc else None,'size':loc[2]if loc else len(data)},'baseline_payload_bytes':len(data),'baseline_payload_SHA256':payloadsha(data),'native_header_signature_format_average_width_height_mips':list(struct.unpack_from('<6I',data)),'native_RGBA_SHA256':nativeRGBA,'native_size':list(a.size),'RGBA_extrema':[[int(v[:,:,k].min()),int(v[:,:,k].max())]for k in range(4)],'native_RGB_distinct_count':len(np.unique(v[:,:,:3].reshape(-1,3),axis=0)),'native_RGBA_distinct_count':len(np.unique(v.reshape(-1,4),axis=0)),'fullconstantRGBA':fullRGBAconstant,'constantRGBA':v[0,0].tolist()if fullRGBAconstant else None,'A0_pixel_count':int((v[:,:,3]==0).sum()),'A0_useful_RGB_nonzero_pixel_count':int(((v[:,:,3]==0)&(v[:,:,:3].max(2)>0)).sum()),'historical_alias_locator':str(hist).replace('\\','/'),'historical_alias_present':hist.exists(),'historical_payload_SHA256':payloadsha(hd)if hd is not None else None,'historical_baseline_payload_exact':hd==data if hd is not None else None,'historical_baseline_RGBA_exact':decode_mmp(hd).convert('RGBA').tobytes()==a.tobytes()and decode_mmp(hd).size==a.size if hd is not None else None,'originalPNG_exact_this_alias':Image.open(B/f'original/{i}.png').convert('RGBA').size==a.size and Image.open(B/f'original/{i}.png').convert('RGBA').tobytes()==a.tobytes()}
  rgbpath=out/f'{i}-resource-{rid}-native-opaque-storedRGB.png';apath=out/f'{i}-resource-{rid}-native-A.png';a.convert('RGB').save(rgbpath);a.getchannel('A').save(apath);a.convert('RGB').resize((512,512),Image.Resampling.NEAREST).save(out/f'{i}-resource-{rid}-storedRGB-private-NN512.png');a.getchannel('A').resize((512,512),Image.Resampling.NEAREST).save(out/f'{i}-resource-{rid}-A-private-NN512.png');entry.update(private_RGB=rel(rgbpath),private_A=rel(apath));ims[rid]=a
  if rid==i:
   assert entry['originalPNG_exact_this_alias'] and nativeRGBA==r['source_rgba_sha256'];entry['complete_native_matrix_ref']=r['native_matrix_ref']
  else:matrices[str(rid)]={'logical_id':i,'resource_id':rid,'source_native_size':list(a.size),'complete_original_alias_RGBA_matrix':v.tolist(),'native_RGBA_SHA256':nativeRGBA}
  aliases.append(entry)
 equal={}
 if len(ims)==2:
  aa=np.array(ims[i]);bb=np.array(ims[i|0x01000000]);equal={'dimensions_equal':ims[i].size==ims[i|0x01000000].size,'full_RGBA_equal':ims[i].size==ims[i|0x01000000].size and ims[i].tobytes()==ims[i|0x01000000].tobytes(),'full_payload_equal':aliases[0]['baseline_payload_SHA256']==aliases[1]['baseline_payload_SHA256']}
  if equal['dimensions_equal']:
   d=np.abs(aa.astype(int)-bb.astype(int));equal.update(different_RGBA_pixels=int(np.any(aa!=bb,axis=2).sum()),different_RGB_pixels=int(np.any(aa[:,:,:3]!=bb[:,:,:3],axis=2).sum()),different_A_pixels=int((aa[:,:,3]!=bb[:,:,3]).sum()),mean_absolute_delta_RGBA=d.mean((0,1)).tolist(),max_absolute_delta_RGBA=d.max((0,1)).tolist())
 texture=actual['tables']['Textures'][str(i)];fmt=texture['Format'];isDXT=fmt[:3]=='dxt'
 rows.append({'id':i,'aliases':aliases,'cross_alias_comparison':equal,'actual_Texture_Format':fmt,'CTexture_Import_bIsDXT':isDXT,'resolved_resource_if_gfx_texture_usedxt_0':i|0x01000000 if isDXT else i,'actual_texture_record_SHA256':actual['record_SHA256']['Textures'][str(i)],'phase':'Separate BEFORE BOTHalias audit alongside canonical compressed sourceproof; never observedactualruntime pixels from worker.','actual_consumers_from_before_refs':r['common_actual_records'],'runtime_alias0_from_parent':'Root reported gfx_texture_usedxt=0; GetRealTextureID uses high-bit alias only when CTexture.bIsDXT. Actual Format and import predicate resolved per row. Worker did not run Game or measure GPU.'})
save(f'before-alias-native-matrices-{ST}.json',{'sources':matrices,'qualification':'Only alternative uncompressed alias completeRGBA stored once; compressed/source_i references unchanged common BEFORE native matrix. Native pixel meanings not classified from resourceName or A.'},True)
mp=B/f'before-alias-native-matrices-{ST}.json'
for r in rows:
 for a in r['aliases']:
  if a.get('resource_id')in [int(k)for k in matrices]:a['complete_native_matrix_ref']={'path':rel(mp),'file_SHA256':sha(mp),'source_key':str(a['resource_id'])}
code=ROOT/'Main/GTexture.cpp';ls=code.read_text(encoding='utf-8').splitlines();imp=ROOT/'DBFormat/DataFormat.cpp';il=imp.read_text(encoding='utf-8').splitlines();codeproof={'path':rel(code),'complete_file_SHA256':sha(code),'exact_lines':[{'line':k+1,'text':s}for k,s in enumerate(ls)if k<32 or any(z in s for z in ['gfx_texture_usedxt','GetRealTextureID( pTex )'])],'CTexture_Format_import':{'path':rel(imp),'complete_file_SHA256':sha(imp),'exact_lines':[{'line':k+1,'text':s}for k,s in enumerate(il)if 949<=k<=975]},'qualification':'Current exact dispatch code: gfx_texture_usedxt true uses i; false AND bIsDXT uses i|0x01000000; false AND NOT bIsDXT retains i. CTexture::Import bIsDXT = Format.substr(0,3)==dxt proved separately and applied per actual DB row. Runtime state supplied by root report, not measured by worker.'}
save(f'before-runtime-alias-code-{ST}.json',codeproof)
save(f'before-alias-proof-{ST}.json',{'batch':ST,'phase':'BEFORE bothaliases independent native parity proof, before any toolcall','source_before_call_ref':{'path':rel(before),'file_SHA256':sha(before)},'actual_GTexture_dispatch_code_ref':{'path':rel(B/f'before-runtime-alias-code-{ST}.json'),'file_SHA256':sha(B/f'before-runtime-alias-code-{ST}.json')},'rows':rows,'not_runtimevalidated':True,'shared_writes':False,'no_retry_no_RGBrepair':True,'qualification':'Historical/release parity checked separately per actualresource alias; missing historical alias honestunknown. BOTH payload/nativeRGBA/dims/alpha independently audited; never assert actualruntimeRGBA equals compressed/source_i merely because originalPNG did.'})
for start in range(0,len(rows),6):
 rgb=Image.new('RGB',(2*280,6*290),(24,24,24));al=Image.new('RGB',rgb.size,(24,24,24));d=ImageDraw.Draw(rgb);ad=ImageDraw.Draw(al)
 for k,r in enumerate(rows[start:start+6]):
  for j,a in enumerate(r['aliases']):
   if not a.get('present_in_baseline',True):continue
   x=j*280;y=k*290;im=Image.open(ROOT/a['private_RGB']);alpha=Image.open(ROOT/a['private_A']);assert im.width<=280 and im.height<=267;rgb.paste(im,(x,y+23));al.paste(alpha.convert('RGB'),(x,y+23));d.text((x,y),f"ID{r['id']} resource{a['resource_id']}",fill='white');ad.text((x,y),f"ID{r['id']} A{a['RGBA_extrema'][3]}",fill='white')
 rgb.save(out/f'all-alias-storedRGB-{start}.png');al.save(out/f'all-alias-A-{start}.png')
assert before.read_bytes()==beforebytes
print(json.dumps([{'id':r['id'],'same':r['cross_alias_comparison'],'aliases':[{'resource':a['resource_id'],'format':a.get('native_header_signature_format_average_width_height_mips',[None,None])[1],'histParity':a.get('historical_baseline_payload_exact'),'A':a.get('RGBA_extrema',[[],[],[],[]])[3],'RGBcount':a.get('native_RGB_distinct_count')}for a in r['aliases']]}for r in rows],indent=2))

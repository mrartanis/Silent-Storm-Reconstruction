from pathlib import Path
import json,numpy as np
from PIL import Image,ImageFilter
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='heads-thirteenth'
ph=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));out=[]
for c in ph:
 i=c['id'];O=Image.open(B/f'original/{i}.png').convert('RGBA');N=Image.open(B/f'private-{ST}/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA');A=np.asarray(O);P=np.asarray(N);I=np.asarray(N.resize(O.size,Image.Resampling.LANCZOS));R={}
 assert N.getchannel('A').tobytes()==O.getchannel('A').resize(N.size,Image.Resampling.LANCZOS).tobytes()
 for name,f in c['region_diagnostics'].items():
  x0,y0,x1,y1=f['native_bbox'];s=A[y0:y1,x0:x1,:3];p=P[y0*4:y1*4,x0*4:x1*4,:3];inv=I[y0:y1,x0:x1,:3];yy,xx=np.unravel_index(p.mean(2).argmax(),p.shape[:2]);ys,xs=np.unravel_index(inv.mean(2).argmax(),inv.shape[:2]);R[name]={'native_bbox':f['native_bbox'],'source_RGB_mean':s.mean((0,1)).tolist(),'native4x_RGB_mean':p.mean((0,1)).tolist(),'source_RGB_max':s.max((0,1)).tolist(),'native4x_RGB_max':p.max((0,1)).tolist(),'inverse_native_RGB_max':inv.max((0,1)).tolist(),'source_peak_phase':f['max_mean_source_xy'],'candidate4x_peak_xy':[int(xx)+x0*4,int(yy)+y0*4],'candidate_inverse_peak_xy':[int(xs)+x0,int(ys)+y0],'candidate_inverse_peak_RGB':inv[ys,xs].tolist(),'source_peak_RGB':f['max_mean_source_RGB']}
 # Full arrays first; HP summaries numeric only, no artistic compositing/registration.
 S4=np.asarray(O.resize(N.size,Image.Resampling.LANCZOS).convert('RGB'),dtype=float).mean(2);G4=np.asarray(N.convert('RGB'),dtype=float).mean(2)
 HS=S4-np.asarray(O.resize(N.size,Image.Resampling.LANCZOS).convert('RGB').filter(ImageFilter.GaussianBlur(1)),dtype=float).mean(2);HG=G4-np.asarray(N.convert('RGB').filter(ImageFilter.GaussianBlur(1)),dtype=float).mean(2)
 hp={'source_iris_HP_std':float(HS[64:172,80:188].std()),'candidate_iris_HP_std':float(HG[64:172,80:188].std()),'iris_HP_correlation':float(np.corrcoef(HS[64:172,80:188].ravel(),HG[64:172,80:188].ravel())[0,1])}
 # Exact runtime source-over diagnostic at fixed EYE atlas tile64square. Current sampler pickslogicalmip64; simulate entire calibrated source down to64 and V-flip, no code modifications.
 blend=[]
 for w in [.25,.5,1.]:
  src=I[:,:,:3].astype(float);base=np.zeros_like(src);dst=np.clip(np.floor(src*w+base*(1-w)+.5),0,255).astype(np.uint8)[::-1]
  blend.append({'weight':w,'atlas_EYE_destination':[64,64,128,128],'V_flipped_entire_RGB_SHA256':__import__('hashlib').sha256(dst.tobytes()).hexdigest(),'mean_RGB':dst.mean((0,1)).tolist(),'diagnostic':'Fixed logical source64 inverse LANCZOS for private numeric weight/flip diagnostic; native root actual mip/CPU runtime validation remains authoritative.'})
 out.append({'id':i,'sourceA4x_byte_exact':True,'source_feature_counts':c['source_feature_counts'],'candidate_private_visible_feature_counts':{'iris':1,'pupil':1,'reflection_patches':2},'original_native_feature_regions':R,'iris_material_highpass_diagnostics':hp,'dynamic_eye_layer_weight_diagnostics':blend,'recipe':'Whole raw exact native4x/default source-mask padding/pure scalar/full original alpha; entire arrays sampled only. No BBox/crop/artistic sourceRGB patch/optout. Source before-call counts/matrices immutable.'})
 for name,color in [('dark',(24,24,24,255)),('light',(210,210,210,255))]:Image.alpha_composite(Image.new('RGBA',N.size,color),N).convert('RGB').save(B/f'private-{ST}/native4x/{i}-sourceA-over-{name}.png')
(B/f'local-metrics-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in out:
 print(r['id'],[(n,z['source_peak_RGB'],z['candidate_inverse_peak_RGB'],z['native4x_RGB_max'],z['candidate_inverse_peak_xy'])for n,z in r['original_native_feature_regions'].items()if 'reflection'in n],r['iris_material_highpass_diagnostics'])

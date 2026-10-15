"""Single-coordinator whole-menu jobs/import and production projection check."""
from pathlib import Path
import hashlib,json,sys
from PIL import Image
R=Path(__file__).resolve().parents[3];H=R/'assets/terrain-hd';W=H/'expanded/mainmenu-complete-20261015';sys.path.insert(0,str(H))
from prepare_sources import prepare,decode_mmp
from import_generated import import_jobs
from match_brightness import main as calibrate
from build_pack import mmp
read=lambda p:json.loads(Path(p).read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def rel(p):return p.resolve().relative_to(R).as_posix()
def save(p,v):
 data=(json.dumps(v,ensure_ascii=False,indent=2)+'\n').encode()
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
audit=read(W/'source-audit.json');rows={r['id']:r for r in audit['records']};ids=sorted(i for i,r in rows.items() if r['status']=='artwork-pending')
versions={i:1 for i in ids};versions.update({4656:3,4657:4,4662:3,5256:2,5257:2})
if sys.argv[1]=='freeze':
 visual=read(W/'artwork-visual-review-final.json');assert visual['artwork_ready_IDs']==ids and visual['eligible_art_unprocessed']==0
 jobs=[]
 for i in ids:
  v=versions[i];stem=f'{i}-mainmenu-complete-20261015-v{v}';a=read(W/'calls'/f'{stem}-arguments.json');tool=read(W/'calls'/f'{stem}-tool-result.json');raw=W/'raw'/f'{stem}-raw.png'
  suffix='-source-stored' if i==4801 else '';numeric=W/'reviews'/f'{stem}{suffix}-numeric.json';n=read(numeric)
  assert n['raw_SHA256']==sha(raw) and n['source_alpha_LANCZOS4x_exact'];assert (W/'calls'/f'{stem}-prompt.txt').read_bytes()==a['prompt'].encode()+b'\n'
  j={'id':i,'generated':rel(raw),'prompt':a['prompt'],'generation_variant':f'mainmenu-complete-20261015-v{v}'+('-stored' if i==4801 else ''),'reference_pngs':a['referenced_image_paths'],'actual_generation_call':rel(W/'calls'/f'{stem}-tool-result.json'),'actual_arguments':rel(W/'calls'/f'{stem}-arguments.json'),'BEFORE':rel(W/'calls'/f'{stem}-BEFORE.json'),'raw_SHA256':sha(raw),'whole_native_forecast_RGBA_SHA256':n['native_RGBA_SHA256'],'category_artwork_proof':rel(W/'artwork-visual-review-final.json'),'numeric_forecast_proof':rel(numeric)}
  if i in (4662,4727,4728,4764):
   j.update(source_mask_rgb_padding=False,source_mask_rgb_padding_reason='Individually inspected actual ordinary diffuse atlas: full selected A is authored auxiliary/material control, not a cutout to erase painted RGB. Preserve the complete generated RGB, including original painted RGB where A is zero; restore the complete original A independently. No RGB patching or alpha-derived filling.')
  if i==4801:
   proof=W/'independent-4801-encoding-purpose-review.json';assert proof.is_file()
   j.update(alpha_encoding='source-stored',alpha_encoding_reason='Individual actual Mat3054 AlphaTransparent + CTransparentMaterial/GTransparent SEffTexture TextureCopyAlpha sourceONE/destINVSRCALPHA and loader proof: source8888 RGB is already authored native contribution; full A is X-constant vertical background fade, not a further RGB multiplication. Generated whole RGB guide restoration uses the same stored domain. See '+rel(proof)+' SHA256 '+sha(proof),source_mask_rgb_padding=False,source_mask_rgb_padding_reason='The original full RGB is painted cloud contribution independent of the authored vertical A fade; do not mask or fill RGB by A. Retain original source-stored contribution and restore full source A independently.')
  jobs.append(j)
 save(W/'jobs-final.json',jobs)
 candidates=read(W/'candidates.json')
 for i in (4664,4665,5600,5601):
  r=rows[i];candidates.append({'texture':r['texture'],'category':'interface-v5','role':'original-technical-control','source_resource_id':r['actual_selected_resource'],'source_resource_selection_reason':r['selection_proof'],'usage':{'direct_texture_uses':r['uses']}})
 save(W/'coordinator-candidates.json',candidates)
 save(W/'artwork-final.json',{'status':'artwork-complete-awaiting-whole-build','scope':33,'accepted_preserved':9,'new_artwork_ready':20,'artwork_ready_IDs':ids,'technical_original_IDs':[4664,4665,5600,5601],'eligible_art_unprocessed':0,'fonts_and_text_renderer_excluded':True,'jobs':rel(W/'jobs-final.json'),'jobs_SHA256':sha(W/'jobs-final.json'),'visual_review':rel(W/'artwork-visual-review-final.json')})
 print('Whole33 artwork frozen:20 ready,9 accepted preserved,4 individually technical original; no eligible artwork pending.')
elif sys.argv[1]=='import':
 old=read(R/'build/hd-before-mainmenu-complete-20261015-groups-sources.json');assert read(H/'sources.json')==old
 queue=read(H/'expanded/queue.json');assert len(queue)==3320 and set(ids).isdisjoint(x['id'] for x in queue)
 assert not (W/'coordinator-prepared.json').exists()
 prepared=prepare(read(W/'coordinator-candidates.json'),'G:/SS/Silent-Storm/Complete/Textures','G:/SS/lab/baseline/res',R/'build/hd-mainmenu-prepared-intermediate.json',append=False)
 assert len(prepared)==24
 for p in prepared:
  if p['id'] in (4664,4665,5600,5601):p.update(status='original-technical-map',reason='Individual typed DB binding: '+('; '.join(x['table']+'.'+x['field'] for x in rows[p['id']]['uses']))+' is bump/normal or gloss material control, not menu color artwork. Source retained through original fallback; see source-audit.json.')
  else:assert p['status']=='pending',p
 save(W/'coordinator-prepared.json',prepared)
 (H/'expanded/queue.json').write_text(json.dumps(queue+prepared,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 jobs=read(W/'jobs-final.json');import_jobs(jobs);calibrate(set(ids))
 current={r['id']:r for r in read(H/'sources.json')['textures']};assert len(current)==2837
 for r in old['textures']:assert current[r['id']]==r
 print('Single import+scalar calibration20 complete; old2817 source objects unchanged.')
elif sys.argv[1]=='projection':
 sources={r['id']:r for r in read(H/'sources.json')['textures']};records=[]
 for j in read(W/'jobs-final.json'):
  r=sources[j['id']];im=Image.open(H/r['png']).convert('RGBA');src=Image.open(H/r['reference_png']).convert('RGBA');native=decode_mmp(mmp(im,r.get('alpha_encoding')=='premultiplied',src if r.get('native_alpha_reference') else None));h=hashlib.sha256(native.tobytes()).hexdigest();assert h==j['whole_native_forecast_RGBA_SHA256'],j['id']
  records.append({'id':j['id'],'production_forecast_native_RGBA_exact':True,'RGBA_SHA256':h,'balanced_PNG_SHA256':sha(H/r['png']),'scalar_calibration':r['brightness_calibration']})
 save(W/'production-projection-check.json',{'all20_exact_artist_reviewed_forecasts':True,'records':records})
 print('All20 actual production RGB/full sourceA match reviewed forecasts exactly.')

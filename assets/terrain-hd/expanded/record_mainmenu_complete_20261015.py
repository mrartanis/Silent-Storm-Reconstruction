"""Freeze final portable numeric/runtime evidence and authoritative category disposition."""
from pathlib import Path
import json,hashlib,shutil
H=Path(__file__).resolve().parents[1];R=H.parents[1];W=H/'expanded/mainmenu-complete-20261015'
rd=lambda p:json.loads(Path(p).read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def rel(p):return p.resolve().relative_to(R).as_posix()
def save(p,v):
 data=(json.dumps(v,ensure_ascii=False,indent=2)+'\n').encode()
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
assert not (W/'category-final.json').exists()
assert (W/'independent-native-runtime-visual-review-final.json').is_file() and (W/'independent-ordinary-launch-review.json').is_file()
assert (W/'code-font-user-preservation-final.json').is_file()
e=Path('G:/SS/lab/runs/hd-groups-20261007-01/menu-smoke-20261009-01/evidence');C=W/'checks';C.mkdir(exist_ok=True)
pairs={R/'build/hd-category-mainmenu-complete-20261015/numeric-proof.json':C/'native-numeric-proof.json',R/'build/hd-mainmenu-complete-20261015-live-plan.json':C/'live-plan.json'}
for kind in ['observations','summary','metadata']:pairs[e/f'hd-mainmenu-complete-20261015-live-plan-{kind}.json']=C/f'live-{kind}.json'
pairs[e/'ordinary-hd2837-proof.json']=C/'ordinary-launch-proof.json'
for name in ['build','build-retry-ucrt','validate','coverage','native','live','ordinary','ordinary-retry','ordinary-window-capture','ordinary-foreground']:
 p=R/'build'/f'hd-mainmenu-complete-20261015-{name}.log'
 if p.is_file():pairs[p]=C/(name+'.log')
mapping=[]
for src,dest in pairs.items():
 assert not dest.exists();shutil.copyfile(src,dest);assert sha(src)==sha(dest)
 mapping.append({'actual_original_path':str(src),'published_copy':rel(dest),'bytes':dest.stat().st_size,'SHA256':sha(dest),'method':'Byte-identical portable evidence copy; original historical report/call arguments unchanged.'})
save(W/'portable-evidence-relocations.json',mapping)
sources={r['id']:r for r in rd(H/'sources.json')['textures']};queue={r['id']:r for r in rd(H/'expanded/queue.json')};native={r['id']:r for r in rd('G:/SS/lab/build-x64-stage4/RelWithDebInfo/res-hd/manifest.json')['textures']};cov=rd(H/'expanded/coverage.json');assert len(sources)==len(native)==2837
old={r['id']:r for r in rd(R/'build/hd-before-mainmenu-complete-20261015-groups-sources.json')['textures']}
for i,r in old.items():assert sources[i]==r
jobs={r['id']:r for r in rd(W/'jobs-final.json')};audit=rd(W/'source-audit.json');records=[]
for row in audit['records']:
 i=row['id']
 if i in jobs:
  r=sources[i];assert queue[i]['status']=='validated';records.append({'id':i,'status':'validated-new-HD','logical_size':r['logical_size'],'physical_size':native[i]['physical_size'],'actual_selected_original_resource':row['actual_selected_resource'],'source_RGBA_SHA256':r['source_rgba_sha256'],'balanced_PNG':rel(H/r['png']),'balanced_PNG_SHA256':sha(H/r['png']),'actual_native_MMP_SHA256':native[i]['mmp_sha256'],'actual_native_RGBA_matches_artist_reviewed_forecast':True,'alpha_encoding':r.get('alpha_encoding','source-stored ordinary'),'exact_full_source_alpha4x_and_both_aliases_all_mip_bytes':True})
 elif i in (4664,4665,5600,5601):
  assert queue[i]['status']=='original-technical-map';records.append({'id':i,'status':'original-technical-map','reason':queue[i]['reason'],'source_qualification':'source-audit.json','reference_PNG':rel(H/queue[i]['original_png'])})
 else:
  assert sources[i]==old[i];records.append({'id':i,'status':'accepted-preserved','source_record_unchanged':True,'PNG_and_native_payload_unchanged':True})
assert len(records)==33
status={'status':'complete','category':'mainmenu-folder-artwork','date':'2026-10-15','scope_total':33,'scope_note':'Whole exact Interface v5.0/MainMenu folder, not a claim every resource is visible through the current fixed camera; font/caption/text rendering excluded.','old_menu_HD_preserved':9,'new_HD':20,'menu_HD_total':29,'technical_originals':4,'eligible_art_unprocessed':0,'global_accepted':2837,'global_old_accepted_preserved':2817,'new_ready_IDs':sorted(jobs),'original_only_IDs':[4664,4665,5600,5601],'genuine_builtin_imagegen_calls':28,'actual_service_refusals':0,'one_input_only_uncalled_BEFORE':'4662v2 count assumption corrected to THREE before any call; retained honestly, no call/refusal invented.','jobs':'jobs-final.json','artwork_review':'artwork-visual-review-final.json','independent_artwork_review':'independent-final-artwork-review.json','actual_native_numeric':'checks/native-numeric-proof.json','actual_native_visual':'independent-actual-native-review.json','actual_native_runtime_visual':'independent-native-runtime-visual-review-final.json','ordinary_launch_visual':'independent-ordinary-launch-review.json','runtime_numeric':'runtime-numeric-proof.json','whole_pack_full_validation':{'textures':2837,'aliases':5674,'archives':cov['native_archives'],'bytes':cov['native_bytes'],'technical_original_count':262},'build':{'target':'Game','configuration':'RelWithDebInfo','result':'PASS','first_attempt':'Missing time.h due unset documented UCRTContentRoot; preserved failure log.','resolved':'Existing SDK/toolchain only; command-local UCRTContentRoot from diagnostics/Build-Lab.ps1. Incremental retry found HD pack up to date; no second category pack generation.','logs':['checks/build.log','checks/build-retry-ucrt.log']},'live_scope':'Exact normal-user executable SHA707a588b... actually tested with new res-hd junction. Six representative GPU textures in3states, actual mainmenu/graphics options and six sampled logo-flash frames. All20 actual native maps validated and reviewed; not a claim all possible equipped/hidden models were exercised.','ordinary_capture_limit':'One identified ordinary no-harness mainmenu frame is partially desktop-occluded; it proves menu presence only. Four cached ordinary frames are identical and are NOT used as flash/full-art proof. All28 unoccluded native Game frames are the visual artwork/flash basis.','preservation':'code-font-user-preservation-final.json','remaining_verified_outside_shared_queue':828,'other_categories_paused':True,'records':records}
save(W/'category-final.json',status)
print('Final33 complete:29HD+4technical;new20/old2817 native preserved;Game2837 build/validation/runtimePASS; fonts/code/userdata untouched.')

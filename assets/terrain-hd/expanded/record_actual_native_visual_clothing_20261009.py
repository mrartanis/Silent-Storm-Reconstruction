from pathlib import Path
import json,hashlib
B=Path(__file__).resolve().parent;ROOT=B.parents[2];N=ROOT/'build/hd-category-clothing-complete-20261008'
def ref(p):return {'path':p.relative_to(ROOT).as_posix(),'SHA256':hashlib.sha256(p.read_bytes()).hexdigest()}
ids=[2011,3313,6238,2933,2235,6833]
findings={
2011:'Actual complete native canvas retains brown/slate diffuse camouflage, plain left rectangle without stitch dots, original right binding pitch/positions and two bottom oval paint marks. Original weak y64/y74 fragments more defined exactly as independently qualified for v9; no new stripe positions or material change.',
3313:'Actual full native retains smooth broad brown/slate-blue camouflage and black/gray mechanical paint domains, existing openings/panels/wing paint and5005/021 stencils. v5 has original digits and source diffuse paint without earlier excessive stipple.',
6238:'Actual upside-down face map preserves complete UV orientation, single ear/top fields, closed pink mouth, two dim eye fields and lower connected brown hair tips. Existing799 raw candidate independently recalibrated to6238 complete source A; finer original hair painting is qualified, no new anatomy or accepted799 production copy.',
2933:'Actual whole armor skin retains existing metal/weathered blue fields, black openings, original stencil23, glyph/edge patterns and source layout. More defined existing wear footprints retained as prior qualification; no new panel/label/number.',
2235:'Actual entire olive/cream uniform atlas retains six source button dots, five right belt loops, two top pocket borders, clipped black gaps and original watch/binding fields. Fine paint/folds clearer within old material; source counts/normalized UV remain.',
6833:'Actual whole tail coat canvas retains original subdued dark cloth, plaid central field, original repeated binding phase, pale top arcs as painted marks and lower amber flatfield/old clipping. No granular metal reinterpretation or extra fasteners from rejected old attempt.'}
out=B/'actual-native-visual-review-clothing-complete-20261008-20261009.json';assert not out.exists()
report={'stage':'native-artwork-transfer','status':'pass-native-awaiting-single-runtime-review','reviewer':'/root/clothing_complete',
 'all168_actual_native_RGBA_byte_equal_individually_artist_reviewed_whole_forecasts':True,
 'readonly_whole_transfer_proof':ref(B/'actual-native-transfer-review-clothing-complete-20261008-20261009.json'),
 'actual_native_export_source':'Coordinator hd-category-native-check.py reads ACTUAL packed release resources via ReleaseResources and decode_mmp(actual), not a prediction-only PNG. Primary/highbit whole MMP including every mip equals expected native, fresh source matches historical/release; previous2649 source/PNG/native preserved.',
 'direct_whole_actual_native_canvases_privately_viewed':[{'id':i,'file':ref(N/f'{i}-native.png'),'findings':findings[i]}for i in ids],
 'own104_direct_transfer_verified':True,'helper64_whole_transfer_verified_helper_independent_visual_stage_separate':True,
 'source_layout_full_alpha_material_and_pattern_scale_review_transferred_by_complete_RGBA_identity':True,
 'actual_Game_GPU_or_mesh_view_claim':False,'runtime_frames_pending':True,
 'new_generation_calls':0,'existing_frozen_shared_metadata_modified':False}
out.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(ref(out)))

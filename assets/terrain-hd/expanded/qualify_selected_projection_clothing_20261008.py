from pathlib import Path
import json,hashlib
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
p=B/f'individual-final-selected-source-projection-qualification-{ST}.json';assert not p.exists()
dif=read(B/f'selected-source-final-projection-differences-{ST}.json')['records'];rows=[]
for r in dif:
 i=r['id'];assert i in [3873,4555]
 reason=('Whole selectedUC16781089 RGB/A, full saved v1 raw and newly calibrated entire4x privately reviewed. Original two blue-black cloth fields, soft broad folds and faint horizontal top bands, narrow right gray field with one original upper gray oval paint/sole and old lower gray strip remain in full normalized UV. Fine matte modulation is more defined; no new folds, seams, ribs or hardware. Old primary-source projection is not relabeled UC. New selectedUC scalar0.90380859375 vs old0.8870849609375 changes RGB channel MAE about0.25/255, full A unchanged.' if i==3873 else 'Whole selectedUC16781771 RGB/A, full saved v1 raw and newly calibrated entire4x privately reviewed. Original two olive cloth fields/broad folds and narrow brown leather field keep native bounds, lower clipped black gaps, exact buckle paint count and original subdued diffuse material. Fine painted modulation and existing fold contrast stronger, explicitly retained as qualification; no new weave, seams, buckles or shifted panels. Old primary-source projection is not relabeled UC. New selectedUC scalar0.84423828125 vs old0.83966064453125 changes RGB MAE0.24/0.18/0.09, full A unchanged.')
 rows.append({'id':i,'status':'ready','selected_prediction_SHA256':r['prediction']['SHA256'],'new_global_gain':r['gain'],'old_vs_selected_RGBA_MAE':r['old_vs_selected_RGBA_MAE'],'whole_selected_RGB_A_raw_new_projection_privately_viewed':True,'new_generation_call':False,'reason':reason})
p.write_text(json.dumps({'status':'complete','records':rows,'old_BEFORE_call_raw_primary_projection_files_immutable':True,'artist_RGB_local_edits':False},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Qualified full selected-source recalibration', [r['id']for r in rows])

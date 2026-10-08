"""Readonly whole-category check against frozen worker RGB/fullselectedA forecasts."""
from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];HD=B.parent;ST='clothing-complete-20261008'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
sourcep=HD/'sources.json';assets={r['id']:r for r in read(sourcep)['textures']}
jobs=read(B/f'jobs-{ST}-final.json');q=read(B/f'whole-category-numeric-QA-{ST}-final.json');expected={r['id']:r for r in q['checks']}
helperids={r['id']for r in read(B/'jobs-clothing-garments-complete-20261008-FINAL.json')};rows=[]
for j in jobs:
 i=j['id'];a=assets[i];assert 'brightness_calibration' in a,(i,'production calibration not yet complete')
 assert a['generation_variant']==j['generation_variant']
 assert a['source_rgba_sha256']==j['source_selected_RGBA_SHA256'],(i,'selected source mismatch')
 original=Image.open(HD/a['reference_png']).convert('RGBA');assert hashlib.sha256(original.tobytes()).hexdigest()==j['source_selected_RGBA_SHA256']
 p=HD/a['png'];im=Image.open(p).convert('RGBA');target=Image.open(ROOT/expected[i]['prediction']['path']).convert('RGBA');assert im.size==target.size
 alpha=original.getchannel('A').resize(im.size,Image.Resampling.LANCZOS)
 native=im.convert('RGB').convert('RGBA');native.putalpha(alpha)
 exact=native.tobytes()==target.tobytes()
 nr=np.asarray(native).astype(int);tr=np.asarray(target).astype(int)
 row={'id':i,'worker':'garments_helper' if i in helperids else 'clothing_complete','production_png':a['png'],'production_png_SHA256':sha(p),
  'actual_selected_source_resource_id':a['source_resource_id'],'actual_source_RGBA_SHA256':a['source_rgba_sha256'],
  'production_RGB_and_complete_source_A4x_equals_frozen_prediction':exact,'max_RGBA_channel_delta':np.abs(nr-tr).max((0,1)).tolist(),
  'predicted_native_RGBA_SHA256':hashlib.sha256(native.tobytes()).hexdigest(),'frozen_prediction_RGBA_SHA256':expected[i]['prediction_RGBA_SHA256'],
  'production_scalar':a['brightness_calibration']['cells'][0]['rgb_gain'],'worker_frozen_scalar':expected[i]['gain']}
 rows.append(row)
assert len(rows)==168
out=B/f'production-projection-check-{ST}.json';assert not out.exists()
out.write_text(json.dumps({'status':'pass' if all(r['production_RGB_and_complete_source_A4x_equals_frozen_prediction']for r in rows)else'fail',
 'ready_ID_count':168,'own104_count':sum(r['worker']=='clothing_complete'for r in rows),'helper64_count':sum(r['worker']=='garments_helper'for r in rows),
 'sources_snapshot_SHA256':sha(sourcep),'frozen_numeric_QA_SHA256':sha(B/f'whole-category-numeric-QA-{ST}-final.json'),
 'shared_metadata_mutated':False,'actual_native_payload_or_runtime_claim':False,'checks':rows},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'report':str(out),'SHA256':sha(out),'production_forecast_match':sum(r['production_RGB_and_complete_source_A4x_equals_frozen_prediction']for r in rows),
 'mismatches':[r['id']for r in rows if not r['production_RGB_and_complete_source_A4x_equals_frozen_prediction']],'shared_writes':False}))

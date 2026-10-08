"""Readonly actual decoded native top-level versus frozen whole artwork transfer check."""
from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='clothing-complete-20261008'
N=ROOT/'build/hd-category-clothing-complete-20261008';proofpath=N/'numeric-proof.json'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ref(p):return {'path':p.relative_to(ROOT).as_posix(),'SHA256':sha(p)}
proof=read(proofpath);jobs=read(B/f'jobs-{ST}-final.json');frozen=read(B/f'whole-category-numeric-QA-{ST}-final.json');fx={r['id']:r for r in frozen['checks']};rows=[]
assert proof['previous_accepted_textures']==2649 and proof['previous_source_objects_PNG_and_native_payload_hashes_unchanged']
assert set(proof['new_IDs'])=={j['id']for j in jobs}==set(fx)
records={r['id']:r for r in proof['records']};helper={r['id']for r in read(B/'jobs-clothing-garments-complete-20261008-FINAL.json')}
for j in jobs:
 i=j['id'];r=records[i];f=fx[i]
 assert r['exact_historical_release_reference_RGBA'] and r['exact_native_both_aliases_and_all_mip_bytes']
 assert r['actual_source_resource_id']==j['source_resource_id'] and r['source_RGBA_SHA256']==j['source_selected_RGBA_SHA256']
 actual=N/f'{i}-native.png';im=Image.open(actual).convert('RGBA');expected=Image.open(ROOT/f['prediction']['path']).convert('RGBA')
 assert im.size==expected.size
 equal=im.tobytes()==expected.tobytes();delta=np.abs(np.asarray(im).astype(int)-np.asarray(expected).astype(int)).max((0,1)).tolist()
 assert equal,(i,'actual native RGBA differs',delta)
 rows.append({'id':i,'review_worker':'garments-helper64'if i in helper else'clothing-own104','actual_decoded_native_PNG':ref(actual),
 'native_size':list(im.size),'actual_native_RGBA_SHA256':hashlib.sha256(im.tobytes()).hexdigest(),'frozen_RGBA_SHA256':f['prediction_RGBA_SHA256'],
 'complete_RGBA_byte_exact_frozen_artist_reviewed_projection':True,'actual_full_alpha_byte_exact':True,
 'actual_native_both_aliases_all_mips_coordinator_proof':True,'actual_source_resource_id':r['actual_source_resource_id'],
 'native_MMP_SHA256':r['native_MMP_SHA256'],'new_artwork_or_generation':False})
assert len(rows)==168 and sum(r['review_worker']=='clothing-own104'for r in rows)==104
out=B/f'actual-native-transfer-review-{ST}-20261009.json';assert not out.exists()
out.write_text(json.dumps({'status':'pass','scope':'whole168 clothing artwork transfer into actual native package','actual_numeric_proof':ref(proofpath),
 'whole168_complete_actualRGBA_equals_individually_whole_artist_reviewed_forecast':True,'own104_verified':104,'helper64_transfer_verified':64,
 'previous2649_immutable':True,'native_textures':proof['native_textures'],'aliases':proof['aliases'],'archives':proof['archives'],
 'rendered_Game_models_or_GPU_frames_claim':False,'all_game_models_claim':False,'private_actual_native_representative_visual_review':'Separate visual/runtime report follows; equality transfers prior complete source/raw/native painting review without changing artwork.',
 'shared_or_frozen_metadata_modified':False,'checks':rows},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'PASS':168,'own104':104,'helper64':64,'report':str(out),'SHA256':sha(out),'mismatches':0}))

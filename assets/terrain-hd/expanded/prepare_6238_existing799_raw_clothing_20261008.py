from pathlib import Path
import json, hashlib
from datetime import datetime, timezone
from PIL import Image

B=Path(__file__).resolve().parent
ROOT=B.parents[2]
def read(p): return json.loads(p.read_text(encoding='utf-8'))
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def ref(p): return {'path':p.relative_to(ROOT).as_posix(),'SHA256':sha(p)}
def save(p,v):
    assert not p.exists(), p
    p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

tag='clothing-complete-20261008-accepted799-reuse-v1'
audit=next(r for r in read(B/'source-check-clothing-complete-20261008.json') if r['id']==6238)
alias=next(a for a in audit['aliases'] if a['resource_id']==6238)
source=ROOT/alias['fullRGBA_PNG']
orig=B/'original/6238.png'
assert Image.open(orig).convert('RGBA').tobytes()==Image.open(source).convert('RGBA').tobytes()
assert hashlib.sha256(Image.open(source).convert('RGBA').tobytes()).hexdigest()==alias['native_RGBA_SHA256']
callp=B/'generated/799-clothing-forty-sixth-call.json'
call=read(callp)
raw=ROOT/call['generated']
argp=B/'generated/799-clothing-forty-sixth-exact-call-argument.txt'
arg=argp.read_bytes().decode('utf-8')
assert arg==call['exact_prompt']
assert sha(raw)==call['raw_sha256']
assert (ROOT/call['prompt_file']).read_bytes()==(arg+'\n').encode('utf-8')
source799=next(r for r in read(B.parent/'sources.json')['textures'] if r['id']==799)
preserved=[B.parent/source799[k] for k in ['png','uncalibrated_png','reference_png','generated_png','prompt_file']]
before=[ref(p) for p in preserved]
# Reuse the exact production-prediction function; its primary6238 equals the explicitly selected fullRGBA.
script=B/'work_clothing_complete_20261008.py'
ns={'__file__':str(script)}
exec(script.read_text(encoding='utf-8').split('mode=sys.argv[1]')[0],ns)
metric=ns['norm'](6238,raw,tag)
assert metric['source_alpha_restored_exact']
assert before==[ref(p) for p in preserved]
provenance={
 'id':6238,'generation_variant':tag,'status':'candidate-pending-whole-production-review',
 'new_generation_call':False,'original_builtin_call_ID':799,'original_call':ref(callp),
 'original_exact_argument':ref(argp),'original_prompt':ref(ROOT/call['prompt_file']),'original_raw':ref(raw),
 'original_reference_files':[ref(ROOT/p) for p in call['reference_pngs']],
 'target_source_resource_id':6238,'target_source':ref(source),'target_fullRGBA_SHA256':alias['native_RGBA_SHA256'],
 'accepted799_primary_source_rgba_sha256':source799['source_rgba_sha256'],
 'accepted799_UC_resource_id':16778015,'accepted799_UC_fullRGBA_equals_selected6238':True,
 'typed_selector_reason':'6238 actual Format8888 Ordinary has bIsDXT false: use whole primary6238 RGB/A. Its RGBA is equal to799 UC16778015; accepted799 production used different primary799 RGBA. Reusing original builtin raw as a separately reviewed candidate, not byte-exact production/native reuse.',
 'reuse_source_id_used':False,'accepted799_preserved':before,
 'new_numeric_prediction':metric,
 'source_RGB_A_and_full_original_raw_privately_viewed':True,
 'review_status':'pending-pure-native-whole-view',
 'utc':datetime.now(timezone.utc).isoformat(),
 'old6238_attempts_preserved':audit['prior_history']['raws']+audit['prior_history']['calls'],
 'no_crop_local_RGB_paint_UV_registration':True
}
save(B/'provenance-reuse-accepted799-clothing-complete-20261008.json',provenance)
print(json.dumps({'id':6238,'preview':metric['production_preview'],'gain':metric['gain'],'correlation':metric['native_RGB_correlation'],'A_exact':metric['source_alpha_restored_exact']}))

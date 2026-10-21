from pathlib import Path
import json,hashlib
from PIL import Image
import numpy as np
B=Path(__file__).resolve().parent; ROOT=B.parents[2]
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ref(p):return {'path':p.relative_to(ROOT).as_posix(),'SHA256':sha(p)}
def save(p,v):
 assert not p.exists(),p
 p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
tag='clothing-complete-20261008-accepted799-reuse-v1'
provp=B/'provenance-reuse-accepted799-clothing-complete-20261008.json';p=read(provp)
callp=ROOT/p['original_call']['path'];c=read(callp)
s=Image.open(ROOT/p['target_source']['path']).convert('RGBA')
old=Image.open(B/'original/799.png').convert('RGBA')
pred=B/f'private-clothing-complete-20261008/native4x/6238-{tag}-pure-native4x.png'
out=Image.open(pred).convert('RGBA'); a=s.getchannel('A').resize(out.size,Image.Resampling.LANCZOS)
assert out.getchannel('A').tobytes()==a.tobytes()
for f in p['accepted799_preserved']: assert sha(ROOT/f['path'])==f['SHA256']
src=np.array(s).astype(float); donor=np.array(old).astype(float)
reason='Whole selected6238 source RGB/A, original799 builtin raw, freshly calibrated pure4x and inverse-native visually reviewed. Original upside-down face paint, one existing ear/top-left wash, closed pink mouth without teeth/opening, two dim eye fields, two top banded fields and connected lower dark-brown hair tips remain in the original complete UV canvas. Fine existing filtered hair paint is more defined; original broad muted skin fields and scale remain, no new anatomy/parts/letters or photo skin pores. Original799 raw is independently suitable for selected6238; this is not byte-exact accepted799 production/native reuse. Whole primary6238 A restored exactly with new global scalar1.0179443359375; accepted799 files/source metadata unchanged. Original call799 used its primary799 reference, not UC16778015; prompt and reference provenance remain honest.'
review={'id':6238,'status':'ready','generation_variant':tag,'reason':reason,
 'no_new_generation_call':True,'provenance':ref(provp),'original_call':ref(callp),
 'actual_source_audit':ref(B/'source-check-clothing-complete-20261008.json'),
 'selected6238_vs_original799_primary_RGBA_equal':s.tobytes()==old.tobytes(),
 'selected6238_vs_original799_primary_RGB_MAE':float(np.abs(src[:,:,:3]-donor[:,:,:3]).mean()),
 'exact_799_UC_donor_relationship_authority':'source-check individual6238 alias6238 exact_accepted_donors799 resource16778015 from historical-release decoded RGBA audit; no relabeling accepted799 primary source',
 'new_prediction':p['new_numeric_prediction'],'pure4x':ref(pred),
 'full_selected_A4x_byte_exact':True,'whole_selected_RGB_A_raw_pure_inverse_privately_viewed':True,
 'accepted799_preserved':p['accepted799_preserved']}
rp=B/'review-reuse-accepted799-clothing-complete-20261008.json';save(rp,review)
job={'id':6238,'generation_variant':tag,'generated':c['generated'],'prompt':c['exact_prompt'],
 'reference_pngs':c['reference_pngs'],'source_alpha_reference':p['target_source']['path'],
 'source_resource_id':6238,'source_resource_selection_reason':p['typed_selector_reason'],
 'review_concern':reason,'reused_existing_generation_call':ref(callp),
 'independent_provenance':ref(provp),'individual_visual_review':ref(rp)}
save(B/'jobs-reuse-accepted799-clothing-complete-20261008.json',[job])
print(json.dumps({'ready':6238,'newcalls':0,'newgain':p['new_numeric_prediction']['gain'],'primary_source_MAE':review['selected6238_vs_original799_primary_RGB_MAE'],'accepted799_unchanged':True}))

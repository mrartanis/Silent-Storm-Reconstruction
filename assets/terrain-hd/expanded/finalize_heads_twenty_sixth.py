from pathlib import Path
from datetime import datetime, timezone
import json, hashlib, shutil
import numpy as np
from PIL import Image
B = Path(__file__).resolve().parent
ROOT = B.parents[2]
ST = 'heads-twenty-sixth'
CALL = [3113]
READY = []
HELD = CALL

def read(p):
    return json.loads(p.read_text(encoding='utf-8'))

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def rel(p):
    return p.relative_to(ROOT).as_posix()

def ref(p):
    return {'path': rel(p), 'file_SHA256': sha(p)}

def save(n, v):
    (B / n).write_text(json.dumps(v, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
before = B / f'source-check-{ST}-before-call.json'
source = B / f'source-check-{ST}.json'
assert source.read_bytes() == before.read_bytes()
P = read(before)
selected = read(B / f'selected-{ST}.json')
scope = read(B / f'scope-{ST}.json')
assert len(P) == len(selected) == 12
actual = read(B / f'actual-records-{ST}.json')
matrices = read(B / f'native-matrices-{ST}.json')['sources']
canon = lambda r: hashlib.sha256(json.dumps(r, ensure_ascii=False, sort_keys=True, separators=(',', ':')).encode('utf-8')).hexdigest()
for table, rows in actual['tables'].items():
    for k, r in rows.items():
        assert canon(r) == actual['record_SHA256'][table][k]
for r in P:
    i = r['id']
    o = Image.open(B / f'original/{i}.png').convert('RGBA')
    assert np.array(matrices[str(i)]['native_RGBA_matrix'], dtype=np.uint8).tobytes() == o.tobytes()
    assert sha(B / f'original/{i}.png') == r['original_PNG_file_SHA256'] and r['source_RGBA_RGB_A_NN_privately_inspected']
    assert r['imagegen_call_count'] == 0
    for k in ['native_matrix_ref', 'common_actual_records']:
        assert sha(ROOT / r[k]['path']) == r[k]['file_SHA256']
    h = ROOT / r['helper_recipe']['output']
    assert Image.open(h).size == o.size and Image.open(h).convert('RGB').tobytes() == o.convert('RGB').tobytes()
notes = {3113: 'Worker HELD pending root independent fullpaint/material/weakphase review, not permanent refusal or human/dental/head/variableA name/familyban. ONE FIRST3113 actual Mat1917→Template1284→Head37 ANDMat5553→Template3036→Heads94/98, no staticModels found unknownruntime, no dynamicTHMID. Native256 Ordinary straightRGB/sourceA0..255/full historical-release-originalRGBA/POT/98actualDBrows exact. NativeopaqueRGBtoolref/NNprivateonly; fullraw1254→whole1024 RGB-FIRST LANCZOS/fullsourceAbyteexact/defaultpad0/clear0/noPremul/zero255clips. Broad source invertedhead/closedmouth/TWOexistingfaceeyes,brows,ears/ONE nose/ONEseparateeyeiris-pupil/TWOupperUVdental-lashstrips/flatred/blackUV retained, no newphysical tooth/eyelash/iris/anatomy parts counted. Hold specific sourcepaint/lightfootprint/weakphase: oldONE dimpale2pixel eyeRGBpatch gets narrower brighter verticalglint, iris sourcefilteredbluegray shades become more defined/radiating wedgepaint; olddentalgraystrip patches become roundedstronger/reshaped relative to their softsource phases, face nose/lip edges more distinct. No falselynewcatchlight count or whitening255clipping claim, only oldpigment redistribution/materialinterpretation. At96,103 original198/178/173→187/157/152 vsnatural194/173/169; adjacent97,103 equaloriginal198/178/173→231/206/200 vsnatural198/178/173. Next97,104 original151/128/126→178/154/149 vsnatural154/131/128, oldpatch verticalstrength redistributed. Pupil97,98 original10/2/0→13/5/2 vsnatural11/3/1, sameONEpupil. Sourceoldgrayedge22,48 original107/85/90→34/29/28 vsnatural98/78/81, next23,48 original90/69/57→111/93/92 vsnatural80/61/52. Literalrow44/54/59 pigment extrema complete inrepeatproof/naturalroundtrip, not physicaltoothcounts or acceptance thresholds. Native0,128 originalRGB8/0/0 A255→108/68/45 vsnatural31/17/11, isolatedsourceweakedgecontext not standalone automatic1pixelhold or holecounter; originalblack/A255paint authoritative. Flatred field original96/29/32std0 has weaknewvariation eveninterior compared naturalroundtrip, quantitative std in source-specificprobes. Sourceface nose64,199 original214/158/115→215/163/127 vsnatural216/161/117; closedmouth64,185 source173/109/74→173/111/80 vsnatural174/110/75, sourceclosedidentity retained. Farblack255,255 source0RGBA→0/1/0 storedRGB, sourceweakRGB/alpha semantics retained by default no optout. No photo pores/newhairfibers added or familyban; qualitative oldfilteredpaint/core/stripshape concerns, moderatecontrast/onepixel alone not automatic hold. Eleven other genuine fullhead sources pending0call for independent detailednativeweakphase/material audits, not technical/nameban. All12newIDs outsideHeads1–25/12payloads guarded accepted/raw/call/held/exactRGBA donors. Exactarg+ONE LF/raw/refSHA/call saved immediately/sourcebefore immutable/no sourceprompt errors found/no RGBrepair/crop/BBox/rotate/registration/localgain/paddingoptout/retry. Root sole commonmetadata/import/native/Game/Git/acceptance. Exact numerical overview: {"source_peak_xy":[96,103],"source_peak_RGB":[198,178,173],"production_at_source_peak_RGB":[187,157,152],"max_local_mean_lift":71.0,"max_local_mean_lift_xy":[0,128],"max_local_mean_loss":-63.666666666666664,"max_local_mean_loss_xy":[22,48],"pure_scalar":0.990936279296875,"flatred_source_natural_production_interior_std":[[0.0,0.0,0.0],[0.13009849070589066,0.2732914093864751,0.20428265790563208],[0.7317596224592603,0.6725490481906994,0.6029320719064005]]}'}
corrections = read(B / f'postcall-corrections-{ST}.json')
save(f'postcall-corrections-{ST}.json', corrections)
metrics = {r['id']: r for r in read(B / f'metrics-{ST}.json')}
details = {r['id']: r for r in read(B / f'detail-metrics-{ST}.json')}
phases = {r['id']: r for r in read(B / f'material-phase-{ST}.json')}
records = []
post = []
jobs = []
for i in CALL:
    cp = B / f'generated/{i}-{ST}-call.json'
    c = read(cp)
    assert sha(ROOT / c['generated']) == c['raw_sha256']
    assert (ROOT / c['prompt_file']).read_bytes() == (c['exact_prompt'] + '\n').encode('utf-8') and sha(ROOT / c['prompt_file']) == c['prompt_sha256']
    assert all((sha(ROOT / p) == h for p, h in c['reference_sha256'].items()))
    for k in ['pattern_constraints_ref', 'native_matrix_ref', 'common_actual_records', 'source_before_call_ref']:
        assert sha(ROOT / c[k]['path']) == c[k]['file_SHA256']
    assert c['literal_actual_tool_arguments'] == {'prompt': c['exact_prompt'], 'referenced_image_paths': [(ROOT / p).as_posix() for p in c['reference_pngs']], 'transparent_background': False}
    o = Image.open(B / f'original/{i}.png').convert('RGBA')
    n = Image.open(B / f'private-{ST}/native4x/{i}-calibrated-native-alpha-private.png').convert('RGBA')
    assert n.size == (o.width * 4, o.height * 4) and n.getchannel('A').tobytes() == o.getchannel('A').resize(n.size, Image.Resampling.LANCZOS).tobytes()
    assert Image.open(ROOT / c['generated']).convert('RGBA').getchannel('A').getextrema() == (255, 255)
    status = 'worker-ready-candidate-pending-root-art-native-acceptance' if i in READY else 'held-original-matte-paint-material-localstrength-review'
    r = {'id': i, 'status': status, 'call_count': 1, 'reason': notes[i], 'generated': c['generated'], 'raw_sha256': c['raw_sha256'], 'prompt_file': c['prompt_file'], 'prompt_argument_sha256': c['prompt_argument_sha256'], 'prompt_canonical_sha256': c['prompt_sha256'], 'call_metadata': ref(cp), 'reference_sha256': c['reference_sha256'], 'source_transform_recipe': c['source_transform_recipe'], 'metrics': metrics[i], 'detail_metrics': details[i], 'material_phase': phases[i], 'repeat_phase_ref': {**ref(B / f'repeat-phase-{ST}.json'), 'id': i}, 'source_specific_probes_ref': ref(B / f'postcall-source-specific-probes-{ST}.json'), 'postcall_material_details_ref': {**ref(B / f'postcall-material-details-{ST}.json'), 'id': i}, 'production_matrix_ref': {**ref(B / f'production-matrices-{ST}.json'), 'source_key': str(i)}, 'fullsource_fullraw_defaultnative4x_RGB_A_privately_inspected': True}
    if i in READY:
        jobs.append({'id': i, 'generated': c['generated'], 'prompt': c['exact_prompt'], 'reference_png': c['reference_pngs'][0], 'reference_recipe': c['source_transform_recipe'], 'worker_status': status, 'worker_review_note': notes[i]})
    else:
        rd = B / f'rejected/{ST}'
        rd.mkdir(parents=True, exist_ok=True)
        for suffix, p in [('raw.png', ROOT / c['generated']), ('prompt.txt', ROOT / c['prompt_file']), ('call.json', cp)]:
            target = rd / f'{i}-attempt1-{suffix}'
            assert not target.exists()
            shutil.copyfile(p, target)
        save(f'rejected/{ST}/{i}-attempt1-reason.json', {'id': i, 'status': status, 'reason': notes[i], 'raw_sha256': c['raw_sha256'], 'no_repeat': True, 'not_permanent_refusal': True})
        r['rejected_raw'] = rel(rd / f'{i}-attempt1-raw.png')
    records.append(r)
    post.append({'id': i, 'imagegen_call_count': 1, 'postcall_status': status, 'sourcebefore_zero_call_snapshot_immutable': True, 'fullsource_fullraw_native4x_storedRGB_fullA_privately_reviewed': True})
sourceonly = []
for r in P:
    if r['id'] in CALL:
        continue
    d = {'id': r['id'], 'status': 'genuine-head-face-color-pending-nativephase-material-review', 'call_count': 0, 'reason': r['source_only_reason'], 'native_matrix_ref': r['native_matrix_ref'], 'common_actual_records': r['common_actual_records'], 'actual_static_consumers': r['fresh_actual_static_consumers'], 'native_RGB_distinct_count': r['native_RGB_distinct_count'], 'dominant_RGB_count_values': r['dominant_RGB_count_values'], 'recommended_original_only': False}
    sourceonly.append(d)
    records.append(d)
orig = read(B / f'original-only-proof-{ST}.json')
origids = {r['id'] for r in orig}
for d in sourceonly:
    if d['id'] in origids:
        d.update(status='individual-original-only-entireconstantRGBA-recommendation-rootreview', recommended_original_only=True, original_only_proof_ref={**ref(B / f'original-only-proof-{ST}.json'), 'id': d['id']}, not_runtimeaccepted=True)
save(f'source-only-detail-{ST}.json', sourceonly)
save(f'jobs-{ST}.json', jobs)
save(f'postcall-source-status-{ST}.json', post)
S = read(B.parent / 'sources.json')['textures']
Q = read(B / 'queue.json')
ex = read(B / f'exclusions-{ST}.json')
assert not set(scope['selected_ids']).intersection(ex['ids']) and all((r['source_rgba_sha256'] not in ex['blocked_hashes'] for r in P))
guards = {'utc': datetime.now(timezone.utc).isoformat(), 'before_source_entry_count': ex['source_entry_count'], 'before_queue_count': ex['queue_count'], 'postcall_source_entry_count': len(S), 'postcall_queue_count': len(Q), 'source_entry_count_not_finalaccepted_claim': True, 'source_current_status_counts': {st: sum((r.get('status') == st for r in S)) for st in sorted({str(r.get('status')) for r in S})}, 'own_postcall_overlap': [{'id': r['id'], 'status': r.get('status')} for r in S + Q if r['id'] in scope['selected_ids']], 'sources_SHA256': sha(B.parent / 'sources.json'), 'queue_SHA256': sha(B / 'queue.json'), 'calls': 1, 'retries': 0, 'shared_writes': False, 'policy': 'FreshS/Q/globalraw/jobs/calls/prompts/held/refusals/exactRGBA before each solefirstcall. Previous source-only0call views honestly reaudited, no fake never-viewed claim. Concurrentrootwrites not worker.'}
save(f'final-guards-{ST}.json', guards)
recordcount = sum((len(v) for v in actual['tables'].values()))
save(f'review-{ST}.json', {'batch': ST, 'status': 'stable-complete-held-bounded-review', 'selected_ids': scope['selected_ids'], 'unique_native_RGBA_count': scope['unique_native_RGBA_count'], 'generated_ids': CALL, 'ready_ids': READY, 'held_ids': HELD, 'genuine_pending_uncalled_ids': scope['genuine_pending_uncalled_ids'], 'recommended_original_only_ids': [], 'call_count': 1, 'max_authorized_calls': 3, 'no_retries': True, 'records': records, 'source_before_call': ref(before), 'source_check': ref(source), 'pattern_before_call': ref(B / f'pattern-constraints-{ST}.json'), 'native_matrix': ref(B / f'native-matrices-{ST}.json'), 'actual_records': ref(B / f'actual-records-{ST}.json'), 'production_matrix': ref(B / f'production-matrices-{ST}.json'), 'natural_RGB_roundtrip': ref(B / f'natural-RGB-roundtrip-{ST}.json'), 'repeat_phase': ref(B / f'repeat-phase-{ST}.json'), 'postcall_corrections_ref': ref(B / f'postcall-corrections-{ST}.json'), 'source_parity': 'Fresh12 genuinepending fullsource256POT/12uniqueRGBA/ALLoutsideHeads1–25 priorselected/historyrelease originalfullRGBA/nativeType/A parity/98actualuniqueDBrows. Actual Materials→TemplateID→Heads, unknownModels/runtime notunused; no dynamicTHMID falselyattached. Exact3113source invertedclosedmouth/literalparts/softpaint/eye2pixelpatch/weakshadephase beforeproof. No original-only recommendation. AlloriginalnonconstantCOLOR, genuinepending orheld.', 'QA': 'Worker HELD pending root independent fullpaint/material/weakphase review, not permanent refusal or human/dental/head/variableA name/familyban. ONE FIRST3113 actual Mat1917→Template1284→Head37 ANDMat5553→Template3036→Heads94/98, no staticModels found unknownruntime, no dynamicTHMID. Native256 Ordinary straightRGB/sourceA0..255/full historical-release-originalRGBA/POT/98actualDBrows exact. NativeopaqueRGBtoolref/NNprivateonly; fullraw1254→whole1024 RGB-FIRST LANCZOS/fullsourceAbyteexact/defaultpad0/clear0/noPremul/zero255clips. Broad source invertedhead/closedmouth/TWOexistingfaceeyes,brows,ears/ONE nose/ONEseparateeyeiris-pupil/TWOupperUVdental-lashstrips/flatred/blackUV retained, no newphysical tooth/eyelash/iris/anatomy parts counted. Hold specific sourcepaint/lightfootprint/weakphase: oldONE dimpale2pixel eyeRGBpatch gets narrower brighter verticalglint, iris sourcefilteredbluegray shades become more defined/radiating wedgepaint; olddentalgraystrip patches become roundedstronger/reshaped relative to their softsource phases, face nose/lip edges more distinct. No falselynewcatchlight count or whitening255clipping claim, only oldpigment redistribution/materialinterpretation. At96,103 original198/178/173→187/157/152 vsnatural194/173/169; adjacent97,103 equaloriginal198/178/173→231/206/200 vsnatural198/178/173. Next97,104 original151/128/126→178/154/149 vsnatural154/131/128, oldpatch verticalstrength redistributed. Pupil97,98 original10/2/0→13/5/2 vsnatural11/3/1, sameONEpupil. Sourceoldgrayedge22,48 original107/85/90→34/29/28 vsnatural98/78/81, next23,48 original90/69/57→111/93/92 vsnatural80/61/52. Literalrow44/54/59 pigment extrema complete inrepeatproof/naturalroundtrip, not physicaltoothcounts or acceptance thresholds. Native0,128 originalRGB8/0/0 A255→108/68/45 vsnatural31/17/11, isolatedsourceweakedgecontext not standalone automatic1pixelhold or holecounter; originalblack/A255paint authoritative. Flatred field original96/29/32std0 has weaknewvariation eveninterior compared naturalroundtrip, quantitative std in source-specificprobes. Sourceface nose64,199 original214/158/115→215/163/127 vsnatural216/161/117; closedmouth64,185 source173/109/74→173/111/80 vsnatural174/110/75, sourceclosedidentity retained. Farblack255,255 source0RGBA→0/1/0 storedRGB, sourceweakRGB/alpha semantics retained by default no optout. No photo pores/newhairfibers added or familyban; qualitative oldfilteredpaint/core/stripshape concerns, moderatecontrast/onepixel alone not automatic hold. Eleven other genuine fullhead sources pending0call for independent detailednativeweakphase/material audits, not technical/nameban. All12newIDs outsideHeads1–25/12payloads guarded accepted/raw/call/held/exactRGBA donors. Exactarg+ONE LF/raw/refSHA/call saved immediately/sourcebefore immutable/no sourceprompt errors found/no RGBrepair/crop/BBox/rotate/registration/localgain/paddingoptout/retry. Root sole commonmetadata/import/native/Game/Git/acceptance. Exact numerical overview: {"source_peak_xy":[96,103],"source_peak_RGB":[198,178,173],"production_at_source_peak_RGB":[187,157,152],"max_local_mean_lift":71.0,"max_local_mean_lift_xy":[0,128],"max_local_mean_loss":-63.666666666666664,"max_local_mean_loss_xy":[22,48],"pure_scalar":0.990936279296875,"flatred_source_natural_production_interior_std":[[0.0,0.0,0.0],[0.13009849070589066,0.2732914093864751,0.20428265790563208],[0.7317596224592603,0.6725490481906994,0.6029320719064005]]}', 'ownership': 'Own Heads26 only. FrozenHeads25/24/23/Clothing51 and all oldscopes exact unchanged. Root sole shared/import/native/build/Game/C++/Git/finalacceptance. Currentroot49 publication2480/3ce4b51f separate from honest originalBEFORE source/local2480publicationprevious2474; no retroedit.'})
assert before.read_bytes() == source.read_bytes()
mp = B / f'sha256-{ST}.json'
files = list(B.glob(f'*{ST}*.json')) + list(B.glob('*heads_twenty_sixth*.py')) + list((B / 'generated').glob(f'*{ST}*.png')) + list((B / 'generated').glob(f'*{ST}*-call.json')) + list((B / 'generated').glob(f'*{ST}*-exact-call-argument.txt')) + list((B / f'private-{ST}').rglob('*')) + list((B / f'rejected/{ST}').rglob('*'))
for i in CALL:
    files.extend([B / f'generated/{i}-raw.png', B / f'generated/{i}-prompt.txt'])
save(mp.name, {rel(p): sha(p) for p in sorted(set(files)) if p.is_file() and p != mp})
assert all((sha(ROOT / p) == h for p, h in read(mp).items()))
print(json.dumps({'ready': READY, 'held': HELD, 'FIRST_calls': 1, 'genuine_pending0call': len(scope['genuine_pending_uncalled_ids']), 'originalrecommend0call': [], 'actualDB_unique_rows': recordcount, 'manifest_entries': len(read(mp)), 'manifest_differences': 0, 'source_before_byteunchanged': True, 'sourceentries_queue_current': [len(S), len(Q)], 'SHA256': {n: sha(B / f'{n}-{ST}.json') for n in ['jobs', 'review', 'source-check', 'selected', 'sha256']}}, indent=2))

from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path):
    return json.loads(path.read_text(encoding='utf-8'))
def portable(path):
    return path.relative_to(ROOT).as_posix()
def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

review_path = BASE / 'review-equipment-twenty-fourth.json'
snapshot = BASE / 'worker-review-equipment-twenty-fourth-first-ready-snapshot.json'
assert snapshot.exists() and snapshot.read_bytes() == review_path.read_bytes()
old_sha = sha(snapshot)
coordinator = read(BASE / 'coordinator-art-review-twenty-third.json')
assert coordinator['worker_reviews_SHA256'][portable(review_path)] == old_sha
manifest_path = BASE / 'sha256-equipment-twenty-fourth.json'
old_manifest = read(manifest_path)
for relative, digest in old_manifest.items():
    assert sha(ROOT / relative) == digest, relative
immutable = {relative: digest for relative, digest in old_manifest.items() if relative != portable(review_path)}

ids = [5432, 1822]
review = read(review_path)
assert review['ready_ids'] == [5432] and review['pending_ids'] == [1822]
assert review['call_count'] == 2
prompt_provenance = []
for texture_id in ids:
    call = read(BASE / 'generated' / f'{texture_id}-equipment-twenty-fourth-call.json')
    assert sha(ROOT / call['generated']) == call['raw_sha256']
    prompt = ROOT / call['prompt_file']
    actual_argument = call['exact_prompt'].encode('utf-8')
    assert hashlib.sha256(actual_argument).hexdigest() == call['prompt_sha256']
    if texture_id == 5432:
        assert prompt.read_bytes() == actual_argument + b'\n'
        exact_path = BASE / 'generated' / '5432-equipment-twenty-fourth-exact-call-prompt.txt'
        if exact_path.exists():
            assert exact_path.read_bytes() == actual_argument
        else:
            exact_path.write_bytes(actual_argument)
        prompt_provenance.append({'id': texture_id, 'existing_prompt_file': portable(prompt), 'existing_prompt_file_SHA256': sha(prompt), 'difference': 'Existing prompt file now has one terminal LF beyond exact actual call argument. Actual call JSON/job prompt retained exact CRLF argument and original SHA; no existing file/call/job bytes edited.', 'exact_call_argument_file': portable(exact_path), 'exact_call_argument_SHA256': sha(exact_path)})
    else:
        assert sha(prompt) == call['prompt_sha256']
        assert prompt.read_bytes() == actual_argument
        prompt_provenance.append({'id': texture_id, 'existing_prompt_file': portable(prompt), 'existing_prompt_file_SHA256': sha(prompt), 'exact_call_argument_matches_file': True})
    for relative, digest in call['reference_sha256'].items():
        assert sha(ROOT / relative) == digest
    assert sha(BASE / 'pattern-constraints-equipment-twenty-fourth.json') == call['pattern_constraints_file_sha256']

prompt_proof = BASE / 'prompt-provenance-equipment-twenty-fourth.json'
write(prompt_proof, {'scope': 'Post-call exact argument verification; prior call metadata and existing prompt files immutable. Exact actual prompt argument always recoverable from original call JSON/job.', 'records': prompt_provenance})

sources_path = ROOT / 'assets/terrain-hd/sources.json'
queue_path = BASE / 'queue.json'
source_bytes = sources_path.read_bytes()
queue_bytes = queue_path.read_bytes()
sources = json.loads(source_bytes.decode('utf-8'))['textures']
queue = json.loads(queue_bytes.decode('utf-8'))
own_sources = [r for r in sources if r['id'] in ids]
own_queue = [r for r in queue if r['id'] in ids]
assert [r['id'] for r in own_sources] == [5432]
assert [r['id'] for r in own_queue] == [5432]
precall = read(BASE / 'exclusions-equipment-twenty-fourth.json')
assert precall['accepted_count'] == 2332 and precall['queue_count'] == 2792
assert not set(ids).intersection(precall['ids'])
source_proof = read(BASE / 'source-check-equipment-twenty-fourth.json')
for record in source_proof:
    assert record['source_rgba_sha256'] not in precall['blocked_hashes']

guard_path = BASE / 'final-guards-equipment-twenty-fourth.json'
write(guard_path, {
    'audit_utc': datetime.now(timezone.utc).isoformat(),
    'precall_snapshot': portable(BASE / 'exclusions-equipment-twenty-fourth.json'),
    'precall_counts': {'accepted': 2332, 'queue': 2792},
    'precall_selected_absent_from_all_ids_and_blocked_native_RGBA': True,
    'postcall_snapshot_counts': {'accepted': len(sources), 'queue': len(queue)},
    'postcall_snapshot_SHA256': {'sources': hashlib.sha256(source_bytes).hexdigest(), 'queue': hashlib.sha256(queue_bytes).hexdigest()},
    'own_postcall_source_ids': [r['id'] for r in own_sources],
    'own_postcall_queue': [{'id': r['id'], 'status': r.get('status')} for r in own_queue],
    'postcall_own_overlap_reason': 'Coordinator explicitly reviewed and imported5432 AFTER this worker completed its single built-in call and ready QA. This is not a pre-call overlap. Worker never regenerated/imported/accepted it. Current own5432 overlap is recorded, not falsely asserted zero.',
    '1822_still_absent_from_sources_and_main_queue': True,
    'shared_writes': False,
    'scope': 'Fresh read-only current sources/queue snapshot. Original pre-call exclusions/parity/typed/source proof retained unchanged. No new calls or newly selected candidates.'
})
survey_path = BASE / 'survey-equipment-twenty-fourth.json'
survey = {
    5431: 'Dense two-dimensional checker; exact native count/phase not established by this visual-only survey. Repeating patterns are allowed when proved; no blanket prohibition.',
    5435: 'Visible labels/glyph-like marks; not selected for an artistic edit.',
    5436: 'Grain/material fragments and full source detail count/pattern scale not fully demonstrated in this visual-only survey.',
    5437: 'Mostly flat brown polygons/circles without clearly resolved original painted grain in this survey.',
    5439: 'Visible labels/glyph-like marks; not selected for an artistic edit.',
    1821: 'Fine grid/mark phase not fully established in this visual-only survey.',
    1827: 'Tiny marks/slots/lines not fully established in this visual-only survey.'
}
write(survey_path, {
    'scope': 'Seven unselected full original native PNGs privately viewed. This survey makes no fresh DB/historical/release parity claim for these IDs and is not a claim that the remaining group is unsuitable or exhausted.',
    'records': [{'id': texture_id, 'source_original': f'assets/terrain-hd/expanded/original/{texture_id}.png', 'original_png_sha256': sha(BASE / 'original' / f'{texture_id}.png'), 'call_count': 0, 'status': 'unselected-source-only-survey', 'reason': reason} for texture_id, reason in survey.items()],
    'call_count': 0
})
added = {
    'previous_worker_review_snapshot': {'path': portable(snapshot), 'sha256': old_sha, 'matches_coordinator_saved_worker_review_SHA256': True},
    'final_guards': portable(guard_path),
    'unselected_original_only_survey': portable(survey_path),
    'whole_native_material_metrics': portable(BASE / 'material-metrics-equipment-twenty-fourth.json'),
    'postcall_prompt_artifact_verification': portable(prompt_proof),
    'coordinator_postcall_import': {'id': 5432, 'art_review': 'Passed coordinator full source/raw/pure native512x128 QA per parent message; imported only scalar/full original sourceA. Coordinator decision remains separate.', 'worker_repeat_call': False},
    'changes_since_coordinator_saved_worker_review': 'Only seven ancillary metadata fields added: exact old worker-review snapshot, final guard reference, unselected original survey reference, native material metrics reference, post-call prompt artifact verification, coordinator post-call import note, and this change note. Existing ready/pending IDs, artistic notes, metrics, jobs, source-before-call proof, original/raw/prompt/reference/call bytes remain unchanged. Current5432 prompt file has one terminal LF beyond exact actual argument; separate exact-argument artifact records the original argument without editing the existing file.'
}
assert not set(added).intersection(review)
review.update(added)
write(review_path, review)
for relative, digest in immutable.items():
    assert sha(ROOT / relative) == digest, relative
assert sha(snapshot) == old_sha
assert all(read(review_path)[key] == read(snapshot)[key] for key in read(snapshot))
manifest_files = sorted(BASE.glob('*equipment-twenty-fourth*.json'))
manifest_files += sorted((BASE / 'generated').glob('*equipment-twenty-fourth-call.json'))
manifest_files += sorted((BASE / 'generated').glob('*equipment-twenty-fourth-exact-call-prompt.txt'))
manifest = {portable(path): sha(path) for path in manifest_files if path != manifest_path}
write(manifest_path, manifest)
assert all(sha(ROOT / relative) == digest for relative, digest in read(manifest_path).items())
print(json.dumps({'ready': [5432], 'pending': [1822], 'calls': 2, 'precall_counts': [2332, 2792], 'postcall_counts': [len(sources), len(queue)], 'old_review_SHA256': old_sha, 'new_review_SHA256': sha(review_path), 'verified_manifest_files': len(manifest), 'stable': {name: sha(BASE / f'{name}-equipment-twenty-fourth.json') for name in ['jobs', 'review', 'source-check', 'selected', 'sha256']}}, ensure_ascii=False))

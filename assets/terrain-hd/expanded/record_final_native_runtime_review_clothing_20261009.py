"""Record completed worker visual review; verify evidence without editing assets."""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[3]
EXP = ROOT / 'assets/terrain-hd/expanded'

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def reference(name):
    p = EXP / name
    return {'path': p.relative_to(ROOT).as_posix(), 'SHA256': sha(p)}

def immutable_write(p, obj):
    data = (json.dumps(obj, indent=2, ensure_ascii=False) + '\n').encode('utf-8')
    if p.exists():
        raise FileExistsError(f'Immutable report already exists: {p}')
    p.write_bytes(data)

runtime = json.loads((EXP / 'clothing-complete-20261008-runtime-check.json').read_text(encoding='utf-8'))
transfer = json.loads((EXP / 'actual-native-transfer-review-clothing-complete-20261008-20261009.json').read_text(encoding='utf-8'))
category = json.loads((EXP / 'category-final-clothing-complete-20261008.json').read_text(encoding='utf-8'))
conversion = json.loads((EXP / 'private-clothing-complete-20261008/runtime-final-20261009/actual22-frame-format-only.json').read_text(encoding='utf-8'))
representatives = [2011, 3313, 4299, 5481, 6238, 2016]
names = [f'hd2817-clothing-{i}-{s}' for i in representatives for s in ['on', 'off', 'restored']]
names += ['hd2817-on', 'hd2817-options', 'hd2817-off', 'hd2817-restored']
assert runtime['status'] == 'complete'
assert runtime['all_new_native_checked'] == 168 and len(runtime['GPU_checks']) == 18
assert runtime['normal_quit_exit_code'] == 0 and runtime['own_process_absent_after_quit']
assert [s['terrain_HD'] for s in runtime['scene_states']] == [11, 0, 11]
assert all(c['pending'] == 0 and c['placeholder'] == 0 for c in runtime['GPU_checks'])
assert transfer['status'] == 'pass' and len(transfer['checks']) == 168
assert transfer['whole168_complete_actualRGBA_equals_individually_whole_artist_reviewed_forecast']
assert transfer['previous2649_immutable']
assert len(category['all_ID_dispositions']) == 300 and category['eligible_art_unprocessed'] == 0
assert category['accepted_preserved'] == 95 and category['ready_ID_count'] == 168 and category['retained_original_count'] == 37
screenshots = {s['name']: s for s in runtime['screenshots']}
conversions = {Path(c['original_actual_BMP']).stem: c for c in conversion}
assert set(names) == set(screenshots) == set(conversions) and len(names) == 22
frames = []
for name in names:
    s, c = screenshots[name], conversions[name]
    actual, private = Path(c['original_actual_BMP']), Path(c['private_view_PNG'])
    assert sha(actual) == c['original_BMP_SHA256'] == s['SHA256']
    assert sha(private) == c['private_view_PNG_SHA256']
    assert c['stored_RGB_byte_exact_format_only_conversion'] and c['size'] == [2560, 1440]
    frames.append({'name': name, 'actual_local_only_BMP': str(actual), 'actual_BMP_SHA256': sha(actual),
                   'dimensions': [2560, 1440], 'whole_frame_privately_viewed': True,
                   'view_method': 'Byte-identical stored RGB format-only PNG; tool display resized to 2048x1152, no crop or image editing.',
                   'visual_status': 'pass'})

report = {
    'utc': datetime.now(timezone.utc).isoformat(),
    'status': 'pass', 'worker_stage': 'complete-native-and-runtime-artwork-review',
    'reviewer': 'clothing_complete',
    'category_scope': {'original_IDs': 300, 'accepted_preserved': 95, 'new_HD_ready': 168,
                       'individual_retained_originals': 37, 'eligible_art_unprocessed': 0},
    'category_frozen_artwork': reference('category-final-clothing-complete-20261008.json'),
    'all168_native_transfer': reference('actual-native-transfer-review-clothing-complete-20261008-20261009.json'),
    'representative_actual_native_visual_review': reference('actual-native-visual-review-clothing-complete-20261008-20261009.json'),
    'helper64_actual_native_numeric_review': reference('actual-native-numeric-clothing-garments-complete-20261009-v1.json'),
    'helper64_actual_native_whole_visual_review': reference('actual-native-visual-clothing-garments-complete-20261009-v1.json'),
    'coordinator_actual_runtime_numeric_proof': reference('clothing-complete-20261008-runtime-check.json'),
    'actual_game_SHA256': runtime['game_SHA256'],
    'single_test_Game': {'process_id': runtime['own_process_id'], 'normal_quit_exit_code': 0,
                         'absent_after_quit': True, 'fps': runtime['fps'], 'frame_ms': runtime['frame_ms']},
    'visual_frame_count': 22, 'whole_actual_GPU_frames': frames,
    'per_representative_findings': [
        {'id': 2011, 'status': 'pass', 'finding': 'Camo and vertical binding layout, plain outline and alpha support stable on/off/restored. Existing weak y64/y74 fragments receive local contrast gain as independently qualified at full-map artwork review; small GPU panels do not independently resolve those fragments.',
         'qualification': reference('independent-art-review-2011-heads-worker-final.json')},
        {'id': 3313, 'status': 'pass', 'finding': 'Smooth slate/brown mechanical paint, openings and mark/stencil fields retain positions. No added panel displacement or coarse grain visible; source softens with HD off and reviewed detail returns.'},
        {'id': 4299, 'status': 'pass', 'finding': 'Skin/plaid/red-brown paint fields preserve layout; existing red speckle footprints and empty support fields remain consistent. Previously qualified microcontrast is retained; no additional large stain or newly opened alpha region visible.',
         'qualification': reference('independent-art-review-clothing-complete-20261008-4299-v2.json')},
        {'id': 5481, 'status': 'pass', 'finding': 'Slender ochre garment/collar canvas, white collar, old buckle and small buttons keep placement and silhouette through all three states. This is the independently reviewed exact whole-source sibling of 3894.'},
        {'id': 6238, 'status': 'pass', 'finding': 'Whole face, hair and auxiliary UV pieces preserve silhouette and landmarks; original lower-detail view and restored candidate remain coherent. Separate selected6238 alpha and calibration verified by complete native transfer. No new call or byte-exact accepted799 production reuse is implied.',
         'qualification': reference('provenance-reuse-accepted799-clothing-complete-20261008.json')},
        {'id': 2016, 'status': 'pass', 'finding': 'Deliberately plain brown field with existing small corner feature and cream alpha diagnostic support remains plain in all states; no invented texture grain or shifted feature visible.'}
    ],
    'scene_and_options_findings': 'Scene layout, selected unit, HUD, portrait and fog boundary remain coherent. Grass detail visibly softens with HD off and returns after restoration. Options show 2560x1440, HD textures on, maximum anisotropy, V-sync and FXAA. Expected unit/portrait animation differs between captures; frames are not treated as pixel-identical scenes.',
    'diagnostic_background_qualification': 'Black/white portions are diagnostic canvas or unchanged source alpha support, present across states; they are not claimed to be newly generated texture padding or new transparency holes.',
    'scope_limit': 'Complete actual native RGBA of all168 new maps equals the full individually artist-reviewed forecasts; both aliases and every mip are validated by coordinator proof. Actual live GPU visual review covers six representative texture diagnostic views, one scene and settings. It does not certify close-up rendering of every clothing mesh or every retained functional source control.',
    'previous2649_unchanged': True,
    'shared_or_frozen_metadata_modified': False,
    'new_imagegen_calls': 0,
    'public_comparisons_HTML_or_images': False,
    'publication_owner': 'heads_complete',
    'publication_claim': 'Worker checks complete; commit/push completion is owned and reported by coordinator.'
}
output = EXP / 'native-runtime-visual-review-clothing-complete-20261008-final.json'
immutable_write(output, report)

additional = [
    'check_production_projection_clothing_20261008.py',
    'production-projection-check-clothing-complete-20261008.json',
    'restore_publication_proofs_clothing_20261008.py',
    'check_native_transfer_clothing_20261009.py',
    'actual-native-transfer-review-clothing-complete-20261008-20261009.json',
    'record_actual_native_visual_clothing_20261009.py',
    'actual-native-visual-review-clothing-complete-20261008-20261009.json',
    Path(__file__).name,
    output.name,
]
manifest = {'status': 'complete', 'scope': 'Additional own-worker stable checks after frozen strict publication manifest; add this manifest itself as well.',
            'original_strict_manifest': reference('publication-SHA-manifest-clothing-complete-20261008-final.json'),
            'files': [{**reference(name), 'bytes': (EXP/name).stat().st_size} for name in additional],
            'private_previews_BMP_HTML_locks_included': False, 'shared_or_frozen_metadata_modified': False}
manifest_path = EXP / 'publication-additional-checks-clothing-complete-20261009.json'
immutable_write(manifest_path, manifest)
print(json.dumps({'report': reference(output.name), 'additional_manifest': reference(manifest_path.name), 'additional_files': len(additional)}, indent=2))

import hashlib,json
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
READY=list(range(2382,2390))
SOURCE_ONLY=[1067,5391,5392,5935]
metrics={x['id']:x for x in json.loads((BASE/'metrics-effects-seventh.json').read_text(encoding='utf-8'))}
proof={x['id']:x for x in json.loads((BASE/'source-check-effects-seventh.json').read_text(encoding='utf-8'))}
notes={
2382:'Upper branching cap, central flame mass, large lower-central/left black channel and lower-right curled tail retain original UV positions and scale.',
2383:'Upper thin lattice, left middle projection, central/right bright mass, lower-left source detached patches and bottom tail retain original frame pose.',
2384:'Broad original central mass, upper branches, lower holes and horizontal separated source bottom patches retain their source-specific positions.',
2385:'Original diagonal rightward tongue, central orange band, irregular lower paint and dark low tail retain original UV footprint.',
2386:'Original upper-left hooked source branch, rightward central projection, lower mass and disconnected left source flecks retained.',
2387:'Original upper-left loop, narrow central column, wide low left mass and wispy bottom-right taper retain exact source pose.',
2388:'Original upper cap, left/right tongues, lower-left ringlike source patch and thin lower-right tail retained.',
2389:'Original bright lower-middle horizontal band, central black holes and darker detached lower-left/right source patches retained.'
}
jobs=[];records=[]
for id_ in READY:
    call_path=BASE/f'generated/{id_}-effects-seventh-call.json'
    call=json.loads(call_path.read_text(encoding='utf-8'))
    raw=BASE/f'generated/{id_}-raw.png';prompt=BASE/f'generated/{id_}-prompt.txt'
    assert hashlib.sha256(raw.read_bytes()).hexdigest()==call['raw_sha256']
    assert hashlib.sha256(prompt.read_bytes()).hexdigest()==call['prompt_sha256']
    exact=prompt.read_bytes().decode('utf-8');assert exact==call['exact_prompt']
    assert Image.open(raw).convert('RGBA').getchannel('A').getextrema()==(255,255)
    src=Image.open(BASE/f'original/{id_}.png').convert('RGBA')
    native=Image.open(BASE/f'private-effects-seventh/native4x/{id_}-calibrated-native-alpha-private.png').convert('RGBA')
    alpha=src.getchannel('A').resize(native.size,Image.Resampling.LANCZOS)
    assert native.size==(src.width*4,src.height*4)==(512,512)
    assert alpha.tobytes()==native.getchannel('A').tobytes()
    assert alpha.getextrema()==(0,0)
    call['reference_sha256']={r:hashlib.sha256((ROOT/r).read_bytes()).hexdigest() for r in call['reference_pngs']}
    call['source_transform_recipe']=proof[id_]['helper_recipe']
    call_path.write_text(json.dumps(call,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    jobs.append({'id':id_,'generated':call['generated'],'prompt':exact})
    records.append({'id':id_,'status':'ready-for-coordinator-art-review','call_count':1,
      'review_note':notes[id_]+' Full raw material and source/native512same-scale privately viewed. Orange/brown painted source material and original large/fine holes follow source topology; no new independent puffs/rays/glints/objects identified. Original top-edge occupancy remains, full black canvas and frame-specific component placement retained.',
      'review_concern':'Candidate edges and original thin flame branches are more defined at HD size than source interpolation. Coordinator final material review should confirm this restraint; high spatial correlation alone is not acceptance. These are existing source flame curls, not a snow/dust field with invented curling material. No regeneration for brightness.',
      'source_check':'assets/terrain-hd/expanded/source-check-effects-seventh.json',
      'call_metadata':f'assets/terrain-hd/expanded/generated/{id_}-effects-seventh-call.json',
      'reference_pngs':call['reference_pngs'],'generated':call['generated'],'prompt_file':call['prompt_file'],
      'tool_output':call['tool_output'],'tool_output_session':call['tool_output_session'],
      'raw_sha256':call['raw_sha256'],'prompt_sha256':call['prompt_sha256'],'reference_sha256':call['reference_sha256'],
      'source_transform_recipe':proof[id_]['helper_recipe'],
      'metrics':metrics[id_],
      'source_512_preview':f'assets/terrain-hd/expanded/private-effects-seventh/native4x/{id_}-source-rgb512-private.png',
      'alpha_pipeline':'Actual texture TransparentAdd, actual ParticleInstances997 Additive, native alpha uniformly0 and useful stored RGB. Private pack-equivalent alpha exactly equals native alpha4x; opaque helper/raw alpha never replaces source. Root final pack retains source alpha0 and native RGB.',
      'uv_registration':'None. Full native128x128 canvas normalized4x512x512 without crop/reposition/rotation/repacking/source artistic RGB insertion.',
      'fresh_typed_binding':'ParticleInstances997 Fire03 Texture0..7; all eight actual TextureN bindings and texture records saved in source proof. Actual Particles Wrap0/0 and Pivot0/0. Native implementation selects one entire texture per frame, no atlas grid.',
      'missing_sibling_frames':[x['missing_sibling_texture_slots'] for x in proof[id_]['fresh_typed_usage'] if x['missing_sibling_texture_slots']]})
for x in json.loads((BASE/'source-only-effects-seventh.json').read_text(encoding='utf-8')):records.append(x)
(BASE/'jobs-effects-seventh.json').write_text(json.dumps(jobs,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'review-effects-seventh.json').write_text(json.dumps({'status':'stable-ready-subset-awaiting-coordinator-review','selected_ids':READY+SOURCE_ONLY,'ready_ids':READY,'pending_ids':[],'source_only_ids':SOURCE_ONLY,
 'tool':'Built-in image_gen8 independent full native-RGB source edits, one call per ID; structural source-only4 received zero calls.',
 'ownership':'Only independent expanded package artifacts/private PNGs changed. Shared metadata/import/calibration/native build/Git untouched.',
 'source_parity':'Fresh read-only actual historical MMP == actual release resource == original PNG RGBA bytes/dimensions/hash. Actual texture/ParticleInstances/Particles rows, blend/Wrap/pivot/all frame bindings freshly saved; full native UV implementation proof.',
 'records':records},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Stable ready:',READY,'source-only without calls:',SOURCE_ONLY)

import json,hashlib
from pathlib import Path
from PIL import Image,ImageChops
BASE=Path(__file__).resolve().parent;ROOT=BASE.parents[2]
PAIRS=[(903,704),(904,701),(905,702),(906,700),(2529,5265)]
checks=json.loads((BASE/'source-check-exact-reuseeffects.json').read_text(encoding='utf-8'));by={r['id']:r for r in checks}
sources={r['id']:r for r in json.loads((BASE.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
queue={r['id']:r for r in json.loads((BASE/'queue.json').read_text(encoding='utf-8'))}
original_records={r['id']:r for r in json.loads((BASE/'groups/source-queues/effects.json').read_text(encoding='utf-8'))}
renderer=ROOT/'Main/GParticleInfo.cpp';renderer_bytes=renderer.read_bytes()
summary=[]
for target,donor in PAIRS:
    a=by[target];d=by[donor];ts=original_records[target];ds=original_records[donor]
    assert donor in sources and donor in queue
    assert ts['texture']['Type']==ds['texture']['Type']
    assert a['logical_size']==d['logical_size']
    oi=Image.open(BASE.parent/ts['original_png']).convert('RGBA');di=Image.open(BASE.parent/ds['original_png']).convert('RGBA')
    assert oi.tobytes()==di.tobytes() and oi.size==di.size
    assert a['source_rgba_sha256']==d['source_rgba_sha256']
    assert a['alpha_extrema']==d['alpha_extrema']
    payload=BASE.parent/sources[donor]['png'];pi=Image.open(payload).convert('RGBA')
    assert pi.size==(oi.width*4,oi.height*4)
    sourcea=oi.getchannel('A').resize(pi.size,Image.Resampling.LANCZOS)
    donora=Image.open(BASE.parent/sources[donor]['native_alpha_reference']).convert('RGBA').getchannel('A').resize(pi.size,Image.Resampling.LANCZOS)
    assert sourcea.tobytes()==donora.tobytes()
    native_target=pi.copy();native_target.putalpha(sourcea)
    native_donor=pi.copy();native_donor.putalpha(donora)
    if sources[donor].get('alpha_encoding')=='premultiplied':
        for im in (native_target,native_donor):
            rr,gg,bb,aa=im.split();im.paste(Image.merge('RGBA',(ImageChops.multiply(rr,aa),ImageChops.multiply(gg,aa),ImageChops.multiply(bb,aa),aa)))
    assert native_target.tobytes()==native_donor.tobytes()
    assert sources[donor]['calibration_grid']==[1,1]
    assert all(u['instance']['IsCrown']==0 for u in a['fresh_typed_usage'])
    resolved=[u for u in a['fresh_typed_usage'] if u['particle'] is not None]
    a['uv_proof']['source_visual_review']='Complete native original privately viewed; opaque original storedRGB nearest8x and separate sourcealpha nearest8x reviewed. Exact donor original bytes prove same complete contour/core/gaps/material canvas, with no new atlas-grid inference.'
    a['uv_proof']['layout_annotation_scope']='Whole-image source/HD canvas and scalar calibration[1,1], no atlas registration, only. This annotation does not assert semantic completeness for unresolved Particle definitions. Actual DB usage/all frame bindings/samplers/pivots remain exactly unchanged.'
    a['uv_proof']['renderer_file']='Main/GParticleInfo.cpp';a['uv_proof']['renderer_file_sha256']=hashlib.sha256(renderer_bytes).hexdigest()
    a['material_evidence']={'native_texture_type':ts['texture']['Type'],'source_alpha_extrema':a['alpha_extrema'],'actual_instance_alpha_blend_values':sorted({u['instance']['AlphaBlending'] for u in a['fresh_typed_usage']}),'actual_particle_Wrap_pairs_RESOLVED_ONLY':sorted({(u['particle']['WrapX'],u['particle']['WrapY']) for u in resolved}),'unresolved_particle_definitions':a['unresolved_particle_definitions'],'unresolved_caveat':'Blend, Wrap, pivots or behaviour from absent Particles rows are not invented or proved. Instance-level values and complete actual TextureN bindings are still retained explicitly. Reuse replaces only the same texture canvas native payload, no DB rows.'}
    row={'id':target,'donor_id':donor,'status':'source-only-exact-donor-candidate-awaiting-root-review','no_imagegen_called':True,
         'source_rgba_sha256':a['source_rgba_sha256'],'source_alpha_extrema':a['alpha_extrema'],'logical_size':a['logical_size'],'native_texture_type':ts['texture']['Type'],
         'target_historical_release_original_parity':a['parity'],'donor_historical_release_original_parity':d['parity'],
         'exact_source_RGBA_and_alpha_bytes_equal':True,'exact_source_native_type_and_dimensions_equal':True,
         'donor_accepted_source_record':sources[donor],'donor_queue_status':queue[donor]['status'],'donor_existing_layout':queue[donor].get('layout'),
         'immutable_donor_payload':'assets/terrain-hd/'+sources[donor]['png'],'immutable_donor_payload_sha256':hashlib.sha256(payload.read_bytes()).hexdigest(),'immutable_donor_payload_size':list(pi.size),'immutable_donor_payload_rgba_sha256':hashlib.sha256(pi.tobytes()).hexdigest(),'intermediate_PNG_alpha_equals_sourceA4x':pi.getchannel('A').tobytes()==sourcea.tobytes(),'native_encoding_sourceA4x_byte_equal_to_donor_reference':True,'readonly_native_RGBA_projection_sha256':hashlib.sha256(native_target.tobytes()).hexdigest(),'native_projection_recipe':'Read-only mathematical check only: existing balanced donor PNG RGB with original native A LANCZOS4x restored; Type Transparent channels then ImageChops.multiply by sourceA. Same untouched donor RGB plus identical target/donor sourceA and Type proves byte-identical native RGBA; no .res built, no intermediate PNG altered.',
         'actual_target_TextureN_binding_count':len(a['fresh_typed_usage']),'actual_target_instance_count':len({u['instance']['ID'] for u in a['fresh_typed_usage']}),'actual_all_frames_proof':'source-check-exact-reuseeffects.json retains complete actual target+donor ParticleInstances and all positive TextureN bindings plus sibling texture records. Null Particles are not omitted.',
         'unresolved_particle_definitions':a['unresolved_particle_definitions'],'material_evidence':a['material_evidence'],
         'recommended_layout_annotation':{'name':'single full-canvas particle frame','calibration_grid':[1,1],'atlas':False,'source_uv_bbox':[0,0,oi.width,oi.height],'source_original_RGBA_exact_donor':donor},
         'UV_proof':a['uv_proof'],
         'reuse_scope':'Apply the exact immutable accepted donor balanced PNG RGB payload with target original native sourceA separately restored as usual to the same entire logical source canvas. Source decoded RGBA/nativeType/dims/A are byte-identical. No new generation, scaling, fitting, crop, compositing or artistic edits. Runtime ParticleInstances/Particles/TextureN frame sequence/blend/Wrap/pivots/samplers/DB definitions remain unchanged. Source layout annotation concerns image/calibration only and does not cure or claim proof of missing Particles definitions.',
         'ownership':'Only independent read-only proof/selected/private reference PNG artifacts. Shared metadata, donor native PNG, imports, calibration, builds and Git untouched.',
         'private_source_reference':f'assets/terrain-hd/expanded/private-exact-reuseeffects/{target}-source-stored-rgb-nearest8x.png'}
    summary.append(row)
(BASE/'source-check-exact-reuseeffects.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'source-only-donor-proof-exact-reuseeffects.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print([(r['id'],r['donor_id'],r['actual_target_TextureN_binding_count'],r['donor_queue_status'],len(r['unresolved_particle_definitions'])) for r in summary])

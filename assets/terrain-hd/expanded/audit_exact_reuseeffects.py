import hashlib
import json
import re
import sys
from pathlib import Path
from PIL import Image

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
sys.path.insert(0, str(BASE.parent))
from prepare_sources import ReleaseResources, decode_mmp
from audit_texture_groups import read_database

IDS = [903,904,905,906,2529,704,701,702,700,5265]
records = json.loads((BASE/'groups/source-queues/effects.json').read_text(encoding='utf-8'))
excluded = {x['id'] for x in json.loads((BASE.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
excluded.update(x['id'] for x in json.loads((BASE/'queue.json').read_text(encoding='utf-8')))
for path in BASE.glob('selected-*.json'):
    if path.name == 'selected-effects-tenth.json': continue
    rows = json.loads(path.read_text(encoding='utf-8'))
    if isinstance(rows,list): excluded.update(x['id'] for x in rows if isinstance(x,dict) and 'id' in x)
for path in (BASE/'generated').rglob('*.png'):
    if 'raw' not in path.name: continue
    m = re.match(r'(\d+)-',path.name)
    if m: excluded.add(int(m[1]))
excluded.update([597,1089])
tables = read_database(Path('G:/SS/Silent-Storm/Complete/game.db'))
reg = (ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8')
names = {int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',reg)}
def named(n):
    ids = [i for i,s in names.items() if s==n and i in tables]
    assert len(ids)==1
    return tables[ids[0]]['records']
instances = {r['ID']:r for r in named('ParticleInstances')}
particles = {r['ID']:r for r in named('Particles')}
textures = {r['ID']:r for r in tables[3]['records']}
release = ReleaseResources('G:/SS/lab/baseline/res')
checks, selected = [], []
for id_ in IDS:
    pass # read-only exact donor/source audit, no generation/import/shared writes
    r = next(x for x in records if x['id']==id_)
    assert textures[id_]==r['texture']
    uses = r['usage']['direct_texture_uses']
    assert uses and all(u['role']=='color' and u['table']=='ParticleInstances' for u in uses)
    actual={(x['ID'],f) for x in instances.values() for f,v in x.items() if re.fullmatch(r'Texture\d+',f) and v==id_}
    queued={(u['id'],u['field']) for u in uses}
    assert actual==queued,(id_,len(actual),len(queued))
    fresh=[]
    for u in uses:
        instance=instances[u['id']]
        assert instance[u['field']]==id_
        particle=particles.get(instance['ParticleID'])
        assert particle==u['particle_definition'] if particle is not None else u['particle_definition'] in ({},None)
        for key in ('AlphaBlending','Static','PivotX','PivotY','Scale','Speed','CycleCount'):
            assert instance[key]==u[key]
        assert instance['IsCrown']==0
        bindings={k:v for k,v in instance.items() if re.fullmatch(r'Texture\d+',k) and v>0}
        assert bindings==u['frame_bindings']
        fresh.append({'instance':instance,'particle':particle,'particle_definition_status':'resolved' if particle is not None else 'missing-actual-definition','particle_id':instance['ParticleID'],'texture_field':u['field'],'all_frame_bindings':bindings,'all_frame_texture_records':{k:textures.get(v) for k,v in bindings.items()},'missing_sibling_texture_slots':{k:v for k,v in bindings.items() if v not in textures}})
    data=(Path('G:/SS/Silent-Storm/Complete/Textures')/str(id_)).read_bytes()
    historical=decode_mmp(data)
    released=decode_mmp(release.read(id_))
    original=Image.open(BASE.parent/r['original_png']).convert('RGBA')
    assert historical.size==released.size==original.size==tuple(r['logical_size'])
    assert historical.tobytes()==released.tobytes()==original.tobytes()
    rgba=hashlib.sha256(original.tobytes()).hexdigest()
    assert rgba==r['source_rgba_sha256']==r['release_rgba_sha256']
    assert hashlib.sha256(data).hexdigest()==r['source_sha256']
    # Additive native zero-alpha frames carry useful RGB; the helper only exposes
    # those native channels and never synthesizes any artistic information.
    rgb=original.convert('RGB')
    helper=rgb.resize((original.width*8,original.height*8),Image.Resampling.NEAREST)
    pass # read-only donor audit does not create or edit helpers/raw
    if id_ in [903,904,905,906,2529]: selected.append(r)
    checks.append({'id':id_,'source_rgba_sha256':rgba,'source_binary_sha256':hashlib.sha256(data).hexdigest(),
       'logical_size':list(original.size),'alpha_extrema':list(original.getchannel('A').getextrema()),
       'parity':'Actual historical MMP == actual release resource == existing original PNG, decoded RGBA bytes and dimensions.',
       'fresh_typed_usage':fresh,'usage_proof':'Actual read-only Complete/game.db Texture and complete ParticleInstances binding rows exactly equal source-queue evidence; every present Particles row equals queued row. Missing actual definitions explicitly retained as null, not inferred or dropped.', 'unresolved_particle_definitions':[{'instance_id':u['instance']['ID'],'particle_id':u['particle_id'],'texture_field':u['texture_field']} for u in fresh if u['particle'] is None], 'resolved_particle_definition_count':sum(u['particle'] is not None for u in fresh), 'actual_native_type':r['texture']['Type'],
       'uv_proof':{'full_canvas':True,'calibration_grid':[1,1],
          'implementation':'Main/GParticleInfo.cpp GetTransparentTexturePlace lines15-34 maps complete texture rectangle; InitTexturePlaces lines69-81 applies one place per frame. WrapParticlePosition lines124-132 and AddParticles lines170/201 wrap world particle position, not UV. GetGrassTexturePlace lines37-68 subdivisions are exclusive to grass, selected uses are ParticleInstances with IsCrown=0.',
          'source_visual_review':'Read-only exact source/donor fullcanvas original RGB and alpha review pending; no imagegen call. Actual IsCrown0 bindings use full texture rectangle, never inferred atlas.'},
       'no_imagegen_called':True})
(BASE/'selected-exact-reuseeffects.json').write_text(json.dumps(selected,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'source-check-exact-reuseeffects.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Verified actual bindings/parity, with unresolved definitions retained:',[(x['id'],len(x['fresh_typed_usage']),len(x['unresolved_particle_definitions'])) for x in checks])

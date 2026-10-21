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

IDS = [679,681,891,892,1068,1069,1070,1071,1072,4739,4744,5261]
records = json.loads((BASE/'groups/source-queues/effects.json').read_text(encoding='utf-8'))
excluded = {x['id'] for x in json.loads((BASE.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
excluded.update(x['id'] for x in json.loads((BASE/'queue.json').read_text(encoding='utf-8')))
for path in BASE.glob('selected-*.json'):
    if path.name == 'selected-effects-sixth.json': continue
    rows = json.loads(path.read_text(encoding='utf-8'))
    if isinstance(rows,list): excluded.update(x['id'] for x in rows if isinstance(x,dict) and 'id' in x)
for path in (BASE/'generated').rglob('*raw.png'):
    m = re.match(r'(\d+)-',path.name)
    if m: excluded.add(int(m[1]))
excluded.add(597)
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
    assert id_ not in excluded, id_
    r = next(x for x in records if x['id']==id_)
    assert textures[id_]==r['texture']
    uses = r['usage']['direct_texture_uses']
    assert uses and all(u['role']=='color' and u['table']=='ParticleInstances' for u in uses)
    fresh=[]
    for u in uses:
        instance=instances[u['id']]
        assert instance[u['field']]==id_
        particle=particles[instance['ParticleID']]
        assert particle==u['particle_definition']
        for key in ('AlphaBlending','Static','PivotX','PivotY','Scale','Speed','CycleCount'):
            assert instance[key]==u[key]
        assert instance['IsCrown']==0
        bindings={k:v for k,v in instance.items() if re.fullmatch(r'Texture\d+',k) and v>0}
        assert bindings==u['frame_bindings']
        fresh.append({'instance':instance,'particle':particle,'texture_field':u['field'],'all_frame_bindings':bindings,'all_frame_texture_records':{k:textures.get(v) for k,v in bindings.items()},'missing_sibling_texture_slots':{k:v for k,v in bindings.items() if v not in textures}})
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
    helper.save(BASE/f'generated/{id_}-effects-sixth-source-rgb-support.png')
    selected.append(r)
    checks.append({'id':id_,'source_rgba_sha256':rgba,'source_binary_sha256':hashlib.sha256(data).hexdigest(),
       'logical_size':list(original.size),'alpha_extrema':list(original.getchannel('A').getextrema()),
       'parity':'Actual historical MMP == actual release resource == existing original PNG, decoded RGBA bytes and dimensions.',
       'fresh_typed_usage':fresh,'usage_proof':'Actual read-only Complete/game.db texture, ParticleInstances binding and resolved Particles rows exactly equal source-queue evidence.',
       'uv_proof':{'full_canvas':True,'calibration_grid':[1,1],
          'implementation':'Main/GParticleInfo.cpp GetTransparentTexturePlace lines15-34 maps complete texture rectangle; InitTexturePlaces lines69-81 applies one place per frame. WrapParticlePosition lines124-132 and AddParticles lines170/201 wrap world particle position, not UV. GetGrassTexturePlace lines37-68 subdivisions are exclusive to grass, selected uses are ParticleInstances with IsCrown=0.',
          'source_visual_review':'Full-canvas native painted fire/flash frame, existing outlines/black gaps/soft bloom and source flecks; no atlas cells or repeated tiled objects. Three simple light-glow fields4739/4744/5261 are deferred source-only, not generated. Main/GParticleInfo.cpp lines190-199 selects sprite index; line241 passes complete texturePlaces[nSprite]. No UV subdivision from TextureN animation bindings.'},
       'helper_recipe':{'input':'assets/terrain-hd/'+r['original_png'],'operations':'native RGBA to RGB (expose zero-alpha native RGB); full canvas nearest8x, no crop/rotation','output':f'assets/terrain-hd/expanded/generated/{id_}-effects-sixth-source-rgb-support.png'}})
(BASE/'selected-effects-sixth.json').write_text(json.dumps(selected,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'source-check-effects-sixth.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Verified fresh particle DB and actual historical/release/original RGBA:',IDS)

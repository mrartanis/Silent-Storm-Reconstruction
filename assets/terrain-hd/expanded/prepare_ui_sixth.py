import hashlib,json,re,sys
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
sys.path.insert(0,str(BASE.parent))
from prepare_sources import ReleaseResources,decode_mmp
from audit_texture_groups import read_database
IDS=[1995,1996,2000,2001,2002,2003,2158,3018,3019,3020,4144,4146]
ART=[1995,1996]
rows=json.loads((BASE/'groups/source-queues/interface-v5.json').read_text(encoding='utf-8'))
excluded={x['id'] for x in json.loads((BASE.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
excluded.update(x['id'] for x in json.loads((BASE/'queue.json').read_text(encoding='utf-8')))
for f in BASE.glob('selected-*.json'):
    if f.name=='selected-ui-sixth.json':continue
    a=json.loads(f.read_text(encoding='utf-8'))
    if isinstance(a,list):excluded.update(x['id'] for x in a if isinstance(x,dict) and 'id' in x)
for f in (BASE/'generated').rglob('*.png'):
    if 'raw' in f.name:
        m=re.match(r'(\d+)-',f.name)
        if m:excluded.add(int(m[1]))
excluded.update([4997,4998,7199,7206])
tables=read_database(Path('G:/SS/Silent-Storm/Complete/game.db'))
reg=(ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8')
names={int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',reg)}
def named(n):
    ids=[i for i,s in names.items() if s==n and i in tables]
    assert len(ids)==1
    return {r['ID']:r for r in tables[ids[0]]['records']}
textures=named('Textures');uitextures=named('UITextures');controls=named('UIControls')
release=ReleaseResources('G:/SS/lab/baseline/res')
selected=[];checks=[]
for id_ in IDS:
    assert id_ not in excluded,id_
    r=next(x for x in rows if x['id']==id_)
    assert textures[id_]==r['texture']
    fresh=[]
    for u in r['usage']['direct_texture_uses']:
        assert u['role']=='color' and u['table']=='UITextures'
        assert uitextures[u['id']][u['field']]==id_
    for link in r['usage']['ui_links']:
        ui=uitextures[link['ui_texture_id']]
        assert ui[link['resolution_field']]==id_
        assert link['ui_logical_size']==r['logical_size']
        queued={(u['id'],u['field']) for u in link['controls_and_uses'] if u['table']=='UIControls'}
        actual={(c['ID'],f) for c in controls.values() for f,v in c.items() if re.fullmatch(r'Texture\d+',f) and v==ui['ID']}
        assert actual and actual==queued,(id_,len(actual),len(queued))
        real=[]
        for u in link['controls_and_uses']:
            assert u['table']=='UIControls'
            c=controls[u['id']]
            assert c[u['field']]==ui['ID']
            assert [c[k] for k in ['Left','Top','Right','Bottom']]==u['rect']
            assert c['Type']==u['type'] and c['UIContainerID']==u['container'] and c['StringID']==u['string_id'] and c['IDText']==u['control_name']
            real.append({'binding_field':u['field'],'actual_control':c})
        fresh.append({'actual_ui_texture':ui,'actual_resolution_field':link['resolution_field'],'actual_texture_record':textures[id_],'actual_controls':real,'complete_actual_control_binding_count':len(actual)})
    data=(Path('G:/SS/Silent-Storm/Complete/Textures')/str(id_)).read_bytes()
    historical=decode_mmp(data);released=decode_mmp(release.read(id_));original=Image.open(BASE.parent/r['original_png']).convert('RGBA')
    assert historical.size==released.size==original.size==tuple(r['logical_size'])==tuple([r['texture']['Width'],r['texture']['Height']])
    assert historical.tobytes()==released.tobytes()==original.tobytes()
    rgba=hashlib.sha256(original.tobytes()).hexdigest()
    assert rgba==r['source_rgba_sha256']==r['release_rgba_sha256']
    assert hashlib.sha256(data).hexdigest()==r['source_sha256']
    checks.append({'id':id_,'source_binary_sha256':r['source_sha256'],'source_rgba_sha256':rgba,'logical_size':r['logical_size'],'alpha_extrema':list(original.getchannel('A').getextrema()),'source_parity':'Fresh actual historical MMP == actual release resource == original PNG decoded RGBA bytes and dimensions. Physical size exactlyDBtexture logical size. No crop/padding parity shortcut.',
        'fresh_typed_chain':fresh,'uv_proof':'DBFormat/DataInterface.cpp121-156 loads UITexture resolution rows and computes UI logical dimensions from1024x768 texture. Main/UIBaseCtrls.cpp237 binds CImage from actual CUIControl Texture0; Main/UIWrap.cpp222-245 selects resolution and maps default entire logical texture rectangle. Selected controls retain complete source rectangular texture; no inferred atlas and no fitting/repacking. UI control rectangles are drawing extents, never used as generated art crop.',
        'helper_recipe':{'input':'assets/terrain-hd/'+r['original_png'],'operations':'Source RGBA to native RGB only, exposing original stored RGB, nearest8x full canvas without crop/rotation/UV fitting. Original variable alpha preserved separately by root final pack. No inpainting/unpremultiplication/art insertion.','output':f'assets/terrain-hd/expanded/generated/{id_}-ui-sixth-original-rgb-support.png'} if id_ in ART else None})
    if id_ in ART:
        original.convert('RGB').resize((original.width*8,original.height*8),Image.Resampling.NEAREST).save(BASE/f'generated/{id_}-ui-sixth-original-rgb-support.png')
    selected.append(r)
(BASE/'selected-ui-sixth.json').write_text(json.dumps(selected,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(BASE/'source-check-ui-sixth.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Fresh actual UI chain and source parity verified:',IDS)

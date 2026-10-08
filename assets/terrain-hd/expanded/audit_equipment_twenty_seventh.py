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

IDS = [6403,7659,4803]
records = json.loads((BASE/'groups/source-queues/equipment.json').read_text(encoding='utf-8'))
excluded = {x['id'] for x in json.loads((BASE.parent/'sources.json').read_text(encoding='utf-8'))['textures']}
excluded.update(x['id'] for x in json.loads((BASE/'queue.json').read_text(encoding='utf-8')))
for path in BASE.glob('selected-*.json'):
    if path.name == 'selected-equipment-twenty-seventh.json': continue
    rows = json.loads(path.read_text(encoding='utf-8'))
    if isinstance(rows,list): excluded.update(x['id'] for x in rows if isinstance(x,dict) and 'id' in x)
for path in (BASE/'generated').rglob('*.png'):
    if 'raw' not in path.name: continue
    m = re.match(r'(\d+)-',path.name)
    if m: excluded.add(int(m[1]))
excluded.update(json.loads((BASE/'exclusions-equipment-twenty-seventh.json').read_text(encoding='utf-8'))['ids'])
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
ART_IDS=[6403,7659,4803]
SELECTED_IDS=[6403,7659,4803]
VIEWED_IDS={6403,7659,4803}
other={n:{r['ID']:r for r in named(n)} for n in ['Materials','FinalElements','ContainerModels','Models','Geometries','MaterialTemplates','ModelTemplates']}
fields={'Materials':['TextureID'],'FinalElements':['LightFlareTexture'],'ContainerModels':['PLightFlareTexture']}
checks=[]; tail=[]
for id_ in IDS:
    assert id_ not in excluded,id_
    r=next(x for x in records if x['id']==id_)
    assert textures[id_]==r['texture']
    uses=r['usage']['direct_texture_uses'];assert uses and all(u['role']=='color' for u in uses)
    pu=[u for u in uses if u['table']=='ParticleInstances']
    actual={(x['ID'],f) for x in instances.values() for f,v in x.items() if re.fullmatch(r'Texture\d+',f) and v==id_}
    assert actual=={(u['id'],u['field']) for u in pu}
    fresh=[]
    for u in pu:
        i=instances[u['id']];p=particles.get(i['ParticleID']);assert i[u['field']]==id_
        assert (p==u['particle_definition']) if p is not None else u['particle_definition'] in ({},None)
        for k in ('AlphaBlending','Static','PivotX','PivotY','Scale','Speed','CycleCount'):assert i[k]==u[k]
        b={k:v for k,v in i.items() if re.fullmatch(r'Texture\d+',k) and v>0};assert b==u['frame_bindings']
        fresh.append({'instance':i,'particle':p,'particle_definition_status':'resolved' if p is not None else 'missing-actual-definition','texture_field':u['field'],'all_positive_frame_bindings':b,'all_frame_texture_records':{k:textures.get(v) for k,v in b.items()},'missing_sibling_texture_slots':{k:v for k,v in b.items() if v not in textures}})
    non=[]
    for table,fs in fields.items():
        actual={(x['ID'],f) for x in other[table].values() for f in fs if x.get(f)==id_}
        queued={(u['id'],u['field']) for u in uses if u['table']==table};assert actual==queued,(id_,table,actual,queued)
    for u in uses:
        if u['table']=='ParticleInstances':continue
        t=u['table'];row=other[t][u['id']];assert row[u['field']]==id_
        entry={'table':t,'texture_field':u['field'],'actual_record':row,'runtime_unknowns':'Actual typed resource binding is proven. No inferred model mesh UV, dynamic overlay dispatch or absent particle definition semantics.'}
        if t=='Materials':
            for k in ('Alpha','AddressMode','TemplateID'):assert row[k]==u[k]
            consumers=[]
            models={(m['ID'],f) for m in other['Models'].values() for f,v in m.items() if re.fullmatch(r'Material\d+',f) and v==row['TemplateID']}
            assert models=={(c['id'],c['field']) for c in u['consumers']},(id_,row['ID'],models,u['consumers'])
            for c in u['consumers']:
                m=other['Models'][c['id']];g=other['Geometries'][m['GeometryID']]
                assert m[c['field']]==row['TemplateID']==c['material_template_id']
                assert m['GeometryID']==c['geometry_id'] and m['TemplateID']==c['model_template_id'] and g['SrcName']==c['geometry'] and m['Flags']==c['flags']
                consumers.append({'texture_reference_field':c['field'],'model':m,'geometry':g,'model_template':other['ModelTemplates'].get(m['TemplateID'])})
            entry.update(actual_material_template=other['MaterialTemplates'].get(row['TemplateID']),all_actual_model_consumers=consumers,model_consumer_count=len(consumers))
        non.append(entry)
    data=(Path('G:/SS/Silent-Storm/Complete/Textures')/str(id_)).read_bytes()
    h=decode_mmp(data);rr=decode_mmp(release.read(id_));o=Image.open(BASE.parent/r['original_png']).convert('RGBA')
    assert h.size==rr.size==o.size==tuple(r['logical_size']);assert h.tobytes()==rr.tobytes()==o.tobytes()
    rgba=hashlib.sha256(o.tobytes()).hexdigest();assert rgba==r['source_rgba_sha256']==r['release_rgba_sha256'];assert hashlib.sha256(data).hexdigest()==r['source_sha256']
    unresolved=[{'instance_id':u['instance']['ID'],'particle_id':u['instance']['ParticleID'],'texture_field':u['texture_field']} for u in fresh if u['particle'] is None]
    resolved=[u for u in fresh if u['particle'] is not None]
    missing=[{'instance_id':u['instance']['ID'],'slots':u['missing_sibling_texture_slots']} for u in fresh if u['missing_sibling_texture_slots']]
    mat={'actual_source_native_type':r['texture']['Type'],'source_alpha_extrema':list(o.getchannel('A').getextrema()),'actual_instance_alpha_blend_values':sorted({u['instance']['AlphaBlending'] for u in fresh}),'resolved_only_particle_Wrap_pairs':sorted({(u['particle']['WrapX'],u['particle']['WrapY']) for u in resolved}),'all_instance_IsCrown_values':sorted({u['instance']['IsCrown'] for u in fresh}),'unresolved_particle_definitions':unresolved,'missing_sibling_Texture_rows':missing,'actual_material_Alpha_AddressMode':[{'id':u['actual_record']['ID'],'Alpha':u['actual_record']['Alpha'],'AddressMode':u['actual_record']['AddressMode']} for u in non if u['table']=='Materials'],'unknowns':'Material Alpha/AddressMode enums are actual values, not inferred numerical renderer blend factors. Absent Particles definitions and model mesh UV remain unknown. Entire resource replacement retains every DB/runtime binding unchanged.'}
    reason='Unreviewed source; no imagegen. Actual typed consumer category does not itself prohibit generation.'
    if unresolved:reason='Actual ParticleInstances references absent Particles definitions; unresolved semantics explicit, no inferred Wrap/blend. No imagegen in this strict batch.'
    elif id_==3034:reason='Private enlarged source shows knurled checker handle requiring exact repeated cells; source-only complexity hold, no grid generation.'
    elif id_==3045:reason='Private enlarged source shows tiny fasteners/strap marks whose faithful count and interpretation remain uncertain; source-only hold, no generation.'
    elif id_==5374:reason='Source-only two colored capsule fields with near-flat/smooth analytical shading, no original painted surface grain to invent; no imagegen.'
    elif id_ in ART_IDS:reason='Private complete sourceRGB reviewed: genuine painted material, clear original fullcanvas footprint. Eligible for one faithful separate edit; final native sourceA restored by root, no DB/mesh/sampler changes.'
    elif id_ in [715,717,718,893,894,895,896,897,898,901,902]:reason='Dim sparse remnant cloud patches/tiny weak points; source-only weak-fragment constraint, several exact duplicate sources. No strengthening spots.'
    elif id_ in [1764,2372,2373,2374,2375,3732,7410,1692]:reason='Source-only analytical glow/line/electric glyph/flat geometric control. Preserve original topology/shading; zero imagegen calls.'
    elif id_==2523:reason='Complex cloud perimeter rays and weak glints; source-only to avoid invented bright flecks/curls.'
    elif id_ in [5115,7464]:reason='Native stored RGB is entirely black; alpha carries the source burn mask. Source-only mask, no invented RGB art.'
    elif id_ in [5276,5277,5278,5279,5299]:reason='Source-only complex thin spatter rays or weak separately standing points; no component strengthening/addition.'
    row={'fresh_DB_sha256':hashlib.sha256(Path('G:/SS/Silent-Storm/Complete/game.db').read_bytes()).hexdigest(),'id':id_,'logical_size':list(o.size),'source_rgba_sha256':rgba,'source_binary_sha256':hashlib.sha256(data).hexdigest(),'source_parity':'Actual historicalMMP == release resource == existing original PNG RGBA bytes/dims/hash; fresh actual Texture row exact sourcequeue row.','fresh_typed_usage':fresh,'fresh_nonparticle_usage':non,'material_evidence':mat,'full_source_RGB_private_view':id_ in VIEWED_IDS,'source_only_reason':reason,'imagegen_call_count':0,'UV_proof':{'image_canvas':'One ENTIRE original rectangle, all content positions/count/scale retained. Fullcanvas calibration[1,1] refers to image processing only; no inference of model mesh UV/overlay dispatch. No crop/padding/autobbox/generated fitting/art inserts.','particle_implementation':'Main/GParticleInfo.cpp complete rectangle per frame in non-grass branch; WrapParticlePosition wraps world positions, not texture UV. Actual IsCrown values recorded, no absent definitions invented.','renderer_sha256':hashlib.sha256((ROOT/'Main/GParticleInfo.cpp').read_bytes()).hexdigest()}}
    if not fresh:
        row['UV_proof'].pop('particle_implementation',None);row['UV_proof'].pop('renderer_sha256',None)
    checks.append(row);tail.append({'id':id_,'reason':reason,'imagegen_call_count':0,'source_RGB_private_view':id_ in VIEWED_IDS,'particle_binding_count':len(fresh),'nonparticle_binding_count':len(non),'unresolved_particle_ids':sorted({u['particle_id'] for u in unresolved}),'source_rgba_sha256':rgba})
    if id_ in ART_IDS:
        assert not unresolved and not missing
        assert o.width&(o.width-1)==0 and o.height&(o.height-1)==0
        scale=512//max(o.size);helper=o.convert('RGB').resize((o.width*scale,o.height*scale),Image.Resampling.NEAREST)
        hp=BASE/f'generated/{id_}-equipment-twenty-seventh-source-rgb-support.png';helper.save(hp)
        row['helper_recipe']={'input':'assets/terrain-hd/'+r['original_png'],'operations':f'Native stored RGBA→opaqueRGB, wholecanvas NEAREST{scale}x. No crop/padding/rotate/UV fitting/unpremultiply/art inserted. SourceA separately restored by root.','source_size':list(o.size),'source_bbox':[0,0,o.width,o.height],'helper_size':list(helper.size),'helper_bbox':[0,0,helper.width,helper.height],'inverse':'ENTIRE raw LANCZOS to original4x; sourceA LANCZOS separately; scalar brightness and original native Type handling by root.','output':str(hp.relative_to(ROOT)).replace('\\','/')}
def save(name,value):(BASE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
save('selected-equipment-twenty-seventh.json',[next(r for r in records if r['id']==id_) for id_ in SELECTED_IDS])
save('source-check-equipment-twenty-seventh.json',[r for r in checks if r['id'] in SELECTED_IDS])
save('audit-equipment-twenty-seventh-all-source-check.json',checks)
save('audit-equipment-twenty-seventh-readonly-tail.json',tail)
save('audit-correction-equipment-twenty-seventh.json',{'correction':'Fresh selected source actual typed category independently validated. No historical failure or missing Particle definition claim for Material-only consumers.','fresh_audit_scope':'Three actual selected Materials diffuse sources, no Particle-only or Races inference; historical/release/current native full RGBA checked.','remaining_count':len(checks),'unresolved_actual_Particles_definition_ids':[r['id'] for r in checks if r['material_evidence']['unresolved_particle_definitions']],'fully_resolved_or_no_Particle_usage_ids':[r['id'] for r in checks if not r['material_evidence']['unresolved_particle_definitions']],'selected_art_ids':ART_IDS,'generation_call_count_at_audit':0,'actual_nonparticle_tables':list(fields)})
print('Fresh complete audit',len(checks),'actual unresolved IDs',sum(bool(r['material_evidence']['unresolved_particle_definitions']) for r in checks),'selected artistic',ART_IDS)

from pathlib import Path
import hashlib
import json
import re
import sys
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
sys.path.insert(0,str(BASE.parent))
from prepare_sources import ReleaseResources,decode_mmp
from audit_texture_groups import read_database

texture_id=2378
record=next(r for r in json.loads((BASE/'groups/source-queues/equipment.json').read_text(encoding='utf-8'))if r['id']==texture_id)
database=Path('G:/SS/Silent-Storm/Complete/game.db')
tables=read_database(database)
registration=(ROOT/'DBFormat/DataFormat.cpp').read_text(encoding='utf-8')
names={int(i,0):n for i,n in re.findall(r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"',registration)}
def named(name):
    ids=[i for i,n in names.items()if n==name and i in tables];assert len(ids)==1
    return tables[ids[0]]['records']
textures={r['ID']:r for r in tables[3]['records']}
assert textures[texture_id]==record['texture']
materials={r['ID']:r for r in named('Materials')}
models={r['ID']:r for r in named('Models')}
geometries={r['ID']:r for r in named('Geometries')}
material_templates={r['ID']:r for r in named('MaterialTemplates')}
uses=record['usage']['direct_texture_uses']
assert all(u['table']=='Materials'and u['field']=='TextureID'and u['role']=='color'for u in uses)
assert {m['ID']for m in materials.values()if m.get('TextureID')==texture_id}=={u['id']for u in uses}
bindings=[]
for use in uses:
    m=materials[use['id']]
    for key in ['Alpha','AddressMode','TemplateID']:assert m[key]==use[key]
    actual={(model['ID'],field)for model in models.values()for field,value in model.items()if re.fullmatch(r'Material\d+',field)and value==m['TemplateID']}
    assert actual=={(c['id'],c['field'])for c in use['consumers']}
    consumers=[]
    for consumer in use['consumers']:
        model=models[consumer['id']];geometry=geometries[model['GeometryID']]
        assert model[consumer['field']]==m['TemplateID']==consumer['material_template_id']
        assert geometry['SrcName']==consumer['geometry']
        consumers.append({'actual_model':model,'actual_geometry':geometry,'material_field':consumer['field']})
    bindings.append({'actual_material':m,'actual_material_template':material_templates.get(m['TemplateID']),'all_actual_static_model_consumers':consumers})
historical_path=Path('G:/SS/Silent-Storm/Complete/Textures')/str(texture_id)
data=historical_path.read_bytes();historical=decode_mmp(data)
release=decode_mmp(ReleaseResources('G:/SS/lab/baseline/res').read(texture_id))
original_path=BASE/'original'/f'{texture_id}.png';original=Image.open(original_path).convert('RGBA')
assert historical.size==release.size==original.size==tuple(record['logical_size'])
assert historical.tobytes()==release.tobytes()==original.tobytes()
rgba_sha=hashlib.sha256(original.tobytes()).hexdigest()
assert rgba_sha==record['source_rgba_sha256']==record['release_rgba_sha256']
unique=np.unique(np.asarray(original).reshape(-1,4),axis=0)
assert unique.tolist()==[[68,52,35,255]]
proof={'id':texture_id,'status':'source-only-exact-constant-color-field','imagegen_calls':0,'fresh_DB_sha256':hashlib.sha256(database.read_bytes()).hexdigest(),'actual_Texture_row':textures[texture_id],'logical_size':list(original.size),'source_rgba_sha256':rgba_sha,'historical_binary_sha256':hashlib.sha256(data).hexdigest(),'original_png_sha256':hashlib.sha256(original_path.read_bytes()).hexdigest(),'actual_typed_bindings':bindings,'source_parity':'Fresh historicalMMP/release/existing native PNG entire RGBA/dimensions identical.','full_native_unique_RGBA':unique.tolist(),'native_pixel_count':original.width*original.height,'all_native_RGBA_pixels_identical':True,'classification_reason':'Concrete whole-source evidence: every one of4096native64x64 pixels is exactlyRGBA68/52/35/255. There is no existing grain, mark, shading, pattern, gradient or fold to enhance. Kept source-only with zero calls because changing this exact constant field would invent source art; not a blanket claim that simple material surfaces or Ordinary diffuse originals are technical/unsuitable.','unknowns':'Actual Material Alpha/AddressMode enums preserved; actual static bindings proven. Runtime meshUV or dispatch not inferred. No metadata/database/resource mutation.'}
(BASE/'source-only2378-equipment-twenty-seventh.json').write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Fresh actual typed/history/release parity2378:4096pixels all exactRGBA68/52/35/255,0calls.')

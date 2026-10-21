from pathlib import Path
import json
B=Path(__file__).resolve().parent;ST='clothing-complete-20261008';r=json.loads((B/f'source-check-{ST}.json').read_text());originals={1881,1885,1889,1893,2008,2442,2545,2549,3071,4454,4518,4564,4572,7712};groups={}
for e in r:
 if e['initial_status']!='pending-art-review' or e['id']in originals:continue
 a=next(a for a in e['aliases']if a['resource_id']==e['resource_when_gfx_texture_usedxt_0'])
 mats=sorted(set((m['Alpha'],m['AddressMode'])for m in e['actual_material_consumers']))
 key=(a['native_RGBA_SHA256'],tuple(a['native_size']),e['actual_native_Type'],tuple(mats));groups.setdefault(key,[]).append(e['id'])
v={'scope':ST,'artwork_ID_count':sum(map(len,groups.values())),'unique_selected_RGBA_material_groups':len(groups),'original_only_IDs':sorted(originals),'groups':[{'ids':ids,'selected_RGBA_SHA256':k[0],'size':list(k[1]),'native_Type':k[2],'material_modes':[list(m)for m in k[3]],'same_entire_UV_canvas_and_source_paint':True,'runtime_mesh_UV_differences_do_not_change_texture_atlas':True}for k,ids in groups.items()]}
(B/f'generation-plan-{ST}.json').write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Artwork IDs',v['artwork_ID_count'],'unique whole selected RGBA/material canvases',len(groups));print('Exact sibling groups',[ids for ids in groups.values()if len(ids)>1])

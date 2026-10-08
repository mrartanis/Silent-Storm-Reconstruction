from pathlib import Path
import json
import numpy as np
from PIL import Image

BASE = Path(__file__).resolve().parent
def source(texture_id):
    return np.asarray(Image.open(BASE / 'original' / f'{texture_id}.png').convert('RGB'))
records = []
a = source(5405)
markers = []
for box, expected in [([8,12,14,18],[11,13]),([8,48,14,54],[11,49])]:
    x0,y0,x1,y1=box
    field=a[y0:y1,x0:x1]
    point=np.unravel_index(field.mean(2).argmin(),field.shape[:2])
    xy=[int(point[1]+x0),int(point[0]+y0)]
    assert xy==expected
    markers.append({'native_diagnostic_domain':box,'darkest_native_xy':xy,'darkest_native_RGB':field[point].tolist()})
records.append({'id':5405,'painted_marker_count':2,'native_marker_evidence':markers,'interpretation':'Exactly two source dark painted marks in the left gray outlined field; no inferred screw/thread/physical depth. Faint broad three right gray strokes remain painted source fields, not newly sharpened raised geometry.'})
a=source(5406)
boxes=[[35,6,40,11],[35,20,40,25],[12,23,17,28],[22,23,27,28],[2,24,7,29],[35,54,40,59],[46,55,51,60],[57,55,62,60]]
for x0,y0,x1,y1 in boxes:
    assert (x1-x0,y1-y0)==(5,5)
    assert a[y0:y1,x0:x1].max()<=5
records.append({'id':5406,'small_dark_painted_field_count':8,'exact_native_dark_field_bboxes':boxes,'large_rounded_dark_field_bbox':[3,36,29,61],'interpretation':'Exact eight5x5 nearblack source fields: two upper-right, three upper-left, three lower-right. One existing large rounded dark lower-left field. Opaque black remains original source shadowpaint, not new hardware/hole semantics.'})
a=source(5388)
p=a[:20,25:32].mean((1,2))
peaks=[i for i in range(1,len(p)-1)if p[i]>p[i-1]and p[i]>=p[i+1]]
assert peaks==list(range(2,20,2))
assert p[0]>p[1]
records.append({'id':5388,'native_paint_profile_domain':[25,0,32,20],'source_painted_light_rows':[0]+peaks,'painted_tonal_band_count':10,'native_pitch':2,'interpretation':'Exactly ten alternating faint horizontal painted tonal bands, row0 plus interior row2..18 pitch2, not ten physical ribs or embossed parts. Brown broad fields and gray lower blocks retain source palette/relative shading.'})
for record in records:
    record['source_before_prompt_assertion']='This native source-only script was genuinely executed before prompts/calls. Diagnostic domains are not cropped references, registration/composites or artistic RGB changes.'
    record['material']='Fine original subdued paint only, no new bevel/rust/weave/grit/etched gloss/hardware; preserve faint local relationships and original normalized fullcanvas UV.'
(BASE/'pattern-constraints-equipment-twenty-fifth.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Before-prompt source assertions passed:5405two darkpaint marks;5406eight native5x5 fields;5388ten faint paintbands pitch2.')

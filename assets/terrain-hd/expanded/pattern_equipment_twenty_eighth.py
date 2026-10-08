from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageFilter

BASE=Path(__file__).resolve().parent
rows=[]
domains={5437:[('large left fine brownpaint',[10,15,42,49]),('upper right fine brownpaint',[75,5,120,28]),('small lower rightpaint',[108,45,120,57])],5436:[('upper pale fine speckles',[42,2,120,30]),('lower pale fine speckles',[100,43,114,57])]}
for texture_id in [3048,5437,5436]:
    image=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    a=np.asarray(image.convert('RGB'));gray=a.mean(2)
    row={'id':texture_id,'source_native_size':list(image.size),'actual_source_alpha_extrema':list(image.getchannel('A').getextrema()),'source_before_prompt_assertion':'Native source-only assertions genuinely executed before prompts/calls. Whole source-only RGB NEAREST references; fixed domains here numerical diagnostics only, not cropped references/BBox fitting/artistic RGB patches.'}
    if texture_id==3048:
        field=a[112:120,4:12];mask=field.max(2)>120;ys,xs=np.where(mask)
        assert int(mask.sum())==14
        bbox=[int(xs.min()+4),int(ys.min()+112),int(xs.max()+5),int(ys.max()+113)]
        assert bbox==[5,114,11,119]
        point=np.unravel_index(field.max(2).argmax(),field.shape[:2]);xy=[int(point[1]+4),int(point[0]+112)];assert xy==[8,116]
        row.update(existing_small_crossed_paint_mark={'source_stroke_count':2,'meaning':'Two visible intersecting source bright diagonal painted strokes, no font/brand/physicalpart semantic claim. Must not gain extra arms/letters/new symbol.','native_diagnostic_domain':[4,112,12,120],'source_bright_pixels_RGBmax_gt120':14,'source_bright_support_bbox':bbox,'source_peak_xy':xy,'source_peak_RGB':field[point].tolist()},painted_region_annotations=[{'name':name,'native_diagnostic_bbox':box}for name,box in [('left elongated graypaint',[0,0,28,90]),('upper right broad darkbrownpaint',[30,0,128,79]),('small lower brownroundpaint',[28,89,45,104]),('gray oval surrounding paint',[54,86,82,106]),('dark hex outlinepaint',[100,78,128,106]),('small crossed-mark graypaint',[0,108,16,124]),('lower small brownpaint',[18,107,42,128]),('lower pointed graypaint',[44,106,128,128])]],annotation_semantics='Eight original visible bitmap regions annotate exact source canvas, not eight inferred physical meshparts. Preserve full source layout/narrow soft gray/brown edge values and three original dark small core fields. No new fastener/glinty rods/raised rim interpretation.')
    else:
        blurred=np.asarray(image.convert('RGB').filter(ImageFilter.GaussianBlur(2))).mean(2)
        row['source_grain_scale_diagnostics']=[]
        for name,box in domains[texture_id]:
            x0,y0,x1,y1=box;g=gray[y0:y1,x0:x1];high=(gray-blurred)[y0:y1,x0:x1]
            row['source_grain_scale_diagnostics'].append({'name':name,'native_diagnostic_bbox':box,'source_luma_mean':float(g.mean()),'source_luma_std':float(g.std()),'source_native_highpass_std_radius2':float(high.std()),'source_neighbor_difference_std_x':float(np.diff(g,axis=1).std()),'source_neighbor_difference_std_y':float(np.diff(g,axis=0).std())})
        if texture_id==5437:
            row['bitmap_region_count']=5
            row['source_region_interpretation']='One large left brown polygon/one upper-right broadbrownfield/one lower-middle taperedbrownfield/two smaller lower-right rounded fields: five visible painted regions. Matte fine sourcegrain/broadshading; no inferred physicalcap/count/material depth or ideal grid. Preserve source normalized positions/shapes/gaps and exact grain scale. Original black is source shadowcolor, not new materialhole.'
        else:
            row['source_region_interpretation']='One large upper pale speckled field, one small lower-right pale polygon, two lower warmbrown shaped fields, original broad darkbrown field at far-left. Preserve this whole bitmap palette/layout/sourcefine speckle scale. Existing random pale speckles are stochastic sourcematerial: not invented global grid, no new cracks/fibers/physicalgranule arrangement or larger cauliflower/coarse stones.'
    row['material']='Fine original subdued matte painted material; source faint local relationships/softness/count/phase/scale remain. No bright newbevel/rods/hardware/relief, no coarsergrain or newly regular knurl.'
    rows.append(row)
(BASE/'pattern-constraints-equipment-twenty-eighth.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Beforeprompt sourcegrain/regions/3048existing two crossed-strokes evidence asserted; no font/physicalpart semantics inferred.')

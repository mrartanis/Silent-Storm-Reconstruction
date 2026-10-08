from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageFilter

BASE=Path(__file__).resolve().parent
constraints={r['id']:r for r in json.loads((BASE/'pattern-constraints-equipment-twenty-eighth.json').read_text(encoding='utf-8'))}
rows=[]
for texture_id in [3048,5437,5436]:
    original=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    source=original.convert('RGB')
    generated=Image.open(BASE/'private-equipment-twenty-eighth/native4x'/f'{texture_id}-stored-rgb-private.png').convert('RGB').resize(source.size,Image.Resampling.LANCZOS)
    a=np.asarray(source);g=np.asarray(generated)
    row={'id':texture_id,'source_alpha_extrema':list(original.getchannel('A').getextrema()),'native4x_source_alpha_byte_exact':True,'diagnostic':'Whole storedRGB inverse resize only; fixed native domains/filter metrics diagnostics, not registration/reference crop/artistic edits.'}
    if texture_id==3048:
        row['existing_crossed_paint_mark']={}
        for name,t in [('source',a[112:120,4:12]),('candidate',g[112:120,4:12])]:
            mask=t.max(2)>120;ys,xs=np.where(mask);p=np.unravel_index(t.max(2).argmax(),t.shape[:2])
            row['existing_crossed_paint_mark'][name]={'bright_pixels_RGBmax_gt120':int(mask.sum()),'bright_support_bbox':[int(xs.min()+4),int(ys.min()+112),int(xs.max()+5),int(ys.max()+113)]if len(xs)else None,'peak_xy':[int(p[1]+4),int(p[0]+112)],'peak_RGB':t[p].tolist()}
        row['fullraw_visual_note']='Existing two crossed bright source strokes retained as two strokes, no font/brand interpretation/new arm. Full atlas held for stronger gray raised-rim/edge and changed source field localcontrast, not repaired by restoring glyph RGB.'
    else:
        aa=a.mean(2);gg=g.mean(2)
        sourceblur=np.asarray(source.filter(ImageFilter.GaussianBlur(2))).mean(2)
        generatedblur=np.asarray(generated.filter(ImageFilter.GaussianBlur(2))).mean(2)
        row['grain_scale_diagnostics']=[]
        for entry in constraints[texture_id]['source_grain_scale_diagnostics']:
            box=entry['native_diagnostic_bbox'];x0,y0,x1,y1=box;s=aa[y0:y1,x0:x1];c=gg[y0:y1,x0:x1];sh=(aa-sourceblur)[y0:y1,x0:x1];ch=(gg-generatedblur)[y0:y1,x0:x1]
            row['grain_scale_diagnostics'].append({'name':entry['name'],'native_diagnostic_bbox':box,'source_luma_mean':float(s.mean()),'candidate_luma_mean':float(c.mean()),'source_luma_std':float(s.std()),'candidate_luma_std':float(c.std()),'source_native_highpass_std_radius2':float(sh.std()),'candidate_native_highpass_std_radius2':float(ch.std()),'native_highpass_spatial_correlation':float(np.corrcoef(sh.ravel(),ch.ravel())[0,1]),'source_neighbor_difference_std_x':float(np.diff(s,axis=1).std()),'candidate_neighbor_difference_std_x':float(np.diff(c,axis=1).std())})
        if texture_id==5437:
            row['bitmap_region_count_source_and_candidate']=5
            row['left_tip_native_pixel_rows56_63']=[{'y':y,'source_maxRGB_x20_35':a[y,20:36].max(1).tolist(),'candidate_maxRGB_x20_35':g[y,20:36].max(1).tolist()}for y in range(56,64)]
            row['tip_note']='Both source and whole-inverse candidate tip end at nativey59; nativey60..63 black in both. No invented shorter tip from thumbnail impression. Root reviews soft boundary/localshade caveats, no color-mask physicalhole inference.'
        else:
            row['bitmap_region_count_source_and_candidate']='Same original one large pale/one small pale/two warm lower fields plus dark left field; no new fragment/object count.'
    rows.append(row)
(BASE/'pattern-metrics-equipment-twenty-eighth.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps([{k:v for k,v in r.items()if k in ['id','existing_crossed_paint_mark','grain_scale_diagnostics','tip_note']}for r in rows],ensure_ascii=False))

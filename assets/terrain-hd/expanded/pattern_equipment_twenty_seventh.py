from pathlib import Path
import json
import numpy as np
from PIL import Image

BASE=Path(__file__).resolve().parent
def source(texture_id):
    image=Image.open(BASE/'original'/f'{texture_id}.png').convert('RGBA')
    assert image.getchannel('A').getextrema()==(255,255)
    return np.asarray(image.convert('RGB'))
def peak(a,box,expected):
    x0,y0,x1,y1=box;t=a[y0:y1,x0:x1];p=np.unravel_index(t.max(2).argmax(),t.shape[:2]);xy=[int(p[1]+x0),int(p[0]+y0)];assert xy==expected
    return {'native_diagnostic_domain':box,'source_peak_xy':xy,'source_peak_RGB':t[p].tolist()}
rows=[]
a=source(6403)
rows.append({'id':6403,'source_native_size':[64,32],'actual_source_alpha_extrema':[255,255],'one_isolated_small_warm_paint_peak':peak(a,[33,15,41,23],[37,21]),'interpretation':'Original broad muted brown panels, one faint diagonal shading stroke across left field, central vertical boundary and original small dark pointed/tablike fields below-right, not newly interpreted hardware. Preserve tiny warm paintmark37,21 at original strength; original left partial gray mark stays partial and dim. No global periodic pattern/count asserted.'})
a=source(7659)
rows.append({'id':7659,'source_native_size':[128,64],'actual_source_alpha_extrema':[255,255],'source_light_evidence':[peak(a,[20,2,40,15],[30,9]),peak(a,[20,22,35,35],[30,29])],'interpretation':'Original grayblue broad painted surrounding field and exactly two central brown triangular painted fields, same irregular meeting shape/source shading. One original small brown paintmark30,29; upper gray sourcehighlight30,9 stays narrow/faint, no new white raised seam. Large nearblack rounded right painted field retains original UV silhouette/shadowcolor, no materialhole/physicalhardware inference.'})
a=source(4803)
p=a[16:39,43:64].mean((0,2));maxima=[i+43 for i in range(1,len(p)-1)if p[i]>p[i-1]and p[i]>=p[i+1]];assert maxima==[46,50,52,56,58,62]
rows.append({'id':4803,'source_native_size':[64,64],'actual_source_alpha_extrema':[255,255],'source_curved_paint_band_evidence':[peak(a,[0,12,12,33],[7,30]),peak(a,[0,34,12,55],[5,52]),peak(a,[12,50,32,64],[26,60])],'right_irregular_paint_profile_domain':[43,16,64,39],'right_measured_native_profile_maxima_x':maxima,'interpretation':'Three identifiable curved painted-band portions: upper-left/middle-left/clipped lower-middle, with exact native measured light anchors. Right narrow olive/gray painted field keeps every irregular faint source strip/shadow. Six measured tonal profile maxima are not six invented physical ribs/straps or an ideal pattern. Original darkgray rounded fields/black shadowregions retained, no new lens/gloss/rivets/curls.'})
for r in rows:
    r['source_before_prompt_assertion']='Native source-only assertions genuinely executed before prompt/call. Fullcanvas opaqueRGB NEAREST guide. Fixed domains diagnostics only; never cropped references/BBox fitting/artistic RGB composites.'
    r['material']='Muted original matte gray/warm/olive finepaint, preserve unequal local highlights and weak source relationships. No added glinty rod/raised rim/bevel, new regular knurl/weave/photo grain/parts.'
(BASE/'pattern-constraints-equipment-twenty-seventh.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Beforeprompt sourceasserts passed:6403tiny37,21;7659two fields/30,9+30,29;4803three curvedpaint anchors and actual irregular right profile.')

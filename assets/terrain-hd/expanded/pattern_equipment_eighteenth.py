import json,numpy as np
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
s=np.asarray(Image.open(BASE/'original/609.png').convert('RGB'),float)
g=np.asarray(Image.open(BASE/'private-equipment-eighteenth/native4x/609-stored-rgb-private.png').convert('RGB').resize((128,64),Image.Resampling.LANCZOS),float)
rows=[]
for bbox in [[30,29,31,31],[38,34,40,36]]:
 x0,y0,x1,y1=bbox
 def peak(a):
  v=a[y0:y1,x0:x1].mean(2);y,x=np.unravel_index(v.argmax(),v.shape);return [int(x+x0),int(y+y0)]
 sp=peak(s);gp=peak(g);x,y=sp
 rows.append({'source_bbox':bbox,'source_peak_xy':sp,'candidate_peak_xy':gp,'exact_peak_phase':sp==gp,'source_peak_RGB':s[y,x].tolist(),'candidate_at_source_peak_RGB':g[y,x].tolist()})
records=[{'id':609,'source_pale_speck_count':2,'source_defined_bboxes':[[30,29,31,31],[38,34,40,36]],'records':rows,'exact_constrained_phase':all(r['exact_peak_phase']for r in rows),'method':'Original128x64 fullsource and entire calibrated native512x256 inverse to128x64; independent brightest native sample only within fixed source-defined speckbbox. Diagnostic ROI never used input/composite/registration. Existing topwear contour separately retained by private fullcanvas review; no count claim for every paintwear texel.'},{'id':2377,'repeat_phase_claim':False,'review':'No repeated tiny details or glyphs; broad native64square olive/charcoal fields visually retained, material/border-strength hold. Fullraw and native256square private.'},{'id':4267,'precall_annotation_error':'Original pre-call constraints and exact actual prompt said THREE motifs. That was incorrect source counting. Those actual artifacts remain untouched. Correct sourcecount FOUR fastening motifs, TWO central long painted straps+TWO side loop-like shapes, verified private same-scale original and whole-native inverse. Candidate keeps these four large motifs, but internal small dark spot/hole geometry uncertain, held.','correct_source_major_fastener_motif_count':4,'candidate_major_fastener_motif_count':4,'candidate_internal_hole_count_claim':None,'internal_geometry_review_pending':True,'diagnostic_only':True}]
(BASE/'pattern-metrics-equipment-eighteenth.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
print('609 source speck phase',records[0]['exact_constrained_phase'],'4267 actual2+2 countcorrection explicit')

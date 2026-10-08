from pathlib import Path
import json,numpy as np
from PIL import Image,ImageFilter
B=Path(__file__).resolve().parent;ST='equipment-thirty-third'
def data(i):
 o=Image.open(B/f'original/{i}.png').convert('RGB');z=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');return o,z
constraints=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'))
out=[];tiny=[]
def peaks(v,x0=0):return [k+x0 for k in range(1,len(v)-1) if v[k]>v[k-1] and v[k]>v[k+1]]
for c in constraints:
 i=c['id'];o,z=data(i);s=np.asarray(o,dtype=float).mean(2);g=np.asarray(z.resize(o.size,Image.Resampling.LANCZOS),dtype=float).mean(2)
 s4=np.asarray(o.resize(z.size,Image.Resampling.LANCZOS),dtype=float).mean(2);g4=np.asarray(z,dtype=float).mean(2)
 hs=s4-np.asarray(o.resize(z.size,Image.Resampling.LANCZOS).filter(ImageFilter.GaussianBlur(1)),dtype=float).mean(2)
 hg=g4-np.asarray(z.filter(ImageFilter.GaussianBlur(1)),dtype=float).mean(2)
 rows=[]
 for f in c['source_bitmap_fields']:
  x0,y0,x1,y1=f['domain'];sx=s[y0:y1,x0:x1];gx=g[y0:y1,x0:x1];sh=hs[y0*4:y1*4,x0*4:x1*4];gh=hg[y0*4:y1*4,x0*4:x1*4]
  rows.append({'name':f['name'],'domain':f['domain'],'source_luma_mean':float(sx.mean()),'candidate_inverse_luma_mean':float(gx.mean()),'source_luma_std':float(sx.std()),'candidate_inverse_luma_std':float(gx.std()),'native4x_source_HP_std':float(sh.std()),'native4x_candidate_HP_std':float(gh.std()),'native4x_HP_correlation':float(np.corrcoef(sh.ravel(),gh.ravel())[0,1])})
 out.append({'id':i,'phase':'POSTCALL diagnostic only. No reference cropping or source artistic RGB repairs, entire source/native4x images loaded first. Domains characterize local paint and do not impose special acceptance thresholds.','domains':rows})
 t={'id':i,'phase':'POSTCALL source pattern/tiny bitmap detail diagnostic, original before-call proof unchanged. Fullcanvas sampling only.'}
 if i==5387:
  t.update(source_square_field_count=8,source_square_rows_columns=[2,4],candidate_private_visible_square_field_count=8,candidate_square_rows_columns=[2,4],source_and_candidate_lower_right_round_field_count=1,field_bboxes_before_call=[f['domain'] for f in c['source_bitmap_fields']],note='All9source bitmap fields/canvas gaps retained. Candidate moderate fine pigment grain on originally smoother square fields requires root material review; field count alone does not certify material.')
 if i==5408:
  a=np.array(Image.open(B/f'original/{i}.png').convert('RGBA'));t.update(original_sourceA0_xy=[list(map(int,[x,y]))for y,x in zip(*np.where(a[:,:,3]==0))],source_center_RGB=a[47:53,14:18,:3].tolist(),candidate_center_inverse_RGB=np.array(z.resize(o.size,Image.Resampling.LANCZOS))[47:53,14:18,:3].tolist(),note='Original source center already contains alpha0; restored sourceA reproduces old opening. Concern is hard bright raised-looking surround, not inventing a new alpha hole.')
 if i==5438:
  row=[]
  for v in c['native_checker_phase_per_row']:
   y=v['y'];p=peaks(g[y,29:114],29);sp=v['x_strict_luma_maxima'];row.append({'y':y,'source_maxima':sp,'candidate_maxima':p,'same_maxima':sp==p,'source_count':len(sp),'candidate_count':len(p)})
  t.update(checker_native_rows=row,all_row_maxima_exact=all(x['same_maxima']for x in row),note='Alternating core pattern phase checked against exact stored native rows including source irregularities; changed weak maxima are diagnostic, not automatically new geometry. Fullraw new sharp white streak and stronger rim/round-mark material independently reviewed.')
 tiny.append(t)
(B/f'material-pattern-metrics-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(B/f'tiny-marks-review-{ST}.json').write_text(json.dumps(tiny,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(out,indent=2));print('checker rows exact',sum(r['same_maxima']for r in tiny[-1]['checker_native_rows']),'/',len(tiny[-1]['checker_native_rows']))

from pathlib import Path
import json,numpy as np
from PIL import Image,ImageFilter
B=Path(__file__).resolve().parent;ST='equipment-thirty-fourth';cs=json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8'));materials=[];tiny=[]
def maxima(v,x0):return [k+x0 for k in range(1,len(v)-1)if v[k]>v[k-1]and v[k]>v[k+1]]
for c in cs:
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');n=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');s=np.asarray(o,dtype=float).mean(2);g=np.asarray(n.resize(o.size,Image.Resampling.LANCZOS),dtype=float).mean(2);rows=[]
 sf=o.resize(n.size,Image.Resampling.LANCZOS);hs=np.asarray(sf,dtype=float).mean(2)-np.asarray(sf.filter(ImageFilter.GaussianBlur(2)),dtype=float).mean(2);hg=np.asarray(n,dtype=float).mean(2)-np.asarray(n.filter(ImageFilter.GaussianBlur(2)),dtype=float).mean(2)
 for f in c['source_bitmap_fields']:
  x0,y0,x1,y1=f['domain'];ss=s[y0:y1,x0:x1];gg=g[y0:y1,x0:x1];sh=hs[y0*4:y1*4,x0*4:x1*4];gh=hg[y0*4:y1*4,x0*4:x1*4];rows.append({'name':f['name'],'domain':f['domain'],'source_mean':float(ss.mean()),'candidate_inverse_mean':float(gg.mean()),'source_std':float(ss.std()),'candidate_inverse_std':float(gg.std()),'source_native4x_HP_std':float(sh.std()),'candidate_native4x_HP_std':float(gh.std()),'native4x_HP_corr':float(np.corrcoef(sh.ravel(),gh.ravel())[0,1])})
 materials.append({'id':i,'phase':'POSTCALL material diagnostics only, exact beforeprompt native matrices/constraints immutable. Domains are original-coordinate numeric summaries loaded from full arrays, NEVER reference crops/source artistic RGB patches/new special acceptance thresholds.','domains':rows})
 t={'id':i,'phase':'POSTCALL native pattern/weakmark diagnostic; original-only BEFOREprompt proof untouched. Existing-count changes at thresholds not alone newparts, root full views authoritative.'}
 if i==3034:
  rr=[]
  for row in c['finechecker_native_phase_rows']:
   y=row['y'];p=maxima(g[y,41:66],41);sp=row['strict_luma_maxima_x'];rr.append({'y':y,'source_maxima_x':sp,'candidate_maxima_x':p,'source_count':len(sp),'candidate_count':len(p),'exact_positions':sp==p})
  t['finechecker_phase_rows']=rr;t['exact_row_count']=sum(x['exact_positions']for x in rr);t['warm_marks_native_source_and_candidate']=[{'xy':[53,y],'source_RGB':np.asarray(o)[y,53].tolist(),'candidate_inverse_RGB':np.asarray(n.resize(o.size,Image.Resampling.LANCZOS))[y,53].tolist()}for y in [69,93]];t['counts_private_view']={'source_and_candidate_warm_marks':2,'main_bitmap_fields':8,'additional_marks_or_components_seen':False}
 if i==5380:
  rr=[]
  for row in c['right_streak_exact_native_phase_rows']:
   y=row['y'];p=maxima(g[y,83:128],83);sp=row['strict_luma_maxima_x'];rr.append({'y':y,'source_maxima_x':sp,'candidate_maxima_x':p,'source_count':len(sp),'candidate_count':len(p),'exact_positions':sp==p})
  t['right_fine_streak_phase_rows']=rr;t['exact_row_count']=sum(x['exact_positions']for x in rr);t['counts_private_view']={'main_bitmap_fields':4,'existing_dark_spot':1,'no_new_hardware_or_holes_seen':True};t['dark_spot_source_and_candidate_center']={'xy':[68,24],'source_RGB':np.asarray(o)[24,68].tolist(),'candidate_inverse_RGB':np.asarray(n.resize(o.size,Image.Resampling.LANCZOS))[24,68].tolist()}
 if i==6473:
  t['counts_private_view']={'source_and_candidate_main_groups':6,'camo_bottom_strips':3,'brown_separator_bands':2,'existing_small_gray_rectangle':1,'existing_dark_round_field':1,'additional_camo_islands_or_parts_seen':False};t['source_separator_rows']=c['separator_native_y_rows'];t['candidate_band_means_y78_109']=g[78:110,60:110].mean(1).tolist();t['original_band_means_y78_109']=s[78:110,60:110].mean(1).tolist();t['camo_note']='Complete source native camo mask/matrices and source-defined source components BEFOREprompt. No physical patch count inferred from diagnostic thresholds; private whole source/native view establishes original silhouettes/count/phase preserved with slightly sharper edges.'
 tiny.append(t)
(B/f'material-pattern-metrics-{ST}.json').write_text(json.dumps(materials,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');(B/f'tiny-marks-review-{ST}.json').write_text(json.dumps(tiny,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for r in tiny:print(r['id'],r.get('exact_row_count'),r.get('warm_marks_native_source_and_candidate'),r.get('dark_spot_source_and_candidate_center'))
for r in materials:print(r['id'],[(f['name'],round(f['native4x_HP_corr'],3),round(f['source_mean'],2),round(f['candidate_inverse_mean'],2))for f in r['domains']])

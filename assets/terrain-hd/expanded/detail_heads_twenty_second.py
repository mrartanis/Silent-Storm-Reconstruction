from pathlib import Path
import json,numpy as np
from PIL import Image
B=Path(__file__).resolve().parent;ST='heads-twenty-second';out=[]
for c in json.loads((B/f'pattern-constraints-{ST}.json').read_text(encoding='utf-8')):
 i=c['id'];o=Image.open(B/f'original/{i}.png').convert('RGB');g=Image.open(B/f'private-{ST}/native4x/{i}-stored-rgb-private.png').convert('RGB');prod=np.array(g.resize(o.size,Image.Resampling.LANCZOS));src=np.array(o);d=prod.astype(float)-src;delta=d.mean(2);y,x=np.unravel_index(delta.argmax(),delta.shape);ly,lx=np.unravel_index(delta.argmin(),delta.shape)
 domains=[]
 for r in c['source_domains']:
  x0,y0,x1,y1=r['bbox'];s=src[y0:y1,x0:x1];p=prod[y0:y1,x0:x1];domains.append({'name':r['name'],'bbox':r['bbox'],'source_mean_RGB':s.mean((0,1)).tolist(),'production_mean_RGB':p.mean((0,1)).tolist(),'source_max_RGB':s.max((0,1)).tolist(),'production_max_RGB':p.max((0,1)).tolist()})
 py,px=c['native_source_max_mean_RGB_xy'][1],c['native_source_max_mean_RGB_xy'][0]
 row={'id':i,'source_peak_xy':[px,py],'source_peak_RGB':src[py,px].tolist(),'production_at_source_peak_RGB':prod[py,px].tolist(),'max_local_mean_lift':float(delta[y,x]),'max_local_mean_lift_xy':[int(x),int(y)],'source_at_max_lift_RGB':src[y,x].tolist(),'production_at_max_lift_RGB':prod[y,x].tolist(),'max_local_mean_loss':float(delta[ly,lx]),'max_local_mean_loss_xy':[int(lx),int(ly)],'source_at_max_loss_RGB':src[ly,lx].tolist(),'production_at_max_loss_RGB':prod[ly,lx].tolist(),'domains':domains,'whole_source_mean_RGB':src.mean((0,1)).tolist(),'whole_production_mean_RGB':prod.mean((0,1)).tolist(),'inverse_RGB_correlation':float(np.corrcoef(src.reshape(-1),prod.reshape(-1))[0,1]),'entire_native_source_row_maximum_x':src.mean(2).argmax(1).tolist(),'entire_native_production_row_maximum_x':prod.mean(2).argmax(1).tolist(),'semantics':'Numeric complete storedRGB/localtone diagnostics only, not physicalpart count fromcomponent threshold. RGB extraction BEFORE wholecanvas native LANCZOS inverse. No pixelpatch/localgain/crop/registration, actual original alpha4x separately restored.'}
 out.append(row)
 for name,im in [('source',o.resize(g.size,Image.Resampling.LANCZOS)),('production',g)]:im.resize((512,512),Image.Resampling.NEAREST).save(B/f'private-{ST}/native4x/{i}-{name}-native4x-nearest512.png')
 print(i,'maxlift',round(row['max_local_mean_lift'],2),[int(x),int(y)],row['source_at_max_lift_RGB'],row['production_at_max_lift_RGB'],'maxloss',round(row['max_local_mean_loss'],2),[int(lx),int(ly)],'peak',row['source_peak_RGB'],row['production_at_source_peak_RGB']);print(json.dumps(domains))
(B/f'detail-metrics-{ST}.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

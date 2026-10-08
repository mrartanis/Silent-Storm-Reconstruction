import json,hashlib,numpy as np
from pathlib import Path
from PIL import Image
BASE=Path(__file__).resolve().parent
src=Image.open(BASE/'original/5376.png').convert('RGB');a=np.array(src,dtype=float).mean(2)
def peaks(p):return [j for j,v in enumerate(p)if 0<j<len(p)-1 and v>p[j-1]and v>=p[j+1]]
p=a[50:64,:35].mean(0);expected=list(range(1,32,2));assert peaks(p)==expected;assert len(expected)==16
assert all(a[50:64,x].max()==0 for x in range(0,33,2))
row={'id':5376,'source_native_profile_domain':[0,50,35,64],'source_native_vertical_stripe_maxima_x':expected,'source_stripe_count':16,'source_stripe_pitch':2,'source_black_separator_columns':list(range(0,33,2)),'source_black_separator_maxRGB':0,'source_original_png_sha256':hashlib.sha256((BASE/'original/5376.png').read_bytes()).hexdigest(),'source_pattern_assertions':'Executed and passed BEFORE actualprompt and builtin call. All16source native1pixel bands with black1pixel separators proven. Fixed domains solely readonly diagnostics, never references/cropping/registration/art inserts.'}
path=BASE/'private-equipment-twenty-third/native4x/5376-stored-rgb-private.png'
if path.exists():
 c=np.array(Image.open(path).convert('RGB').resize(src.size,Image.Resampling.LANCZOS),dtype=float).mean(2);row['candidate_native_vertical_stripe_maxima_x']=peaks(c[50:64,:35].mean(0));row['stripe_count_and_phase_equal']=row['candidate_native_vertical_stripe_maxima_x']==expected
(BASE/'pattern-metrics-equipment-twenty-third.json').write_text(json.dumps([row],ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Source BEFOREprompt assertionpassed:16bars phase1,3,...31/pitch2.')

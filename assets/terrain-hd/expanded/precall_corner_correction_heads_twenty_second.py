from pathlib import Path
import json,hashlib,shutil
B=Path(__file__).resolve().parent;ROOT=B.parents[2];ST='heads-twenty-second'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
draft=B/f'private-{ST}/precall-draft';draft.mkdir(parents=True,exist_ok=True)
patterns=B/f'pattern-constraints-{ST}.json';shutil.copyfile(patterns,draft/patterns.name)
changes=[]
for i in[6718,6721]:
 assert not list(B.rglob(f'{i}-*raw*.png'))and not list(B.rglob(f'{i}-*call*.json'))
 p=B/f'generated/{i}-prompt.txt';shutil.copyfile(p,draft/f'{i}-draft-prompt.txt');old=p.read_bytes();text=old.decode('utf-8');text=text.replace('ONE original whiteauthoringcorner','ONE original authored cornerpigment sample').replace('ONEexistingauthoringcorner','ONEexistingauthored cornerpigment sample').replace('ONEoriginalauthoringwhitecorner','ONEoriginalauthored cornerpigment sample');assert text.encode('utf-8')!=old;p.write_bytes(text.encode('utf-8'));changes.append({'id':i,'initial_prompt_SHA256':hashlib.sha256(old).hexdigest(),'corrected_exact_prompt_SHA256':sha(p),'draft_prompt':(draft/f'{i}-draft-prompt.txt').relative_to(ROOT).as_posix()})
rows=read(patterns)
for c in rows:
 c['source_description']=c['source_description'].replace('ONE original whiteauthoringcorner','ONE original authored cornerpigment sample');c['source_feature_counts']['source_authored_corner_pigment_sample']=c['source_feature_counts'].pop('source_white_authoring_corner');c['count_qualification']=c['count_qualification'].replace('+authoringcorner','+authored cornerpigment sample')
save(patterns,rows)
p=B/'prepare_heads_twenty_second.py';s=p.read_text(encoding='utf-8').replace('ONE original whiteauthoringcorner','ONE original authored cornerpigment sample').replace('source_white_authoring_corner','source_authored_corner_pigment_sample').replace('ONEexistingauthoringcorner','ONEexistingauthored cornerpigment sample').replace('ONEoriginalauthoringwhitecorner','ONEoriginalauthored cornerpigment sample');p.write_text(s,encoding='utf-8')
save(B/f'precall-corrections-{ST}.json',{'phase':'BEFORE any imagegen call, draftmislabel corrected preserving exactoldbytes separately. Final sourcecheck/native matrices/natural roundtrip/fullRGBA were always correct and unchanged.','correction':'6718/6721 cornerpigment is NOT white255; original exactRGBA6718(0,127)=[123,100,84,120],6721=[176,160,156,255]. Initial descriptive word white was copied from DIFFERENT source6783 (truewhite255); native data/corner values already actualcorrect. Source-specific word corrected before actualargument sent. No call/retry/RGBartwork/sourcepixelchange.','draft_pattern_ref':{'path':(draft/patterns.name).relative_to(ROOT).as_posix(),'file_SHA256':sha(draft/patterns.name)},'prompt_changes':changes,'final_before_constraints_ref':{'path':patterns.relative_to(ROOT).as_posix(),'file_SHA256':sha(patterns)},'source_before_byteunchanged':(B/f'source-check-{ST}.json').read_bytes()==(B/f'source-check-{ST}-before-call.json').read_bytes()})
print('BEFORE FIRSTcalls corrected cornerword only; actualRGBA/raw/native/sourceproof unchanged; saved exactdraft bytes')

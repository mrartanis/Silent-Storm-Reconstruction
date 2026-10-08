from pathlib import Path
B=Path(__file__).resolve().parent;ST='heads-twenty-third'
for name in ['precall_guard','preview','material_phase','detail','postcall_material']:
 src=B/f'{name}_clothing_forty_fourth.py' if name!='precall_guard' else B/'precall_guard_heads_twenty_second.py'
 p=B/f'{name}_heads_twenty_third.py';assert not p.exists();s=src.read_text(encoding='utf-8').replace('clothing-forty-fourth',ST).replace('heads-twenty-second',ST).replace('[6823, 7607, 7610]','[6774, 6775, 6776]').replace('[6823,7607,7610]','[6774,6775,6776]');p.write_text(s,encoding='utf-8')
src=(B/'save_heads_twenty_second_call.py').read_text(encoding='utf-8').replace('heads-twenty-second',ST)
src=src.replace("arg=prompt.read_bytes().decode('utf-8')","canonical=prompt.read_bytes();assert canonical.endswith(b'\\n')and not canonical.endswith(b'\\r\\n');arg=canonical[:-1].decode('utf-8');assert (B/f'generated/{i}-{ST}-exact-call-argument.txt').read_bytes()==arg.encode('utf-8')")
src=src.replace("'prompt_sha256':sha(prompt),","'prompt_sha256':sha(prompt),'prompt_argument_sha256':hashlib.sha256(arg.encode('utf-8')).hexdigest(),'canonical_prompt_equals_exact_actual_argument_plus_ONE_additional_LF':True,")
src=src.replace("record['prompt_serialization']='Exact actual UTF8 CRLFargument without finalLF; literal absolute actualtool refs recorded and separate portable reference recipe retained. No fullnative raster/DB matrices duplicated inside callmetadata; immutable common refs+SHA.'","record['prompt_serialization']='Canonical prompt-file UTF8 bytes == exact actual CRLF argument + ONE additional LF, per coordinator Heads23 explicit instruction. Literal actual argument (without serviceLF) and absolute actualtool references retained separately. No fullnative raster/DB matrices duplicated; immutable common refs+SHA.'")
p=B/'save_heads_twenty_third_call.py';assert not p.exists();p.write_text(src,encoding='utf-8')
print('Created own Heads23 canonicalprompt serviceLF tools, no calls yet')

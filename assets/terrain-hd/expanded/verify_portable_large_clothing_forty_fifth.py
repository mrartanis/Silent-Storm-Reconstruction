"""Verify frozen native matrix bytes; --restore restores only an absent original.

Run from the repository root. Existing originals are verified and never replaced.
"""
import gzip,hashlib,json,sys
from pathlib import Path

proof=json.loads(Path('assets/terrain-hd/expanded/portable-large-proof-clothing-forty-fifth.json').read_text(encoding='utf-8'))
packed=Path(proof['stored_path']).read_bytes()
assert len(packed)==proof['stored_bytes'] and hashlib.sha256(packed).hexdigest()==proof['stored_sha256']
raw=gzip.decompress(packed)
assert len(raw)==proof['original_bytes'] and hashlib.sha256(raw).hexdigest()==proof['original_sha256']
manifest=json.loads(Path('assets/terrain-hd/expanded/sha256-clothing-forty-fifth.json').read_text(encoding='utf-8'))
assert manifest[proof['original_path']]==proof['original_sha256']
original=Path(proof['original_path'])
if original.exists():
    assert original.read_bytes()==raw
elif '--restore' in sys.argv:
    with original.open('xb') as output:
        output.write(raw)
print('Frozen original bytes and SHA verified; original manifest remains unchanged.')

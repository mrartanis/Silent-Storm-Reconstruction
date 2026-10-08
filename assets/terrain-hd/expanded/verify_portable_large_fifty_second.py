"""Verify packet49 compressed evidence; --restore writes only missing originals."""
import gzip,hashlib,json,sys
from pathlib import Path
proof=json.loads(Path('assets/terrain-hd/expanded/portable-large-proof-fifty-second.json').read_text(encoding='utf-8'))
for record in proof['records']:
    packed=Path(record['stored_path']).read_bytes()
    assert len(packed)==record['stored_bytes'] and hashlib.sha256(packed).hexdigest()==record['stored_sha256']
    raw=gzip.decompress(packed)
    assert len(raw)==record['original_bytes'] and hashlib.sha256(raw).hexdigest()==record['original_sha256']
    source=Path(record['original_path'])
    if source.exists():
        assert source.read_bytes()==raw
    elif '--restore' in sys.argv:
        with source.open('xb') as output:
            output.write(raw)
print('All compressed frozen evidence verifies to exact original bytes.')

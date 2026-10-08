"""Verify portable metadata archives; --restore recreates only missing exact JSON bytes."""
from pathlib import Path
import json,hashlib,gzip,sys
B=Path(__file__).resolve().parent;ROOT=B.parents[2]
mapping=B/'publication-large-JSON-portability-clothing-complete-20261008.json'
records=json.loads(mapping.read_text(encoding='utf-8'))['mappings'];restore='--restore'in sys.argv;restored=[]
for r in records:
 archive=(ROOT/r['published_gzip_path']).resolve();original=(ROOT/r['original_json_path']).resolve()
 assert archive.is_relative_to(ROOT) and original.is_relative_to(ROOT) and original.suffix=='.json'
 data=archive.read_bytes();assert hashlib.sha256(data).hexdigest()==r['gzip_SHA256']
 decoded=gzip.decompress(data)
 assert len(decoded)==r['original_bytes'] and hashlib.sha256(decoded).hexdigest()==r['original_JSON_SHA256']
 if original.exists():assert original.read_bytes()==decoded,('existing proof differs; will not overwrite',original)
 elif restore:
  original.parent.mkdir(parents=True,exist_ok=True);original.write_bytes(decoded);restored.append(r['original_json_path'])
print(json.dumps({'portable_JSON_archives_verified':len(records),'restored_missing_only':restored,'PNG_modified':False,'existing_files_overwritten':False}))

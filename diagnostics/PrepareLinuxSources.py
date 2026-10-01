"""Resolve historical case-insensitive/backslash includes in a build-only tree.

Only include spellings change. CMake compiles this copy of the tracked sources;
the conversion is deterministic and keeps unchanged file timestamps for Ninja.
"""
import argparse
import re
from pathlib import Path

MODULES = ('Misc', 'MiscDll', 'FileIO', 'Image', 'ADOImport', 'DBFormat',
           'Input', 'Script', 'FModSound', 'Main', 'Game', 'Media', 'third_party',
           'diagnostics')


def prepare(source, output):
    files = [p for module in MODULES for p in (source / module).rglob('*')
             if p.is_file() and p.suffix.lower() in ('.h', '.cpp', '.c', '.inl')]
    names = {str(p.relative_to(source)).replace('\\', '/').lower(): p for p in files}
    count = 0
    for path in files:
        relative = path.relative_to(source)
        text = path.read_bytes().decode('utf-8', errors='surrogateescape')

        def include(match):
            spelling = match[2].replace('\\', '/')
            candidate = (path.parent / spelling).resolve()
            try:
                key = str(candidate.relative_to(source)).replace('\\', '/').lower()
            except ValueError:
                return match[0]
            target = names.get(key)
            if target:
                import os
                spelling = os.path.relpath(target, path.parent).replace('\\', '/')
            return match[1] + spelling + match[3]

        text = re.sub(r'(^\s*#\s*include\s*")([^"\n]+)(")', include, text,
                      flags=re.MULTILINE)
        destination = output / relative
        data = text.encode('utf-8', errors='surrogateescape')
        if not destination.exists() or destination.read_bytes() != data:
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            count += 1
    print(f'Prepared {len(files)} source/header files ({count} updated)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.source.resolve(), args.output.resolve())

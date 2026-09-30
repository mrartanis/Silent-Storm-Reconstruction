"""Check normal and delayed PE imports of the actual Windows game executable."""
from pathlib import Path
import re
import struct
import sys


def imports(path):
    data = path.read_bytes()
    def u16(offset): return struct.unpack_from('<H', data, offset)[0]
    def u32(offset): return struct.unpack_from('<I', data, offset)[0]
    pe = u32(60)
    if data[:2] != b'MZ' or data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not a PE executable')
    optional = pe + 24
    magic = u16(optional)
    if magic not in (0x10b, 0x20b):
        raise ValueError('Unknown PE optional header')
    directories = optional + (112 if magic == 0x20b else 96)
    image_base = struct.unpack_from('<Q' if magic == 0x20b else '<I', data,
                                  optional + (24 if magic == 0x20b else 28))[0]
    sections = pe + 24 + u16(pe + 20)

    def file_offset(rva):
        for i in range(u16(pe + 6)):
            section = sections + 40 * i
            size, address, raw_size, raw = struct.unpack_from('<IIII', data, section + 8)
            if address <= rva < address + max(size, raw_size):
                if rva - address >= raw_size:
                    raise ValueError('Import points outside file-backed section')
                return raw + rva - address
        raise ValueError('Import RVA outside sections')

    names = []
    for index, stride, name_offset in ((1, 20, 12), (13, 32, 4)):
        rva, size = struct.unpack_from('<II', data, directories + 8 * index)
        if not rva:
            continue
        for descriptor in range(file_offset(rva), file_offset(rva) + size, stride):
            if not any(data[descriptor:descriptor + stride]):
                break
            name_rva = u32(descriptor + name_offset)
            if index == 13 and not u32(descriptor) & 1:
                name_rva -= image_base
            start = file_offset(name_rva)
            names.append(data[start:data.index(0, start)].decode('ascii').lower())
    return names


if __name__ == '__main__':
    names = imports(Path(sys.argv[1]))
    banned = [name for name in names if re.fullmatch(r'd3d9\.dll|d3dx9(?:_\d+)?\.dll', name)]
    print('Windows game imports: ' + ', '.join(sorted(names)))
    if banned:
        raise SystemExit('Legacy graphics imports: ' + ', '.join(banned))
    print('No D3D9 or D3DX9 runtime imports')

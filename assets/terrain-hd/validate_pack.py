"""Validate every native HD resource, alias, mip chain, alpha and brightness.

Run after build_pack.py. This verifies the actual archives the game loads.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image, ImageStat

ROOT=Path(__file__).resolve().parent

def chunks(data):
    position=0
    while position<len(data):
        tag=data[position]
        count=4 if data[position+1]&1 else 1
        length=int.from_bytes(data[position+1:position+1+count],'little')>>1
        start=position+1+count
        assert start+length<=len(data)
        yield tag,data[start:start+length]
        position=start+length
    assert position==len(data)

def validate(pack):
    manifest=json.loads((pack/'manifest.json').read_text(encoding='utf-8'))
    assets={a['id']:a for a in manifest['textures']}
    assert len(assets)==len(manifest['textures']), 'Duplicate source IDs'
    entries={}
    for archive in manifest['archives']:
        path=pack/archive['file']
        assert path.stat().st_size==archive['bytes']
        with path.open('rb') as stream:
            digest=hashlib.sha256()
            while block:=stream.read(1024*1024):
                digest.update(block)
            assert digest.hexdigest()==archive['sha256']
            stream.seek(0)
            signature,index=struct.unpack('<II',stream.read(8))
            assert signature==0x96948a22 and 8<=index<archive['bytes']
            stream.seek(index)
            root=dict(chunks(stream.read()))[1]
        pairs=list(chunks(dict(chunks(root))[1]))
        assert len(pairs)==archive['keys']*2
        for n in range(0,len(pairs),2):
            assert pairs[n][0]==1 and pairs[n+1][0]==2
            id=struct.unpack('<i',pairs[n][1])[0]
            offset,length=struct.unpack('<II',pairs[n+1][1])
            assert 8<=offset and offset+length<=index
            assert id not in entries, ('Duplicate packed ID',id)
            entries[id]=(archive['file'],offset,length)
    assert set(entries)=={key for id in assets for key in (id,id|0x01000000)}
    report=[]
    for id,asset in assets.items():
        archive,offset,length=entries[id]
        alias_archive,alias_offset,alias_length=entries[id|0x01000000]
        assert length==alias_length
        with (pack/archive).open('rb') as stream:
            stream.seek(offset)
            data=stream.read(length)
        assert len(data)==length
        if entries[id]!=entries[id|0x01000000]:
            with (pack/alias_archive).open('rb') as stream:
                stream.seek(alias_offset)
                assert data==stream.read(alias_length)
        assert hashlib.sha256(data).hexdigest()==asset['mmp_sha256']
        signature,format,average,width,height,mips=struct.unpack_from('<6I',data)
        assert signature==0x504d4d and format==6
        assert [width,height]==asset['physical_size']
        assert [width,height]==[n*asset['density'] for n in asset['logical_size']]
        w,h=width,height;total=24;levels=0
        while True:
            total+=w*h*4;levels+=1
            if min(w,h)==1:break
            w//=2;h//=2
        assert len(data)==total and levels==mips==asset['mips']
        image=Image.frombytes('RGBA',(width,height),data[24:24+width*height*4],'raw','BGRA')
        png=Image.open(ROOT/asset['png']).convert('RGBA')
        assert hashlib.sha256((ROOT/asset['png']).read_bytes()).hexdigest()==asset['png_sha256']
        if asset.get('native_alpha_reference'):
            expected_alpha=Image.open(ROOT/asset['native_alpha_reference']).convert('RGBA').getchannel('A').resize(image.size,Image.Resampling.LANCZOS)
        else:expected_alpha=png.getchannel('A')
        assert image.getchannel('A').tobytes()==expected_alpha.tobytes(), ('Alpha changed',id)
        if asset.get('alpha_encoding')=='premultiplied':
            for channel in image.split()[:3]:
                assert all(color<=alpha for color,alpha in zip(channel.tobytes(),expected_alpha.tobytes()))
        means=ImageStat.Stat(image.convert('RGB')).mean
        luma=sum(w*m for w,m in zip((.2126,.7152,.0722),means))
        target=asset['brightness_calibration']['original_luma']
        assert abs(luma-target)<.1, ('Brightness mismatch',id,luma,target)
        report.append({'id':id,'archive':archive,'size':[width,height],'mips':mips,
                       'luma':luma,'source_luma':target,'alpha_valid':True})
    (pack/'validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    queue_path = ROOT / 'expanded/queue.json'
    if queue_path.exists():
        queue = json.loads(queue_path.read_text(encoding='utf-8'))
        for item in queue:
            if item['id'] in assets:
                item['status'] = 'validated'
                item['packed_png_sha256'] = assets[item['id']]['png_sha256']
        queue_path.write_text(json.dumps(queue,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'Validated {len(assets)} textures, {len(entries)} aliases, {len(manifest["archives"])} archives; all masks, mip chains and native brightness passed')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack',type=Path,default=ROOT.parents[1]/'res-hd')
    validate(parser.parse_args().pack)

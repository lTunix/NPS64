"""Check ELF/OELF load segments after create-fself (DolphinPS4 launch issue)."""
import struct, sys
from pathlib import Path
directory=Path(sys.argv[1]) if len(sys.argv)>1 else Path('/work/build')
images={}
for name in ('nps64.elf', 'nps64.oelf'):
    data = (directory / name).read_bytes()
    images[name]=data
    assert data[:4] == b'\x7fELF' and data[4:6] == b'\x02\x01', f'{name}: not ELF64 LE'
    assert struct.unpack_from('<H', data, 18)[0] == 62, 'not x86_64'
    offset = struct.unpack_from('<Q', data, 32)[0]
    entsize, count = struct.unpack_from('<HH', data, 54)
    segments = []
    for i in range(count):
        kind, flags, off, va, pa, filesz, memsz, align = struct.unpack_from('<IIQQQQQQ', data, offset + i*entsize)
        if kind != 1:
            continue
        assert filesz <= memsz, f'{name}: invalid segment size'
        assert align >= 0x4000 and (va-off) % 0x4000 == 0, f'{name}: PS4 segment alignment'
        assert not any(va < old_va+old_size and old_va < va+memsz for old_va,old_size in segments), f'{name}: duplicate/overlapping PT_LOAD'
        segments.append((va, memsz))
    assert segments, f'{name}: missing PT_LOAD'
    print(f'PASS: {name} x86_64, {len(segments)} load segments, 16 KiB aligned, no overlap')

# create-fself can omit alignment gaps when calculating writable p_memsz.
# A valid-looking OELF can consequently leave the last BSS page unmapped.
# Compare actual original section extents, not just the alignment of segments.
source=images['nps64.elf'];output=images['nps64.oelf']
offset=struct.unpack_from('<Q',output,32)[0]
stride,count=struct.unpack_from('<HH',output,54)
writable=[]
for i in range(count):
    kind,flags,off,va,pa,filesz,memsz,align=struct.unpack_from('<IIQQQQQQ',output,offset+i*stride)
    if kind==1 and flags&2:writable.append((va,va+memsz))
offset=struct.unpack_from('<Q',source,40)[0]
stride,count,names_index=struct.unpack_from('<HHH',source,58)
names_header=struct.unpack_from('<IIQQQQIIQQ',source,offset+names_index*stride)
names=source[names_header[4]:names_header[4]+names_header[5]]
checked=0
for i in range(count):
    name,kind,flags,va,off,size,link,info,align,entsize=struct.unpack_from('<IIQQQQIIQQ',source,offset+i*stride)
    if kind!=8 or not size or not flags&2 or flags&0x400:continue # allocated NOBITS, excluding TLS
    label=names[name:].split(b'\0',1)[0].decode()
    assert any(start<=va and va+size<=end for start,end in writable), (
        f'{label}: OELF writable segments do not cover original BSS extent '
        f'{va:#x}..{va+size:#x}; writable={writable}')
    checked+=1
assert checked, 'No allocated BSS section checked'
print(f'PASS: OELF writable segments cover all {checked} original BSS section extents')

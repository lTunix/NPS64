"""Check ELF/OELF load segments after create-fself (DolphinPS4 launch issue)."""
import struct
from pathlib import Path
for name in ('nps64.elf', 'nps64.oelf'):
    data = (Path('/work/build') / name).read_bytes()
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

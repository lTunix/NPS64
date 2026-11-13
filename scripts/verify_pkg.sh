#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk="${OO_PS4_TOOLCHAIN:-/work/toolchain/OpenOrbis/PS4Toolchain}"
pkg=out/IV0000-NPSX00064_00-NPS64PS400000000.pkg
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
"$sdk/bin/linux/PkgTool.Core" pkg_validate --verbose "$pkg" > build/pkg-validation.log
"$sdk/bin/linux/PkgTool.Core" pkg_listentries "$pkg" > build/pkg-entries.log
mkdir -p build/pkg-verify
"$sdk/bin/linux/PkgTool.Core" pkg_extract "$pkg" build/pkg-verify > build/pkg-extraction.log
python3 - <<'PY'
from pathlib import Path
import sys, struct
from PIL import Image
sys.path.insert(0, '/work/scripts')
from check_stage import REQUIRED, check
root=Path('/work')
pkg=root/'out/IV0000-NPSX00064_00-NPS64PS400000000.pkg'
with pkg.open('rb') as f:
    assert f.read(4)==b'\x7fCNT', 'Invalid PKG magic'
assert pkg.stat().st_size % 0x8000==0
log=(root/'build/pkg-validation.log').read_text()
checks=[line for line in log.splitlines() if line.startswith('[')]
assert len(checks)>20 and all(line.startswith('[OK]') for line in checks), log
check(root/'build/pkg-verify/uroot')
for name in REQUIRED:
    assert (root/'build/pkg-verify/uroot'/name).read_bytes()==(root/'build/stage'/name).read_bytes(), name
entries=(root/'build/pkg-entries.log').read_text()
assert 'PARAM_SFO' in entries and 'ICON0_PNG' in entries, entries
def entry(name):
    row=next(line.split() for line in entries.splitlines() if line.endswith(' '+name))
    offset,size=int(row[0],16),int(row[1],16)
    assert offset+size<=pkg.stat().st_size, name
    with pkg.open('rb') as stream:
        stream.seek(offset)
        return stream.read(size)
assert entry('ICON0_PNG')==(root/'build/stage/sce_sys/icon0.png').read_bytes()
with Image.open(root/'build/stage/sce_sys/icon0.png') as icon:
    assert icon.size==(512,512) and icon.mode=='RGB', 'PS4 icon format'
def sfo_values(data):
    magic,version,keys,values,count=struct.unpack_from('<IIIII',data)
    assert magic==0x46535000
    result={}
    for index in range(count):
        key,kind,length,capacity,value=struct.unpack_from('<HHIII',data,20+index*16)
        name=data[keys+key:].split(b'\0')[0]
        result[name]=data[values+value:values+value+length]
    return result
staged=sfo_values((root/'build/stage/sce_sys/param.sfo').read_bytes())
packaged=sfo_values(entry('PARAM_SFO'))
# PkgTool adds PUBTOOLINFO/PUBTOOLVER; application fields must stay unchanged.
assert all(packaged.get(key)==value for key,value in staged.items()), 'Packaged SFO differs'
print('PASS: packaged icon matches stage; RGB 512x512; SFO application fields match')
print(f'PASS: PKG {pkg.stat().st_size} bytes, {len(checks)} checks OK, extracted payload matches stage')
PY

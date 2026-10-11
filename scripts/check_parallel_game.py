"""Verify independent package identity, payload and preservation of CPU51."""
from pathlib import Path
import hashlib
import struct

root=Path('/work')
base=root/'build/parallel-game'
elf=(base/'nps64.elf').read_bytes()
assert b'NPS64 00.63 ParaLLEl-RDP original' in elf
assert b'EmuThread: M64CMD_EXECUTE' in elf
assert b'predicated tile binning=on; original raster/color/textures' in elf
assert b'split pipeline: specialized rasterization then depth/color blend; ubershader=0' in elf
assert b'/data/NPS64-Parallel/nps64.log' in elf
assert b'/data/NPS64/games' in elf
assert b'GPU constructors deferred' in elf
sfo=(base/'stage/sce_sys/param.sfo').read_bytes()
assert sfo[:4]==b'\x00PSF'
_,_,keys,values,count=struct.unpack_from('<4sIIII',sfo)
entries={}
for i in range(count):
    key,fmt,length,maximum,offset=struct.unpack_from('<HHIII',sfo,20+i*16)
    name=sfo[keys+key:].split(b'\0',1)[0].decode()
    entries[name]=sfo[values+offset:values+offset+length].rstrip(b'\0')
assert entries['TITLE_ID']==b'NPSP00063'
assert entries['CONTENT_ID']==b'IV0000-NPSP00063_00-NPS64PARALLEL000'
assert entries['APP_VER']==b'00.63'
pkg=root/'out/NPS64-00.63-parallel-rdp-test.pkg'
assert pkg.read_bytes()==(root/'out/IV0000-NPSP00063_00-NPS64PARALLEL000.pkg').read_bytes()
stable=root/'out/NPS64-00.51-cpu.pkg'
if stable.exists():
    assert hashlib.sha256(stable.read_bytes()).hexdigest()=='1f89d7080b684c16e0e77373ac1b29b24aae3e12cee63b401bc1ddaee694ae16'
    print('PASS CPU51 package unchanged')
(root/'out/NPS64-00.63-parallel-rdp-test-SHA256.txt').write_text(
    hashlib.sha256(pkg.read_bytes()).hexdigest()+'  '+pkg.name+'\n')
print('PASS experimental original RDP game build: ELF, separate title/content ID, SFO 00.63 and package hash')

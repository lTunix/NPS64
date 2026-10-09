"""Verify independent package identity, payload and preservation of CPU51."""
from pathlib import Path
import hashlib
import struct

root=Path('/work')
base=root/'build/parallel-tile-probe'
elf=(base/'nps64.elf').read_bytes()
assert b'NPS64 00.56 ORIGINAL TILE CAPTURE' in elf
assert b'EmuThread: M64CMD_EXECUTE' not in elf
assert b'NPS64 00.51 CPU ONLY' not in elf
assert b'/data/NPS64-Parallel/tile-probe.log' in elf
assert b'WORKER started; configured stack=8 MiB' in elf
assert b'COMPILE pipeline case=%u local=%u begin' in elf
assert b'compiler ACO begin' in elf
assert b'compiler NIR postprocess begin' in elf
sfo=(base/'stage/sce_sys/param.sfo').read_bytes()
assert sfo[:4]==b'\x00PSF'
_,_,keys,values,count=struct.unpack_from('<4sIIII',sfo)
entries={}
for i in range(count):
    key,fmt,length,maximum,offset=struct.unpack_from('<HHIII',sfo,20+i*16)
    name=sfo[keys+key:].split(b'\0',1)[0].decode()
    entries[name]=sfo[values+offset:values+offset+length].rstrip(b'\0')
assert entries['TITLE_ID']==b'NPSP00056'
assert entries['CONTENT_ID']==b'IV0000-NPSP00056_00-NPS64PARALLEL000'
assert entries['APP_VER']==b'00.56'
pkg=root/'out/NPS64-00.56-parallel-tile-probe.pkg'
assert pkg.read_bytes()==(root/'out/IV0000-NPSP00056_00-NPS64PARALLEL000.pkg').read_bytes()
stable=root/'out/NPS64-00.51-cpu.pkg'
if stable.exists():
    assert hashlib.sha256(stable.read_bytes()).hexdigest()=='1f89d7080b684c16e0e77373ac1b29b24aae3e12cee63b401bc1ddaee694ae16'
    print('PASS CPU51 package unchanged')
(root/'out/NPS64-00.56-parallel-tile-probe-SHA256.txt').write_text(
    hashlib.sha256(pkg.read_bytes()).hexdigest()+'  '+pkg.name+'\n')
previous=root/'out/NPS64-00.53-parallel-rdp-test.pkg'
if previous.exists():
    assert hashlib.sha256(previous.read_bytes()).hexdigest()=='43046fcf5d343b665cd80ec5aef52f455cd80ee99fac5479865ad635db9affb8'
    print('PASS 00.53 game package unchanged')
prior=root/'out/NPS64-00.54-parallel-tile-probe.pkg'
if prior.exists():
    assert hashlib.sha256(prior.read_bytes()).hexdigest()=='0a387843c8ef062020d9bae0e0c826f8f7f36d5f14b48415d98c0d0a67a8a2be'
    print('PASS 00.54 package unchanged')
print('PASS isolated original tile capture: ELF, explicit worker stack, compile markers, SFO 00.56 and package hash')

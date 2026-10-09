"""Ensure GPU constructors cannot run before the frontend, and use dlmalloc."""
import struct, sys
from pathlib import Path
p=Path('/work/build/nps64.elf');b=p.read_bytes()
shoff=struct.unpack_from('<Q',b,40)[0];ents,n,strings=struct.unpack_from('<HHH',b,58)
sections=[struct.unpack_from('<IIQQQQIIQQ',b,shoff+i*ents) for i in range(n)]
ss=sections[strings];names=b[ss[4]:ss[4]+ss[5]]
def name(s):return names[s[0]:].split(b'\0',1)[0].decode()
byname={name(s):s for s in sections};boot=byname['.init_array'];gpu=byname['.nps64_gpu_init']
minimum = 8 if '--hle-probe' in sys.argv else 24
assert boot[5]==8 and gpu[5]>=minimum and gpu[5]%8==0
symbols={}
s=byname['.symtab'];st=sections[s[6]];strings=b[st[4]:st[4]+st[5]]
for i in range(s[4],s[4]+s[5],s[9]):
 index,info,other,section,value,size=struct.unpack_from('<IBBHQQ',b,i)
 symbols[strings[index:].split(b'\0',1)[0].decode()]=value
assert symbols['__nps64_gpu_init_start']==gpu[3] and symbols['__nps64_gpu_init_end']==gpu[3]+gpu[5]
assert 'mspace_malloc' in symbols and 'sceLibcMspaceCreate' not in symbols and 'sceLibcMspaceMalloc' not in symbols
relocations={}
r=byname['.rela.dyn']
for i in range(r[4],r[4]+r[5],r[9]):
 offset,info,value=struct.unpack_from('<QQq',b,i)
 if info&0xffffffff==8:relocations[offset]=value
assert relocations[boot[3]]==symbols['early_boot']
constructors=[relocations[gpu[3]+i] for i in range(0,gpu[5],8)]
assert all(x!=symbols['early_boot'] for x in constructors)
d=byname['.dynamic'];tags={}
for i in range(d[4],d[4]+d[5],16):
 tag,val=struct.unpack_from('<qQ',b,i);tags[tag]=val
assert tags[25]==boot[3] and tags[27]==8
print('PASS: only allocation-free boot trace runs before main; GPU constructors deferred; Sony mspace replaced by dlmalloc')

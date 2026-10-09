"""Make a red RDP fill, then poison GPU post-render colour/coverage green.
CPU replay must keep its own rendered red image on the following VI."""
import struct,subprocess
from pathlib import Path
root=Path('/work/build/parallel-dump-check');root.mkdir(parents=True,exist_ok=True)
def words(*v):return struct.pack('<'+'I'*len(v),*v)
data=bytearray(b'RDPDUMP2'+words(8<<20,4<<20))
def command(a,b):data.extend(words(2,a>>24&63,2,a,b))
command(0x3f000000|(3<<19)|319,0x100000)
command(0x2d000000,(320*4<<12)|(240*4))
command(0x2f000000|(3<<20),0)
command(0x37000000,0xff0000ff)
command(0x36000000|(319*4<<12)|(239*4),0)
command(0x29000000,0);data.extend(words(5))
vi=[0x00003003,0x100000,320,0,0,0,525,3093,0,0x006c02ec,0x002501ff,0,0x200,0x400]
for i,v in enumerate(vi):data.extend(words(3,i,v))
# Post-render updates are deliberate poison, not the expected CPU result.
data.extend(words(1,0x100000,4096)+words(*([0x00ff00ff]*1024))+words(7))
data.extend(words(8,0x80000,4096)+bytes(4096)+words(9,4))
for i,v in enumerate(vi):data.extend(words(3,i,v))
data.extend(words(4,6));(root/'poison.rdp').write_bytes(data)
import os
subprocess.run([str(root/'replay-cpu'),str(root/'poison.rdp'),str(root),'0,1'],check=True,env=dict(os.environ,NPS64_DUMP_CHECK_RGB='1048576:ff0000ff'))
for frame in (0,1):
 data=(root/f'frame-{frame}.ppm').read_bytes();pixels=data.split(b'\n',3)[3]
 assert sum(pixels[0::3])>100000 and sum(pixels[1::3])==0 and sum(pixels[2::3])==0
print('PASS CPU reference retains its own red framebuffer despite poisoned GPU colour and hidden coverage')

"""Exercise the actual decoder using synthetic, non-executable command bytes."""
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from analyze_ps4_flight import decode,HEADER

def record(words):
    return HEADER.pack(0x43524249,0,110,0x4000000000,len(words),0)+struct.pack('<%dI'%len(words),*words)

data=record([0x80000000,0xffff1000,0xc0031500,4,5,6,1,0xc0017600,0x20c,0x12345678])
records=decode(data)
assert records[0]['sequence']==110
assert records[0]['packets'][0]['groups']==[4,5,6]
assert records[0]['packets'][1]['register_start']=='0xb830'
assert len(decode(data+data))==2
malformed=[data[:-1],data[:10],b'bad!'+data[4:],record([0xc0031500,1]),
           record([0]),record([0xc0001500,1]),HEADER.pack(0x43524249,0,1,0,0xffffffff,0)]
for value in malformed:
    try:decode(value)
    except ValueError:pass
    else:raise AssertionError('Malformed capture accepted')
print('PASS offline PM4 decoder: direct dispatch, SH register offsets, multiple records and truncated/malformed input')

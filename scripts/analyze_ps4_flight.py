#!/usr/bin/env python3
"""Decode CPU-recorded GFX7 submissions without executing GPU commands.

Input is the IBRC format written by DolphinPS4's GNM flight recorder.
Reports recorded commands, not proof that any shader actually completed.
"""
import argparse
import json
from pathlib import Path
import struct

HEADER=struct.Struct('<IIQQII')
OPCODES={0x15:'DISPATCH_DIRECT',0x16:'DISPATCH_INDIRECT',0x37:'WRITE_DATA',
         0x3c:'WAIT_REG_MEM',0x3f:'INDIRECT_BUFFER',0x42:'PFP_SYNC_ME',
         0x43:'SURFACE_SYNC',0x46:'EVENT_WRITE',0x47:'EVENT_WRITE_EOP',
         0x58:'ACQUIRE_MEM',0x76:'SET_SH_REG'}

def decode(data):
    offset=0
    records=[]
    while offset<len(data):
        if len(data)-offset<HEADER.size:raise ValueError('Truncated IBRC header')
        magic,depth,sequence,va,count,top=HEADER.unpack_from(data,offset)
        offset+=HEADER.size
        if magic!=0x43524249:raise ValueError('Invalid IBRC magic')
        if count>1024*1024:raise ValueError('IB exceeds 4 MiB capture limit')
        if count>(len(data)-offset)//4:raise ValueError('Truncated IB data')
        words=struct.unpack_from('<%dI'%count,data,offset)
        offset+=count*4
        packets=[]
        index=0
        while index<count:
            header=words[index]
            if header==0xffff1000: # PKT3_NOP_PAD, the header-only NOP.
                index+=1
                continue
            kind=header>>30
            if kind==2:
                index+=1
                continue
            if kind!=3:raise ValueError('Unexpected packet type at dword '+str(index))
            body_count=((header>>16)&0x3fff)+1
            if body_count>count-index-1:raise ValueError('Truncated PM4 packet')
            opcode=(header>>8)&255
            body=words[index+1:index+1+body_count]
            packet={'dword':index,'opcode':hex(opcode),'name':OPCODES.get(opcode,'UNKNOWN'),
                    'payload':[hex(x) for x in body]}
            if opcode==0x15:
                if len(body)<4:raise ValueError('Short direct dispatch')
                packet['groups']=list(body[:3])
            elif opcode==0x76:
                if len(body)<2:raise ValueError('Short SH register packet')
                # SET_SH_REG's offset is measured in dwords from 0xB000.
                packet['register_start']=hex(0xb000+(body[0]&0xffff)*4)
            elif opcode==0x37 and len(body)>=4:
                packet['destination']=hex(body[1]|(body[2]<<32))
            packets.append(packet)
            index+=1+body_count
        records.append({'sequence':sequence,'depth':depth,'va':hex(va),'top_index':top,'packets':packets})
    return records

def report(directory):
    output=[]
    for path in sorted(directory.glob('ib-*.bin')):
        if path.stat().st_size>128*1024*1024:raise ValueError('Capture too large: '+str(path))
        for record in decode(path.read_bytes()):
            record['file']=path.name
            output.append(record)
    if not output:raise ValueError('No IBRC captures found')
    # Records may contain nested/chained IBs; preserve order within each file.
    # Do not count a nested record as another GPU submission.
    output.sort(key=lambda r:r['sequence'])
    return {'warning':'Recorded CPU command streams; completion must be checked against progress.log.',
            'sequences':sorted({r['sequence'] for r in output}),
            'dispatches':sum(p['name'].startswith('DISPATCH_') for r in output for p in r['packets']),
            'records':output}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    try:result=report(args.directory)
    except (ValueError,OSError) as error:parser.exit(1,str(error)+'\n')
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('Decoded sequences:',result['sequences'],'dispatches:',result['dispatches'])

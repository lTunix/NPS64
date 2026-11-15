#!/usr/bin/env python3
"""Compile the GPL Mupen64Plus DB into bounded, exact CRC/country matches."""
import argparse,hashlib,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
roms=[];rom=None;cheat=None;skipped=0;entries=0
for raw in a.source.read_text(encoding='utf-8-sig').splitlines():
 line=raw.strip()
 if line.startswith('crc '):
  match=re.fullmatch(r'crc ([0-9A-Fa-f]{8})-([0-9A-Fa-f]{8})-C:([0-9A-Fa-f]{2})',line)
  rom={'key':'-'.join(match.groups()).upper(),'name':'','cheats':[]} if match else None
  if rom:roms.append(rom)
  cheat=None
 elif line.startswith('gn ') and rom:rom['name']=line[3:]
 elif line.startswith('cn ') and rom:
  cheat={'name':line[3:],'lines':[]};rom['cheats'].append(cheat)
 elif cheat and line and not line.startswith(('//','cd ')):cheat['lines'].append(line)
def clean(s):return ''.join(c if 32<=ord(c)<127 and c!='|' else ' ' for c in s).strip()
def ident(s):return hashlib.sha256(s.encode()).hexdigest()[:16]
lines=['NPS64CHEATS1']
for rom in roms:
 lines.append('R|'+rom['key']+'|'+clean(rom['name']))
 for cheat in rom['cheats']:
  parsed=[];choices=[];valid=bool(cheat['lines']) and len(cheat['lines'])<=128
  for row in cheat['lines']:
   m=re.fullmatch(r'([0-9a-fA-F]{8})\s+([0-9a-fA-F?]{4})(?:\s+(.*))?',row)
   if not m:valid=False;break
   address,value,options=m.groups();options=options or ''
   opts=re.findall(r'([0-9a-fA-F]{4}):"([^"]*)"',options)
   if '?' in value and (not opts or len(opts)>512):valid=False;break
   if '?' not in value and options:valid=False;break
   if '?' in value:choices.append(opts)
   parsed.append((address,value,opts))
  if choices and len({len(x) for x in choices})!=1:valid=False
  if not valid:skipped+=1;continue
  variants=range(len(choices[0])) if choices else range(1)
  group=ident(rom['key']+'|'+cheat['name'])
  for v in variants:
   pairs=[]
   for address,value,opts in parsed:
    if opts:
     selected=opts[v][0]
     value=''.join(selected[i] if c=='?' else c for i,c in enumerate(value))
    pairs.append(address.upper()+' '+value.upper())
   code=' '.join(pairs);name=cheat['name'].replace('\\',' / ')
   if choices:name+=': '+choices[0][v][1]
   name=clean(name)[:190];key=ident(group+'|'+code)
   lines.append('C|'+key+'|'+group+'|'+name+'|'+code);entries+=1
out='\n'.join(lines)+'\n';a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(out)
print(json.dumps({'games':len(roms),'entries':entries,'skipped_incompatible_cheats':skipped,'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'database_bytes':len(out)},indent=2))

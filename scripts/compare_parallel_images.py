"""Native VI image comparisons, with explicit alignment status. Standard library only."""
import argparse,json,re,struct,zlib,html
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--same-stream',action='store_true');a=p.parse_args();root=Path(a.directory)
def ppm(path):
 data=path.read_bytes();m=re.match(rb'P6\n(\d+) (\d+)\n255\n',data);assert m,path
 w,h=map(int,m.groups());pixels=data[m.end():];assert len(pixels)==w*h*3
 return w,h,pixels
def png(path,w,h,rgb):
 def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
 data=b''.join(b'\0'+rgb[y*w*3:(y+1)*w*3] for y in range(h))
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(data))+chunk(b'IEND',b''))
logs=[(root/k/'run.log').read_text() for k in ('cpu','gpu')]
if a.same_stream:
 assert (root/'commands.rdp').exists() and 'captured before post-render DRAM' in logs[0]
 states=[]
 for log,prefix in zip(logs,('REPLAY','CAPTURE')):
  records={}
  for line in log.splitlines():
   if not line.startswith(prefix+' '):continue
   n=re.search(r'frame=(\d+)',line);vi=re.search(r'VI=([0-9a-f]+) origin=([0-9a-f]+)',line)
   if n and vi:records[int(n[1])]=vi.groups()
  states.append(records)
else:
 cpu_log=(root/'cpu/core.log').read_text();gpu_log=(root/'gpu/core.log').read_text()
 assert 'renderer=Angrylion' in cpu_log and 'renderer=paraLLEl-RDP active' in gpu_log
 assert 'replacement binning/color=off' in gpu_log
 states=[{int(re.search(r'frame=(\d+)',line)[1]):line for line in log.splitlines() if line.startswith('CAPTURE ')} for log in logs]
report=[];sections=[]
for path in sorted((root/'gpu').glob('frame-*.ppm'),key=lambda p:int(p.stem.split('-')[1])):
 frame=int(path.stem.split('-')[1]);other=root/'cpu'/path.name
 if not other.exists():
  report.append(dict(frame=frame,comparable=False,reason='CPU VI produced no valid image'));continue
 cw,ch,c=ppm(other);gw,gh,g=ppm(path)
 item=dict(frame=frame,cpu_size=[cw,ch],gpu_size=[gw,gh],same_rdp_stream=a.same_stream,matched_vi_state=states[0].get(frame)==states[1].get(frame),same_dimensions=cw==gw and ch==gh,comparable=a.same_stream and cw==gw and ch==gh and states[0].get(frame)==states[1].get(frame))
 w,h=max(cw,gw),max(ch,gh);cpu=bytearray(w*h*3);gpu=bytearray(w*h*3)
 for source,dest,sw,sh in ((c,cpu,cw,ch),(g,gpu,gw,gh)):
  for y in range(sh):dest[y*w*3:y*w*3+sw*3]=source[y*sw*3:(y+1)*sw*3]
 delta=bytearray(len(cpu));changed=major=total=maximum=0
 for i in range(0,len(cpu),3):
  d=[abs(cpu[i+k]-gpu[i+k]) for k in range(3)];changed+=any(d);major+=any(x>8 for x in d);total+=sum(d);maximum=max(maximum,*d)
  delta[i:i+3]=bytes(min(x*8,255) for x in d)
 if item['same_dimensions']:item.update(changed_pct=100*changed/(w*h),over8_pct=100*major/(w*h),mean_absolute_error=total/(w*h*3),max_error=maximum)
 for label,data in (('cpu',cpu),('gpu',gpu),('diff',delta)):png(root/(path.stem+'-'+label+'.png'),w,h,data)
 panel=bytearray(w*h*9)
 for y in range(h):
  for i,data in enumerate((cpu,gpu,delta)):panel[y*w*9+i*w*3:y*w*9+(i+1)*w*3]=data[y*w*3:(y+1)*w*3]
 png(root/(path.stem+'-comparison.png'),w*3,h,panel)
 sections.append('<h2>VI '+str(frame)+'</h2><p>'+html.escape(json.dumps(item))+'</p><div class="row">'+''.join('<figure><figcaption>'+label+'</figcaption><img src="'+path.stem+'-'+name+'.png"></figure>' for label,name in (('Angrylion CPU','cpu'),('ParaLLEl-RDP','gpu'),('Diferencia x8','diff')))+'</div>')
 report.append(item);print(json.dumps(item))
assert report
(root/'report.json').write_text(json.dumps(dict(host_only=True,same_rdp_stream=a.same_stream,frames=report),indent=2)+'\n')
(root/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>ParaLLEl / Angrylion</title><style>body{background:#16191d;color:#eee;font:16px sans-serif}.row{display:flex;flex-wrap:wrap}figure{margin:8px}img{max-width:40vw;image-rendering:auto}p{max-width:1200px}</style><h1>Comparacion de VI nativa</h1><p>Prueba local; no representa FPS ni fidelidad confirmada en PS4. '+('Comandos RDP comunes; CPU capturada antes de importar DRAM posterior de GPU.' if a.same_stream else 'Ejecuciones distintas: verificar fase/estado antes de atribuir diferencias al rasterizador.')+'</p>'+''.join(sections),encoding='utf-8')
print('Images and report generated; differences are measured, not automatically accepted')

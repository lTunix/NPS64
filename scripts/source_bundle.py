"""Bundle current project sources, reusing immutable dependencies when available."""
import json,tarfile,os
from pathlib import Path
root=Path('/work');base=root/'out/NPS64-00.07-source.tar.gz';output=root/'out/NPS64-00.13-source.tar.gz'
if os.environ.get('NPS64_SOURCE_VERSION'):
 output=root/('out/NPS64-'+os.environ['NPS64_SOURCE_VERSION']+'-source.tar.gz')
overlays=['vendor/gpu/mesa-26.0.8/src/amd/common/ac_ps4_drm.c',
          'vendor/gpu/mesa-26.0.8/src/amd/vulkan/radv_cmd_buffer.c',
          'vendor/gpu/mesa-26.0.8/src/amd/common/ac_gpu_info.c']
current=json.loads((root/'sources.lock.json').read_text());reuse=False
if base.is_file():
 with tarfile.open(str(base),'r|gz') as t:
  for member in t:
   if member.name=='sources.lock.json':
    reuse=json.load(t.extractfile(member))==current;break
def keep(info):
 parts=Path(info.name).parts
 if '.git' in parts or '__pycache__' in parts or Path(info.name).suffix in {'.a','.o','.d','.pyc'}:return None
 return info
with tarfile.open(str(output),'w:gz',compresslevel=6) as out:
 if reuse:
  with tarfile.open(str(base),'r|gz') as old:
   for info in old:
    if info.name.startswith(('vendor/','reference/')) and info.name not in overlays:
     out.addfile(info,old.extractfile(info) if info.isfile() else None)
  for name in overlays:
   out.add(root/name,arcname=name,filter=keep)
 else:
  for name in ['vendor/parallel-n64','vendor/OpenOrbis/src/modules','vendor/OpenOrbis/.nps64-revision','vendor/gpu','reference/DolphinPS4']:
   out.add(root/name,arcname=name,filter=keep)
 for name in ['README.md','TODO.md','LICENSE','Dockerfile','Dockerfile.gpu','.dockerignore','.gitignore','.gitattributes','sources.lock.json','scripts','src','docs','tests','assets']:
  out.add(root/name,arcname=name,filter=keep)
print('Fuentes correspondientes: '+str(output.relative_to(root)))

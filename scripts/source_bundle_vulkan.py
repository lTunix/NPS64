#!/usr/bin/env python3
"""Bundle the actual Vulkan build inputs, excluding binaries and user data."""
import argparse, json, subprocess, tarfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-work',type=Path,required=True)
p.add_argument('--sdk',type=Path,required=True)
p.add_argument('--version',default='00.70')
a=p.parse_args(); root=Path(__file__).resolve().parents[1]; ref=a.reference_work.resolve(); sdk=a.sdk.resolve()
out=root/'out'/('NPS64-'+a.version+'-source.tar.gz')
blocked={'.o','.a','.so','.prx','.sprx','.elf','.oelf','.pkg','.pyc','.d','.z64','.n64','.v64','.srm','.state','.sav','.profraw'}
def keep(info):
 path=Path(info.name)
 if any(x in {'.git','__pycache__','node_modules'} for x in path.parts) or path.suffix.lower() in blocked:return None
 return info
with tarfile.open(out,'w:gz',compresslevel=6) as t:
 for name in subprocess.check_output(['git','ls-files'],cwd=root,text=True).splitlines():
  if (root/name).is_file():t.add(root/name,arcname=name,filter=keep)
 for source,target in [(root/'vendor/parallel-n64','vendor/parallel-n64'),(root/'build/gpu-game-test/core','corresponding/core-patched'),(ref/'gpu/mesa-26.0.8','corresponding/mesa-26.0.8'),(ref/'gpu/sysroot','corresponding/sysroot'),(ref/'DolphinPS4-reference/port','corresponding/dolphin-port'),(ref/'DolphinPS4-reference/LICENSE','corresponding/DOLPHIN-LICENSE'),(sdk/'src/modules','corresponding/openorbis-modules'),(sdk/'LICENSE','corresponding/OPENORBIS-LICENSE'),(root/'build/compute-probe-ps4/ac_ps4_drm.c','corresponding/ac_ps4_drm.c'),(root/'build/gpu-game-test/manifest.json','corresponding/build-manifest.json')]:
  t.add(source,arcname=target,filter=keep)
 for name in ['prepare.py','cross.ini','libraries.json']:
  t.add(ref/'gpu'/name,arcname='corresponding/gpu-build/'+name,filter=keep)
print(out)

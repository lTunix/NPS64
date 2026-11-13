#!/usr/bin/env python3
"""Serialize captured layouts/SPIR-V and compile them without command submission.

Requires the AMDGPU noop drm-shim in LD_PRELOAD and AMDGPU_GPU_ID=bonaire.
Host RADV compiler output is not evidence of PS4 machine-code correctness.
"""
import argparse,json,os,re,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('capture',type=Path);p.add_argument('--compiler',type=Path,required=True)
p.add_argument('--output',type=Path,required=True);p.add_argument('--pipeline',action='append')
args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
results=[]
for path in sorted(args.capture.glob('pipeline-*.json')):
    data=json.loads(path.read_text());key=data['pipeline']
    if args.pipeline and key not in args.pipeline:continue
    if data['flags']:raise SystemExit('Subgroup stage flags are unsupported by this compiler harness')
    bindings=data['bindings'];spec=data['specialization'];push=data['push']
    code=(args.capture/('shader-'+data['shader']+'.spv')).read_bytes()
    header=struct.pack('<9I',0x5043504e,1,*push,data['flags'],len(bindings),len(spec),len(code))
    blob=header+b''.join(struct.pack('<5I',*b) for b in bindings)+b''.join(struct.pack('<2I',*s) for s in spec)+code
    binary=args.output/(key+'.npcp');binary.write_bytes(blob)
    env=dict(os.environ,RADV_DEBUG='shaderstats',MESA_SHADER_CACHE_DISABLE='true')
    result=subprocess.run([str(args.compiler.resolve()),str(binary.resolve())],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=120)
    (args.output/(key+'.stats.txt')).write_text(result.stdout)
    def number(label):
        match=re.search(r'^'+re.escape(label)+r': (\d+)',result.stdout,re.M)
        return int(match.group(1)) if match else None
    stats={'pipeline':key,'shader':data['shader'],'exit':result.returncode,'sgprs':number('SGPRs'),
           'vgprs':number('VGPRs'),'scratch_bytes':number('Scratch size'),'lds_bytes':number('LDS size'),
           'spilled_sgprs':number('Spilled SGPRs'),'spilled_vgprs':number('Spilled VGPRs')}
    results.append(stats);print(json.dumps(stats),flush=True)
    if result.returncode:raise SystemExit('Offline compile failed: '+key)
if not results:raise SystemExit('No matching captured pipelines')
(args.output/'report.json').write_text(json.dumps({'environment':'Host RADV + virtual BONAIRE; compilation only; no PS4 execution','pipelines':results},indent=2)+'\n')

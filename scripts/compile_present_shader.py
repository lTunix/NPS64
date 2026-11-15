#!/usr/bin/env python3
import argparse,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--glslang',required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'build/present-72/present.spv';out.parent.mkdir(parents=True,exist_ok=True)
subprocess.run([a.glslang,'-V','--target-env','vulkan1.1',str(root/'src/gpu_present.comp'),'-o',str(out)],check=True)
data=out.read_bytes();words=struct.unpack('<'+'I'*(len(data)//4),data)
(root/'src/gpu_present_spv.h').write_text('/* Generated from gpu_present.comp by compile_present_shader.py. */\n#pragma once\n#include <stdint.h>\nstatic const uint32_t nps64_present_spv[]={\n'+''.join(' '+','.join(hex(w) for w in words[i:i+8])+',\n' for i in range(0,len(words),8))+'};\n')

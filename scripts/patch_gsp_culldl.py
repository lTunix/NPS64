#!/usr/bin/env python3
"""Decode legacy and modern GBI 1 cull ranges without changing the clip test."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-angrylion'
(d/'nps64_gsp_culldl.h').write_bytes((project/'src/gsp_culldl.h').read_bytes())
p=d/'rdp_emit_frontend.h';s=p.read_text()
if 'int nps64_culldl_legacy;' not in s:
 assert s.count('} GSPState;')==1;s=s.replace('} GSPState;','    int nps64_culldl_legacy; /* F3DEX/F3DLX 0.95 retains the Fast3D cull format. */\n} GSPState;');p.write_text(s)
p=d/'rdp_emit_hle.c';s=p.read_text()
if '#include "nps64_gsp_culldl.h"' not in s:
 s='#include "nps64_gsp_culldl.h"\n'+s
 anchor='    int ut_is_task_start = gsp_params_at_task_start;';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    st->nps64_culldl_legacy=nps64_f3dex095(rdram,rdram_size,ud);');p.write_text(s)
p=d/'rdp_emit_f3d.c';s=p.read_text()
if '#include "nps64_gsp_culldl.h"' not in s:
 s='#include "nps64_gsp_culldl.h"\n'+s
 old='''                int v0 = (int)(((w0 >> 16) & 0xffu) / 2u);
                int vn = (int)((w1 & 0xffu) / 2u);
                if (gsp_culldl_test(gsp, v0, vn))'''
 new='''                int v0,vn;
                /* Fast3D and 0.95 use 40-byte vertex offsets with an
                 * exclusive end; later F3DEX uses doubled indices. */
                if (nps64_culldl_range(w0,w1,!s_variant_f3dex||gsp->nps64_culldl_legacy,
                                      GSP_MAX_VERTICES,&v0,&vn)
                    && gsp_culldl_test(gsp, v0, vn))'''
 assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
print('GBI 1 legacy/modern cull ranges prepared in',root)

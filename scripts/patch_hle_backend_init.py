#!/usr/bin/env python3
"""Bind the Vulkan HLE backend before the first streaming graphics task."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve()
project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-paraLLEl'
p=d/'parallel_hle.cpp';s=p.read_text()
if 'void parallel_hle_bind_backend(void)' not in s:
    anchor='/* Called from parallelProcessDList() (parallel.cpp). */'
    assert s.count(anchor)==1
    s=s.replace(anchor,'''/* Streaming tasks bypass ProcessDList. Install callbacks at plugin connect
 * and ROM open as well, before the RSP can dispatch its first task. */
void parallel_hle_bind_backend(void)
{
    rdp_emit_set_backend(&par_hle_backend);
}

'''+anchor)
    p.write_text(s)
p=d/'parallel.cpp';s=p.read_text()
if 'void parallel_hle_bind_backend(void);' not in s:
    anchor='void parallel_hle_process_dlist(void);';assert s.count(anchor)==1
    s=s.replace(anchor,anchor+'\nvoid parallel_hle_bind_backend(void);')
    for anchor in ['int parallelInitiateGFX(GFX_INFO Gfx_Info)\n{','int parallelRomOpen(void)\n{']:
        assert s.count(anchor)==1
        s=s.replace(anchor,anchor+'\n\tparallel_hle_bind_backend();')
    p.write_text(s)
print('Early Vulkan HLE backend binding prepared in',root)

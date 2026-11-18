#!/usr/bin/env python3
"""Allow a per-content full tile path without changing other optimizations."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
p=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.cpp';s=p.read_text()
if 'nps64_parallel_set_safe_tiles' not in s:
 s='''#include <atomic>
namespace { std::atomic_bool nps64_full_tile_compat{false}; }
extern "C" void nps64_parallel_set_safe_tiles(unsigned enabled)
{
    nps64_full_tile_compat.store(enabled!=0,std::memory_order_relaxed);
}
'''+s
 old='''    return enabled;
}
}'''
 a=s.index('bool nps64_active_tiles_enabled()');b=s.index('\n}',a)
 part=s[a:b];assert part.count('return enabled;')==1
 s=s[:a]+part.replace('return enabled;','return enabled && !nps64_full_tile_compat.load(std::memory_order_relaxed);')+s[b:]
 p.write_text(s)
print('Per-content full tile compatibility prepared in',root)

#!/usr/bin/env python3
"""Count actual render-batch flush causes without GPU timestamp queries."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve(); project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp'
p=d/'rdp_renderer.hpp';s=p.read_text()
if 'unsigned nps64_reason' not in s:
 assert s.count('void flush_queues();')==1
 p.write_text(s.replace('void flush_queues();','void flush_queues(unsigned nps64_reason = 0);'))
p=d/'rdp_renderer.cpp';s=p.read_text()
if '[rdp-batches]' not in s:
 # Different source copies can have earlier patches, so anchor on function names.
 # All flush call sites are classified from their containing function; unknown
 # sites remain visible rather than silently being counted as a known cause.
 import re
 matches=list(re.finditer(r'(?m)^\w[^\n]*Renderer::([A-Za-z_]+)\([^;]*?\)\s*(?:const\s*)?\n\{',s))
 reasons={'flush_and_signal':1,'set_color_framebuffer':2,'set_depth_framebuffer':3,'draw_shaded_primitive':4,'load_tile':6,'maintain_queues_idle':5,'load_tile_iteration':7}
 for m in reversed(matches):
  a=m.start();end=s.find('\n}\n',m.end())+3
  name=m.group(1);part=s[a:end]
  if 'flush_queues();' in part:
   reason=reasons.get(name,0)
   part=part.replace('flush_queues();',f'flush_queues({reason});')
   s=s[:a]+part+s[end:]
 s=s.replace('void Renderer::flush_queues()','void Renderer::flush_queues(unsigned nps64_reason)')
 anchor='\n\tif (!is_host_coherent)\n';start=s.index('void Renderer::flush_queues(unsigned')
 pos=s.index(anchor,start)
 s=s[:pos]+'''
    // Count nonempty batches only. Atomics permit the UI thread to report
    // while the FIFO render worker is active, without blocking that worker.
    nps64_batch_stats[nps64_reason < 8 ? nps64_reason : 0].fetch_add(1, std::memory_order_relaxed);
    nps64_batch_stats[8].fetch_add(stream.triangle_setup.size(), std::memory_order_relaxed);
    nps64_batch_stats[9].fetch_add(stream.tmem_upload_infos.size(), std::memory_order_relaxed);
    const bool limits[] = {stream.static_raster_state_cache.full(),
        stream.depth_blend_state_cache.full(),
        stream.tile_info_state_cache.size() + 8 > Limits::MaxTileInfoStates,
        stream.triangle_setup.full(),
        stream.span_info_jobs.size() * ImplementationConstants::DefaultWorkgroupSize + Limits::MaxHeight > Limits::MaxSpanSetups,
        stream.max_shaded_tiles + caps.max_tiles_x * caps.max_tiles_y > caps.max_num_tile_instances};
    for (unsigned i=0;i<6;i++) if(limits[i])
        nps64_batch_stats[10+i].fetch_add(1,std::memory_order_relaxed);
'''+s[pos:]
 # Counters never change FIFO ordering, barriers, limits or rendering.
 s='''#include <atomic>
namespace { std::atomic<unsigned long long> nps64_batch_stats[16]{}; }
extern "C" void nps64_parallel_flush_log(void)
{
    unsigned long long v[16];
    for(unsigned i=0;i<16;i++)v[i]=nps64_batch_stats[i].exchange(0,std::memory_order_relaxed);
    unsigned long long batches=0;for(unsigned i=0;i<8;i++)batches+=v[i];
    // Printed below the normal logging includes via the helper definition.
    extern void nps64_batch_stats_print(const unsigned long long *,unsigned long long);
    nps64_batch_stats_print(v,batches);
}
'''+s
 # LOGI is provided by the renderer's existing includes.
 anchor='namespace RDP\n{';assert s.count(anchor)==1
 s=s.replace(anchor,'''void nps64_batch_stats_print(const unsigned long long *v,unsigned long long batches)
{
    LOGI("[rdp-batches] batches=%llu triangles=%llu tmem=%llu unknown=%llu sync=%llu color=%llu depth=%llu capacity=%llu idle=%llu texture_hazard=%llu tmem_capacity=%llu static_full=%llu depth_full=%llu tile_full=%llu primitive_full=%llu span_full=%llu shading_full=%llu\\n",
        batches,v[8],v[9],v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[10],v[11],v[12],v[13],v[14],v[15]);
}

'''+anchor,1)
 p.write_text(s)
print('Low overhead render batch counters prepared in',root)

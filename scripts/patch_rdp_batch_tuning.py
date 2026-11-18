#!/usr/bin/env python3
"""Coalesce native/2x submissions and skip untouched indirect-buffer resets."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp'
(d/'nps64_rdp_batch_policy.h').write_bytes((project/'src/rdp_batch_policy.h').read_bytes())
p=d/'rdp_renderer.cpp';s=p.read_text()
if '[rdp-submit-batches]' not in s:
 s='#include "nps64_rdp_batch_policy.h"\n'+s
 anchor='namespace RDP\n{';assert s.count(anchor)==1
 s=s.replace(anchor,'''namespace {
std::atomic<unsigned long long> nps64_submit_stats[6]{};
bool nps64_batch_tuning_enabled()
{
    static const bool enabled=getenv("NPS64_RDP_REFERENCE_SUBMISSIONS")==nullptr;
    return enabled;
}
}
extern "C" void nps64_parallel_batch_tuning_log(void)
{
    unsigned long long v[6];
    for(unsigned i=0;i<6;i++)v[i]=nps64_submit_stats[i].exchange(0,std::memory_order_relaxed);
    LOGI("[rdp-submit-batches] submissions=%llu native_passes=%llu upscaled_passes=%llu primitives=%llu skipped_indirect_resets=%llu paired_passes=%llu\\n",v[0],v[1],v[2],v[3],v[4],v[5]);
}

'''+anchor)
 # Pair the dependent 1x and 2x work before the regular submit heuristic.
 old='''\t\t\tmaintain_queues();
\t\t\tensure_command_buffer();''';assert s.count(old)==1
 s=s.replace(old,'''            if (!nps64_batch_tuning_enabled()) maintain_queues();
            else nps64_submit_stats[5].fetch_add(1,std::memory_order_relaxed);
\t\t\tensure_command_buffer();''')
 start=s.index('void Renderer::maintain_queues()');a=s.index('\tif (pending_render_passes >=',start);b=s.index('\n\t{\n\t\tsubmit_to_queue();',a)
 s=s[:a]+'''    const auto age=uint64_t(Util::get_current_time_nsecs()-last_submit_ns);
    const auto active=active_submissions.load(std::memory_order_relaxed);
    if (nps64_batch_tuning_enabled() ?
        nps64_rdp_should_submit(pending_render_passes,pending_render_passes_upscaled,
            pending_primitives,pending_primitives_upscaled,active,age) :
        (pending_render_passes >= ImplementationConstants::MaxPendingRenderPassesBeforeFlush ||
         (caps.super_sample_readback && pending_render_passes_upscaled >= ImplementationConstants::MaxPendingRenderPassesBeforeFlush) ||
         pending_primitives >= Limits::MaxPrimitives || pending_primitives_upscaled >= Limits::MaxPrimitives ||
         active==0 || age>1000000))'''+s[b:]
 anchor='void Renderer::submit_to_queue()\n{';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
    if(stream.cmd) {
        nps64_submit_stats[0].fetch_add(1,std::memory_order_relaxed);
        nps64_submit_stats[1].fetch_add(pending_render_passes,std::memory_order_relaxed);
        nps64_submit_stats[2].fetch_add(pending_render_passes_upscaled,std::memory_order_relaxed);
        nps64_submit_stats[3].fetch_add(pending_primitives,std::memory_order_relaxed);
    }
''')
 # Texture-only passes never bin or shade primitives, so their indirect
 # dispatch counters have not changed. Initialization and every real render
 # pass retain the established reset shader.
 a=s.index('void Renderer::submit_render_pass(Vulkan::CommandBuffer &cmd)')
 b=s.index('void Renderer::submit_render_pass_upscaled',a)
 part=s[a:b];old='\tif (!caps.ubershader)\n\t\tclear_indirect_buffer(cmd);';assert part.count(old)==1
 part=part.replace(old,'''    if (!caps.ubershader) {
        if (need_render_pass || !nps64_batch_tuning_enabled()) clear_indirect_buffer(cmd);
        else nps64_submit_stats[4].fetch_add(1,std::memory_order_relaxed);
    }''')
 s=s[:a]+part+s[b:]
 p.write_text(s)
print('Paired/batched queue submissions and texture-only reset elimination prepared in',root)

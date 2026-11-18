#!/usr/bin/env python3
"""Expose the existing Vulkan stage timestamp profiler with a locked report."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan'
p=d/'device.hpp';s=p.read_text()
if 'nps64_timestamp_report' not in s:
 anchor='\tvoid timestamp_log_reset();';assert s.count(anchor)==1
 s=s.replace(anchor,'\tvoid nps64_timestamp_report(const TimestampIntervalReportCallback &cb);\n'+anchor);p.write_text(s)
p=d/'device.cpp';s=p.read_text()
if 'void Device::nps64_timestamp_report' not in s:
 anchor='void Device::timestamp_log_reset()\n{';assert s.count(anchor)==1
 s=s.replace(anchor,'''void Device::nps64_timestamp_report(const TimestampIntervalReportCallback &cb)
{
    // Compiler threads also register intervals. Report and reset under the
    // same device lock that protects publication and frame recycling.
    LOCK();
    managers.timestamps.log_simple(cb);
    managers.timestamps.reset();
}

'''+anchor);p.write_text(s)
p=root/'mupen64plus-video-paraLLEl/rdp.cpp';s=p.read_text()
if 'nps64_parallel_stage_log' not in s:
 anchor='static unique_ptr<Device> device;';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
extern "C" void nps64_parallel_stage_log(void)
{
    const char *enabled=getenv("PARALLEL_RDP_BENCH");
    if (!device || !enabled || strtol(enabled,nullptr,0)<2)return;
    device->nps64_timestamp_report([](const std::string &tag,const Vulkan::TimestampIntervalReport &r) {
        if (tag=="span-setup" || tag=="span-info-jobs" || tag=="tile-binning" ||
            tag=="shading" || tag=="depth-blending" || tag=="render-pass" || tag=="render-pass-upscaled" ||
            tag=="tmem-update" || tag=="ssaa-resolve" || tag=="coherent-copy" ||
            tag=="extract-vram" || tag=="vi-fetch" || tag=="vi-divot" || tag=="vi-scale")
            LOGI("[gpu-stage] stage=%s ms_per_context=%.6f ms_per_call=%.6f calls_per_context=%.3f\\n",
                 tag.c_str(),1000.0*r.time_per_frame_context,
                 1000.0*r.time_per_accumulation,r.accumulations_per_frame_context);
    });
}
''');p.write_text(s)
# Upgrade a previously prepared profiling copy too.
s=p.read_text()
if 'tag=="render-pass-upscaled"' not in s:
 s=s.replace('tag=="render-pass" ||','tag=="render-pass" || tag=="render-pass-upscaled" ||')
 p.write_text(s)
print('Locked GPU stage timestamp report prepared in',root)

#!/usr/bin/env python3
"""Measure inclusive RDP/VI time and FullSync waits without changing ordering."""
from pathlib import Path
import sys
project=Path(__file__).resolve().parents[1]
root=Path(sys.argv[1]).resolve()
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy, not vendor')
p=root/'mupen64plus-video-paraLLEl/rdp.cpp';s=p.read_text()
if 'Nps64RdpMeasure' not in s:
 anchor='namespace RDP\n{'
 assert s.count(anchor)==1
 s=s.replace(anchor,'''extern "C" double nps64_profile_time(void);
namespace RDP
{
static double nps64_list_seconds,nps64_scanout_seconds,nps64_fullsync_seconds;
static unsigned nps64_fullsync_calls;
struct Nps64RdpMeasure {
 double &total;double start;
 explicit Nps64RdpMeasure(double &value):total(value),start(nps64_profile_time()){}
 ~Nps64RdpMeasure(){total+=nps64_profile_time()-start;}
};
extern "C" void nps64_parallel_perf_take(double *list,double *vi,double *sync,unsigned *calls){
 *list=nps64_list_seconds;*vi=nps64_scanout_seconds;*sync=nps64_fullsync_seconds;*calls=nps64_fullsync_calls;
 nps64_list_seconds=nps64_scanout_seconds=nps64_fullsync_seconds=0;nps64_fullsync_calls=0;
}''')
 for name,counter in [('process_commands','nps64_list_seconds'),('complete_frame','nps64_scanout_seconds')]:
  anchor='void '+name+'()\n{';assert s.count(anchor)==1
  s=s.replace(anchor,anchor+'\n Nps64RdpMeasure nps64_measure('+counter+');')
 anchor='bool init()\n{';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n nps64_list_seconds=nps64_scanout_seconds=nps64_fullsync_seconds=0;nps64_fullsync_calls=0;')
 old='''\t\t\tif (synchronous && frontend)
\t\t\t\tfrontend->wait_for_timeline(frontend->signal_timeline());'''
 assert s.count(old)==1
 s=s.replace(old,'''\t\t\tif (synchronous && frontend) {
                Nps64RdpMeasure nps64_measure_sync(nps64_fullsync_seconds);
                frontend->wait_for_timeline(frontend->signal_timeline());
                nps64_fullsync_calls++;
            }''')
 p.write_text(s)
print('RDP/VI inclusive timings and FullSync wait measurements enabled in '+str(root))

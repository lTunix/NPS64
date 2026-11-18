#!/usr/bin/env python3
"""Measure HLE task and streaming-slice wall time without per-vertex clocks."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve()
project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
p=root/'mupen64plus-video-angrylion/rdp_emit_hle.c'
s=p.read_text()
if 'nps64_hle_cpu_perf_take' not in s:
    anchor='void rdp_emit_hle_process_dlist(void)'
    assert s.count(anchor)==1
    s=s.replace(anchor,'static void nps64_hle_process_dlist_impl(void)',1)
    s='''#include <stdint.h>
#include <time.h>
#include <stdatomic.h>
static atomic_ullong nps64_hle_task_ns;
static atomic_uint nps64_hle_task_calls;
static uint64_t nps64_hle_clock_ns(void)
{
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
void nps64_hle_cpu_perf_take(double *seconds,unsigned *calls)
{
    *seconds=(double)atomic_exchange_explicit(&nps64_hle_task_ns,0,memory_order_relaxed)/1e9;
    *calls=atomic_exchange_explicit(&nps64_hle_task_calls,0,memory_order_relaxed);
}
'''+s
    s+='''
void rdp_emit_hle_process_dlist(void)
{
    uint64_t begin=nps64_hle_clock_ns(),end;
    nps64_hle_process_dlist_impl();
    end=nps64_hle_clock_ns();
    if(begin && end>=begin)
        atomic_fetch_add_explicit(&nps64_hle_task_ns,end-begin,memory_order_relaxed);
    atomic_fetch_add_explicit(&nps64_hle_task_calls,1,memory_order_relaxed);
}
'''
    p.write_text(s)
print('One-shot HLE CPU timing prepared in',root)

# Streaming display lists do not pass through the one-shot entry point.
s=p.read_text()
if 'nps64_hle_streaming_dlist_impl' not in s:
    anchor='int angrylion_streaming_dlist(int resume)'
    assert s.count(anchor)==1
    s=s.replace(anchor,'static int nps64_hle_streaming_dlist_impl(int resume)',1)
    s+='\nint angrylion_streaming_dlist(int resume)\n{\n    uint64_t begin=nps64_hle_clock_ns(),end;\n    int result=nps64_hle_streaming_dlist_impl(resume);\n    end=nps64_hle_clock_ns();\n    if(begin && end>=begin)\n        atomic_fetch_add_explicit(&nps64_hle_task_ns,end-begin,memory_order_relaxed);\n    atomic_fetch_add_explicit(&nps64_hle_task_calls,1,memory_order_relaxed);\n    return result;\n}\n'
    p.write_text(s)

s=p.read_text()
if 'nps64_hle_stream_fallback_take' not in s:
    s=s.replace('static atomic_uint nps64_hle_task_calls;', 'static atomic_uint nps64_hle_task_calls;\nstatic atomic_uint nps64_hle_stream_fallbacks;\nunsigned nps64_hle_stream_fallback_take(void)\n{ return atomic_exchange_explicit(&nps64_hle_stream_fallbacks,0,memory_order_relaxed); }')
    anchor='    int result=nps64_hle_streaming_dlist_impl(resume);'
    assert s.count(anchor)==1
    s=s.replace(anchor,anchor+'\n    if(result<0)atomic_fetch_add_explicit(&nps64_hle_stream_fallbacks,1,memory_order_relaxed);')
    p.write_text(s)

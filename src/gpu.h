#ifndef NPS64_GPU_H
#define NPS64_GPU_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef struct {
    uint64_t readbacks;
    double record_seconds,submit_seconds,wait_seconds,invalidate_seconds,total_seconds,snapshot_seconds;
} Nps64GpuPerf;
#ifdef NPS64_GPU
void nps64_gpu_perf_take(Nps64GpuPerf*);
void nps64_gpu_perf_snapshot(double);
bool nps64_gpu_begin(void);
bool nps64_gpu_environment(unsigned,void *);
bool nps64_gpu_start(void);
void nps64_gpu_destroy_context(void);
void nps64_gpu_close(void);
bool nps64_gpu_readback(const void **,unsigned,unsigned);
bool nps64_gpu_failed(void);
#else
static inline void nps64_gpu_perf_take(Nps64GpuPerf *out) {*out=(Nps64GpuPerf){0};}
static inline void nps64_gpu_perf_snapshot(double seconds) {(void)seconds;}
static inline bool nps64_gpu_begin(void) {return false;}
static inline bool nps64_gpu_environment(unsigned c,void *p) {(void)c;(void)p;return false;}
static inline bool nps64_gpu_start(void) {return false;}
static inline void nps64_gpu_destroy_context(void) {}
static inline void nps64_gpu_close(void) {}
static inline bool nps64_gpu_readback(const void **p,unsigned w,unsigned h) {(void)p;(void)w;(void)h;return false;}
static inline bool nps64_gpu_failed(void) {return false;}
#endif
#endif

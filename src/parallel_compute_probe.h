#pragma once
#include <stdint.h>

typedef struct ParallelComputeProbe ParallelComputeProbe;
typedef void (*ParallelProbeReport)(const char *message);
enum { PARALLEL_PROBE_CASES = 7, PARALLEL_PROBE_REPEATS = 64 };
/* One device for the lifetime of the application. A timeout marks it unusable. */
int parallel_probe_open(ParallelComputeProbe **probe, ParallelProbeReport report);
const char *parallel_probe_case_name(unsigned index);
int parallel_probe_run(ParallelComputeProbe *probe, unsigned index, unsigned repeats);
void parallel_probe_close(ParallelComputeProbe *probe);

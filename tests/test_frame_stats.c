#include "frame_stats.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    Nps64FrameStats stats = {0};
    assert(nps64_frame_stats_percentile(&stats, 95) == 0);
    for (unsigned i = 1; i <= 100; i++) nps64_frame_stats_add(&stats, i / 1000.0);
    assert(stats.count == 100);
    assert(fabs(stats.sum_ms - 5050) < 0.001);
    assert(nps64_frame_stats_percentile(&stats, 50) == 50.25);
    assert(nps64_frame_stats_percentile(&stats, 95) == 95.25);
    assert(nps64_frame_stats_percentile(&stats, 99) == 99.25);
    assert(nps64_frame_stats_percentile(&stats, 100) == 100);
    nps64_frame_stats_add(&stats, -1);
    nps64_frame_stats_add(&stats, NAN);
    assert(stats.count == 100);
    nps64_frame_stats_reset(&stats);
    nps64_frame_stats_add(&stats, 2);
    assert(nps64_frame_stats_percentile(&stats, 95) == 2000);
    nps64_frame_stats_reset(&stats);
    nps64_frame_stats_add(&stats, 0);
    assert(nps64_frame_stats_percentile(&stats, 95) == 0);
    puts("PASS timing distributions: ranks, bounds, stalls, reset and invalid samples");
}

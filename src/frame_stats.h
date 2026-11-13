/* Bounded, allocation-free timings. Percentiles are upper bounds in 0.25 ms
 * buckets; values above 256 ms are reported as the observed maximum. */
#pragma once
#include <stdint.h>
#include <string.h>
typedef struct {
    uint32_t buckets[1025];
    uint32_t count;
    double sum_ms, maximum_ms;
} Nps64FrameStats;
static inline void nps64_frame_stats_reset(Nps64FrameStats *stats) {
    memset(stats, 0, sizeof(*stats));
}
static inline void nps64_frame_stats_add(Nps64FrameStats *stats, double seconds) {
    if (!(seconds >= 0)) return;
    double ms = seconds * 1000;
    unsigned bucket = ms >= 256 ? 1024 : (unsigned)(ms * 4);
    stats->buckets[bucket]++;
    stats->count++;
    stats->sum_ms += ms;
    if (ms > stats->maximum_ms) stats->maximum_ms = ms;
}
static inline double nps64_frame_stats_percentile(const Nps64FrameStats *stats, unsigned percent) {
    if (!stats->count || !percent || percent > 100) return 0;
    uint64_t rank = ((uint64_t)stats->count * percent + 99) / 100;
    uint64_t total = 0;
    for (unsigned i = 0; i < 1025; i++) {
        total += stats->buckets[i];
        if (total >= rank) {
            double upper = (i + 1) * 0.25;
            return i == 1024 || upper > stats->maximum_ms ? stats->maximum_ms : upper;
        }
    }
    return stats->maximum_ms;
}

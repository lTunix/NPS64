#pragma once

/* Keep at most one frame of scheduling debt. Expensive VIs must not erase
 * all debt and make the following inexpensive VI sleep for a full period.
 * A sustained overload never sleeps; a long stall cannot cause an unbounded
 * burst of catch-up frames. This does not change the emulated VI rate. */
static inline double nps64_frame_deadline(double previous, double now,
                                        double period)
{
    double next = previous + period;
    double oldest = now - period;
    return next < oldest ? oldest : next;
}

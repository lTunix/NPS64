#include "frame_pacing.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static double simulate(double period, int legacy, int pattern)
{
    double now = 0, deadline = 0;
    for (unsigned i = 0; i < 12000; ++i) {
        double work = pattern == 0 ? period * .25 :
                      pattern == 1 ? period * 1.8 :
                      (i & 1) ? period * .1 : period * 2.5;
        now += work;
        if (legacy) {
            deadline += period;
            if (deadline < now - period) deadline = now;
        } else deadline = nps64_frame_deadline(deadline, now, period);
        if (deadline > now) now = deadline;
        assert(deadline >= now - period - 1e-9);
    }
    return now;
}

int main(void)
{
    for (unsigned hz = 50; hz <= 60; hz += 10) {
        double period = 1.0 / hz;
        assert(fabs(simulate(period, 0, 0) - 12000 * period) < 1e-6);
        assert(fabs(simulate(period, 0, 1) - 12000 * period * 1.8) < 1e-6);
        double old = simulate(period, 1, 2), improved = simulate(period, 0, 2);
        assert(improved < old * .8);
        assert(fabs(improved - 12000 * period * 1.3) < 1e-6);
        /* A minute-long stall retains at most one VI of debt. */
        assert(fabs(nps64_frame_deadline(0, 60, period) - (60-period)) < 1e-9);
        printf("PASS %u Hz: alternating workload %.3f -> %.3f simulated seconds\n",
               hz, old, improved);
    }
}

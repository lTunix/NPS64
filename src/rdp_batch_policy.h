#ifndef NPS64_RDP_BATCH_POLICY_H
#define NPS64_RDP_BATCH_POLICY_H
#include <stdint.h>
/* Keep the existing hard batch limits and deadline. An idle GPU does not
 * require a separate submission for every microscopic render pass. */
static inline int nps64_rdp_should_submit(unsigned passes,unsigned upscaled,
        unsigned primitives,unsigned upscaled_primitives,unsigned active,
        uint64_t elapsed_ns)
{
    if(passes>=8 || upscaled>=8 || primitives>=256 || upscaled_primitives>=256)
        return 1;
    if(elapsed_ns>=1000000) return 1;
    unsigned completed=passes>upscaled?passes:upscaled;
    return active==0 && (completed>=4 || elapsed_ns>=250000);
}
#endif

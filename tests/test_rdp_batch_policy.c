#include "rdp_batch_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned cases=0;
    const uint64_t ages[]={0,249999,250000,999999,1000000,2000000};
    for(unsigned native=0;native<=8;native++)for(unsigned up=0;up<=8;up++)
    for(unsigned prim=0;prim<=256;prim+=32)for(unsigned active=0;active<=2;active++)
    for(unsigned t=0;t<sizeof(ages)/sizeof(*ages);t++) {
        int submit=nps64_rdp_should_submit(native,up,prim,prim,active,ages[t]);
        if(native==8 || up==8 || prim==256 || ages[t]>=1000000)assert(submit);
        if(active==0 && ages[t]>=250000)assert(submit);
        cases++;
    }
    /* Busy GPU can accumulate small work, but deadlines and existing hard
     * limits always force progress. Idle microbatches have a shorter deadline. */
    assert(!nps64_rdp_should_submit(1,1,8,8,0,100000));
    assert(nps64_rdp_should_submit(1,1,8,8,0,250000));
    assert(nps64_rdp_should_submit(4,4,8,8,0,0));
    assert(!nps64_rdp_should_submit(4,4,8,8,1,999999));
    assert(nps64_rdp_should_submit(1,1,8,8,1,1000000));
    assert(nps64_rdp_should_submit(8,0,8,0,1,0));
    assert(nps64_rdp_should_submit(0,8,0,8,1,0));
    assert(nps64_rdp_should_submit(1,1,256,8,1,0));
    assert(nps64_rdp_should_submit(1,1,8,256,1,0));
    printf("PASS: %u submission progress/deadline cases\n",cases);
}

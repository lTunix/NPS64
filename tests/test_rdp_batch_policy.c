#include "rdp_batch_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned cases=0;
    const uint64_t ages[]={0,499999,500000,1999999,2000000,4000000};
    for(unsigned native=0;native<=32;native+=4)for(unsigned up=0;up<=32;up+=4)
    for(unsigned prim=0;prim<=512;prim+=64)for(unsigned active=0;active<=2;active++)
    for(unsigned t=0;t<sizeof(ages)/sizeof(*ages);t++) {
        int submit=nps64_rdp_should_submit(native,up,prim,prim,active,ages[t]);
        if(native==32 || up==32 || prim==512 || ages[t]>=2000000)assert(submit);
        if(active==0 && ages[t]>=500000)assert(submit);
        cases++;
    }
    /* Busy GPU can accumulate small work, but deadlines and existing hard
     * limits always force progress. Idle microbatches have a shorter deadline. */
    assert(!nps64_rdp_should_submit(1,1,8,8,0,250000));
    assert(nps64_rdp_should_submit(1,1,8,8,0,500000));
    assert(nps64_rdp_should_submit(16,16,8,8,0,0));
    assert(!nps64_rdp_should_submit(16,16,8,8,1,1999999));
    assert(nps64_rdp_should_submit(1,1,8,8,1,2000000));
    assert(nps64_rdp_should_submit(32,0,8,0,1,0));
    assert(nps64_rdp_should_submit(0,32,0,8,1,0));
    assert(nps64_rdp_should_submit(1,1,512,8,1,0));
    assert(nps64_rdp_should_submit(1,1,8,512,1,0));
    printf("PASS: %u submission progress/deadline cases\n",cases);
}

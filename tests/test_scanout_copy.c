#include "scanout_copy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){
    const size_t maximum=1280*720*4;
    unsigned char *src=malloc(maximum+32),*dst=malloc(maximum+64);
    assert(src&&dst);
    for(size_t i=0;i<maximum+32;i++)src[i]=(unsigned char)(i*73+i/251);
    const size_t sizes[]={0,1,15,16,63,64,65,127,128,1025,maximum};
    for(unsigned a=0;a<16;a++)for(unsigned b=0;b<16;b++)for(unsigned n=0;n<sizeof(sizes)/sizeof(sizes[0]);n++){
        memset(dst,0xa5,maximum+64);
        unsigned char *out=dst+16+b;
        nps64_scanout_copy(out,src+a,sizes[n]);
        assert(!memcmp(out,src+a,sizes[n]));
        for(unsigned i=0;i<16+b;i++)assert(dst[i]==0xa5);
        for(size_t i=16+b+sizes[n];i<maximum+64;i++)assert(dst[i]==0xa5);
    }
    free(src);free(dst);
    puts("PASS scanout copy: full frame, tails, alignment, canaries and exact bytes");
}

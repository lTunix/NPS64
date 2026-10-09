#include "cpu_game_profile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    uint8_t canonical[64]={0x80,0x37,0x12,0x40},encoded[64];
    const uint8_t crc[8]={0x09,0x46,0x5a,0xc3,0xf8,0xcb,0x50,0x1b};
    memcpy(canonical+16,crc,8);
    for(unsigned order=0;order<=3;order++) {
        if(order==2)continue;
        for(unsigned i=0;i<64;i++)encoded[i^order]=canonical[i];
        assert(nps64_cpu_profile_header(encoded,64)==1);
        assert(nps64_cpu_profile_header(encoded,63)==0);
        encoded[16^order]^=1;
        assert(nps64_cpu_profile_header(encoded,64)==0);
    }
    memset(encoded,0,sizeof(encoded));assert(nps64_cpu_profile_header(encoded,64)==0);
    puts("PASS ROM profile: exact CRC pair, all 3 byte orders, truncation and other games");
    return 0;
}

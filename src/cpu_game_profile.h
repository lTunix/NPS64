#pragma once
#include <stdint.h>
#include <stddef.h>
/* ROM header CRC pair identifies the tested PAL GameCube OoT revision.
 * Filenames and byte order do not affect selection. Other games stay on CPU46. */
static inline unsigned nps64_cpu_profile_header(const uint8_t *source,size_t size) {
    if(size<64)return 0;
    unsigned order;
    if(source[0]==0x80&&source[1]==0x37&&source[2]==0x12&&source[3]==0x40)order=0;
    else if(source[0]==0x37&&source[1]==0x80&&source[2]==0x40&&source[3]==0x12)order=1;
    else if(source[0]==0x40&&source[1]==0x12&&source[2]==0x37&&source[3]==0x80)order=3;
    else return 0;
    static const uint8_t crc[8]={0x09,0x46,0x5a,0xc3,0xf8,0xcb,0x50,0x1b};
    for(unsigned i=0;i<8;i++)if(source[(16+i)^order]!=crc[i])return 0;
    return 1;
}

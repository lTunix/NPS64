#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static inline uint32_t nps64_rom_be32(const uint8_t *p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
static inline void nps64_rom_put32(uint8_t *p,uint32_t value) {
    p[0]=(uint8_t)(value>>24);p[1]=(uint8_t)(value>>16);
    p[2]=(uint8_t)(value>>8);p[3]=(uint8_t)value;
}
/* IPL3 X102 checks the first MiB of cartridge code, before reaching a VI. */
static inline int nps64_rom_crc6102(const uint8_t *rom,size_t size,uint32_t crc[2]) {
    if(size<0x101000)return 0;
    uint32_t t1=0xf8ca4ddc,t2=t1,t3=t1,t4=t1,t5=t1,t6=t1;
    for(size_t i=0x1000;i<0x101000;i+=4) {
        uint32_t d=nps64_rom_be32(rom+i),previous=t6;
        t6+=d;t4+=t6<previous;t3^=d;
        unsigned shift=d&31;
        uint32_t r=(d<<shift)|(d>>((32-shift)&31));
        t5+=r;t2^=t2>d?r:t6^d;t1+=t5^d;
    }
    crc[0]=t6^t4^t3;crc[1]=t5^t2^t1;return 1;
}
/* Apply after ROM database lookup and before its word-endian conversion.
 * Only the verified VC dump matches. Keep its file, identity and saves intact. */
static inline int nps64_rom_compat(uint8_t *rom,size_t size,const uint8_t digest[16]) {
    static const uint8_t tennis_vc[16]={0xd4,0x0f,0xb0,0xe8,0x79,0x96,0xaf,0x90,
        0x2e,0xff,0x9c,0x19,0x40,0x27,0xfc,0x3b};
    if(size!=0x1000000 || memcmp(digest,tennis_vc,16) ||
       nps64_rom_be32(rom)!=0x80371240 ||
       nps64_rom_be32(rom+16)!=0x839f3ad5 ||
       nps64_rom_be32(rom+20)!=0x406d15fa)return 0;
    uint32_t crc[2];
    if(!nps64_rom_crc6102(rom,size,crc) ||
       crc[0]!=0x639f3ad5 || crc[1]!=0xa394cb4f)return 0;
    nps64_rom_put32(rom+16,crc[0]);nps64_rom_put32(rom+20,crc[1]);
    return 1;
}

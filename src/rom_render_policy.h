#ifndef NPS64_ROM_RENDER_POLICY_H
#define NPS64_ROM_RENDER_POLICY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Match the cartridge title, independent of file name or dump byte order. */
static inline bool nps64_rom_needs_full_tiles(const uint8_t *header,size_t size)
{
    if(!header || size<64)return false;
    unsigned swap;
    if(header[0]==0x80 && header[1]==0x37 && header[2]==0x12 && header[3]==0x40)swap=0;
    else if(header[0]==0x37 && header[1]==0x80 && header[2]==0x40 && header[3]==0x12)swap=1;
    else if(header[0]==0x40 && header[1]==0x12 && header[2]==0x37 && header[3]==0x80)swap=3;
    else return false;
    const char title[21]="MARIOKART64         ";
    for(unsigned i=0;i<20;i++)if(header[(32+i)^swap]!=(uint8_t)title[i])return false;
    return true;
}
#endif

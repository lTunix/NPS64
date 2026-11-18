#include "rom_render_policy.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    unsigned char header[64]={0x80,0x37,0x12,0x40},swapped[64];
    memcpy(header+32,"MARIOKART64         ",20);
    for(unsigned swap=0;swap<4;swap++) {
        if(swap==2)continue;
        for(unsigned i=0;i<64;i++)swapped[i]=header[i^swap];
        assert(nps64_rom_needs_full_tiles(swapped,64));
        assert(!nps64_rom_needs_full_tiles(swapped,63));
        swapped[32^swap]='X';assert(!nps64_rom_needs_full_tiles(swapped,64));
    }
    memcpy(header+32,"SUPER MARIO 64      ",20);
    assert(!nps64_rom_needs_full_tiles(header,64));
    header[0]=0;assert(!nps64_rom_needs_full_tiles(header,64));
    assert(!nps64_rom_needs_full_tiles(NULL,64));
    puts("PASS: cartridge-title compatibility, all three dump orders, malformed/truncated inputs and other games");
}

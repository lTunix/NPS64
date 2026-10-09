#include "rom_compat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv) {
    const uint8_t digest[16]={0xd4,0x0f,0xb0,0xe8,0x79,0x96,0xaf,0x90,
        0x2e,0xff,0x9c,0x19,0x40,0x27,0xfc,0x3b};
    uint8_t *rom=calloc(1,0x1000000),wrong[16]={0};
    assert(rom);
    uint32_t crc[2];
    assert(!nps64_rom_crc6102(rom,0x100fff,crc));
    assert(nps64_rom_crc6102(rom,0x101000,crc));
    assert(crc[0]==0xf8ca4ddc);
    assert(crc[1]==(uint32_t)(0xf8ca4ddcu+(0xf8ca4ddcu<<18)));
    assert(!nps64_rom_compat(rom,0,digest));
    assert(!nps64_rom_compat(rom,0x1000000,wrong));
    nps64_rom_put32(rom,0x80371240);
    nps64_rom_put32(rom+16,0x839f3ad5);nps64_rom_put32(rom+20,0x406d15fa);
    assert(!nps64_rom_compat(rom,0x1000000,digest)); /* payload guard */
    assert(nps64_rom_be32(rom+16)==0x839f3ad5);
    if(argc==2) {
        FILE *file=fopen(argv[1],"rb");assert(file);
        assert(fread(rom,1,0x1000000,file)==0x1000000 && fgetc(file)==EOF);
        fclose(file);
        uint8_t *reference=malloc(0x1000000);assert(reference);
        memcpy(reference,rom,0x1000000);
        assert(!nps64_rom_compat(rom,0x1000000,wrong));
        assert(!memcmp(reference,rom,0x1000000));
        assert(nps64_rom_compat(rom,0x1000000,digest));
        assert(nps64_rom_be32(rom+16)==0x639f3ad5 &&
               nps64_rom_be32(rom+20)==0xa394cb4f);
        for(size_t i=0;i<0x1000000;i++)
            if(i<16 || i>=24)assert(rom[i]==reference[i]);
        assert(!nps64_rom_compat(rom,0x1000000,digest)); /* already fixed */
        free(reference);
    }
    free(rom);
    puts("PASS ROM compatibility: length, digest, header, payload CRC and bounded write");
}

#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    mkdir("build/test-storage",0777);
    unsigned char rom[8192]={0x80,0x37,0x12,0x40},swapped[8192];
    rom[16]=0x12;rom[20]=0x34;rom[62]='E';
    Game games[4],g;
    assert(file_write_atomic("build/test-storage/a.z64",rom,sizeof(rom)));
    for(size_t i=0;i<sizeof(rom);i++) swapped[i]=rom[i^1];
    assert(file_write_atomic("build/test-storage/B.v64",swapped,sizeof(swapped)));
    for(size_t i=0;i<sizeof(rom);i++) swapped[i]=rom[i^3];
    assert(file_write_atomic("build/test-storage/c.n64",swapped,sizeof(swapped)));
    unsigned char junk[4096]={0};
    assert(file_write_atomic("build/test-storage/invalid.z64",junk,sizeof(junk)));
    assert(!rom_info("build/test-storage/invalid.z64",&g));
    assert(library_scan("build/test-storage",games,4)==3);
    assert(!strcmp(games[0].name,"a.z64") && !strcmp(games[1].name,"B.v64"));
    assert(!strcmp(games[0].key,games[1].key) && !strcmp(games[0].key,games[2].key));
    for(int i=0;i<3;i++) assert(rom_content_key(&games[i]));
    assert(!strcmp(games[0].key,games[1].key) && !strcmp(games[0].key,games[2].key));
    rom[128]=0x55;
    assert(file_write_atomic("build/test-storage/modified.z64",rom,sizeof(rom)));
    assert(rom_info("build/test-storage/modified.z64",&g) && rom_content_key(&g));
    assert(strcmp(g.key,games[0].key));
    unlink("build/test-storage/modified.z64");
    assert(library_scan("build/test-storage",games,1)==1);
    assert(library_scan("build/no-such-directory",games,4)==0);
    char payload[]="partida persistente",restored[sizeof(payload)];
    assert(file_write_atomic("build/test-storage/test.srm",payload,sizeof(payload)));
    assert(file_read_exact("build/test-storage/test.srm",restored,sizeof(restored)));
    assert(!memcmp(payload,restored,sizeof(payload)));
    assert(!file_read_exact("build/test-storage/test.srm",restored,sizeof(restored)-1));
    assert(state_write("build/test-storage/test.state",games[0].key,payload,sizeof(payload)));
    assert(state_read("build/test-storage/test.state",games[0].key,restored,sizeof(restored)));
    assert(!state_read("build/test-storage/test.state","otra-rom",restored,sizeof(restored)));
    FILE *f=fopen("build/test-storage/test.state","r+b");assert(f);
    assert(!fseek(f,-1,SEEK_END));fputc(0x7f,f);fclose(f);
    assert(!state_read("build/test-storage/test.state",games[0].key,restored,sizeof(restored)));
    assert(!file_write_atomic("build/no-such-directory/file",payload,sizeof(payload)));
    puts("PASS: byte order ROM, escaneo, persistencia, estados corruptos/incompatibles");
}

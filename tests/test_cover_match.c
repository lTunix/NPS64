#include "cover_match.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    FILE *f=fopen("assets/cover-catalog.txt","rb");assert(f);char *catalog=calloc(1,256*1024);assert(catalog);assert(fread(catalog,1,256*1024-1,f)>0);fclose(f);
    const char *roms[]={"007 - GoldenEye (USA).n64","Legend of Zelda, The - Ocarina of Time (E) (GC) [!].z64","Legend of Zelda, The - Majora's Mask - Collector's Edition (E) (GC) [!].z64","Mario Kart 64 (E) (V1.0) [!].z64","Mario Tennis (E) (VC).z64","Resident Evil 2 (USA) (Rev A).n64","Super Mario 64 (Europe) (En,Fr,De).n64"};
    for(unsigned i=0;i<sizeof(roms)/sizeof(*roms);i++){char file[256],url[1024];bool ok=cover_match(catalog,roms[i],file,sizeof(file));printf("%s -> %s\n",roms[i],ok?file:"unmatched");assert(ok);if(ok){assert(cover_url(file,url,sizeof(url)));assert(!strchr(url,' '));assert(!strstr(url,"%0D"));assert(strstr(file,i==0||i==5?"(USA)":"(Europe)"));assert(!strstr(file,"(Debug)"));}}
    char result[256];cover_title("Legend of Zelda, The (E) [!].n64",result,sizeof(result));assert(!strcmp(result,"The Legend of Zelda"));
    assert(!cover_match(catalog,"Totally Unknown Game (USA).n64",result,sizeof(result)));assert(!cover_url("../escape.png",result,sizeof(result)));assert(!cover_url("folder/file.png",result,sizeof(result)));assert(!cover_url("file.png",result,2));
    char a[17],b[17];cover_key("Mario.n64",a);cover_key("Mario.n64",b);assert(!strcmp(a,b));cover_key("Mario.z64",b);assert(strcmp(a,b));free(catalog);puts("PASS: cover titles, legacy names, regions, URL escaping, unknown games and stable cache keys");
}

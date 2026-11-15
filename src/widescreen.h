/* Optional per-revision projection patches; no renderer changes. GPL-2.0-or-later */
#ifndef NPS64_WIDESCREEN_H
#define NPS64_WIDESCREEN_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {uint32_t crc1,crc2;unsigned bytes;const char *name,*code;} Nps64WideProfile;
/* Source URLs and validation scope are recorded in docs/WIDESCREEN-00.36.md. */
static const Nps64WideProfile nps64_wide_profiles[]={
 {0x635a2bff,0x8b022326,8*1024*1024,"Super Mario 64 (USA)",
  "8127D55E 47E0 8127BD20 3C07 8127BD22 3FE3 D11A3E40 8FA7 811A3E40 3C07 D11A3E42 0054 811A3E42 3FE3"},
 {0x916b8b5b,0x780b85a4,16*1024*1024,"Super Smash Bros. (USA)",
  "8110D288 3C07 8110D28A 3FE8 810D0F64 3C07 810D0F66 3FE8 8103B884 3FE3"}
};
static inline uint32_t nps64_wide_be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline const Nps64WideProfile *nps64_wide_header(const unsigned char *header,size_t size,long bytes){
 if(size<64)return NULL;unsigned char b[64];memcpy(b,header,64);
 if(!memcmp(b,"\x37\x80\x40\x12",4)){for(unsigned i=0;i<64;i+=2){unsigned char t=b[i];b[i]=b[i+1];b[i+1]=t;}}
 else if(!memcmp(b,"\x40\x12\x37\x80",4)){for(unsigned i=0;i<64;i+=4){unsigned char a=b[i],v=b[i+1];b[i]=b[i+3];b[i+1]=b[i+2];b[i+2]=v;b[i+3]=a;}}
 if(memcmp(b,"\x80\x37\x12\x40",4)||b[62]!=0x45||b[63]!=0)return NULL;
 for(unsigned i=0;i<sizeof(nps64_wide_profiles)/sizeof(*nps64_wide_profiles);i++){
  const Nps64WideProfile *p=&nps64_wide_profiles[i];
  if(bytes==(long)p->bytes&&nps64_wide_be32(b+16)==p->crc1&&nps64_wide_be32(b+20)==p->crc2)return p;
 }
 return NULL;
}
static inline const Nps64WideProfile *nps64_wide_probe(const char *path){
 FILE*f=fopen(path,"rb");if(!f)return NULL;unsigned char b[64];size_t n=fread(b,1,64,f);
 long bytes=fseek(f,0,SEEK_END)?-1:ftell(f);fclose(f);return nps64_wide_header(b,n,bytes);
}
#endif

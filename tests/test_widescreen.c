#include "widescreen.h"
#include "cheats.h"
#include <assert.h>
int main(void){
 unsigned char header[64]={0x80,0x37,0x12,0x40};header[62]=0x45;
 for(unsigned p=0;p<2;p++){
  const Nps64WideProfile *profile=&nps64_wide_profiles[p];
  uint32_t words[]={profile->crc1,profile->crc2};
  for(unsigned w=0;w<2;w++)for(unsigned i=0;i<4;i++)header[16+4*w+i]=(unsigned char)(words[w]>>(24-8*i));
  assert(nps64_cheat_code_valid(profile->code));
  assert(nps64_wide_header(header,64,profile->bytes)==profile);
  assert(!nps64_wide_header(header,63,profile->bytes));
  assert(!nps64_wide_header(header,64,profile->bytes+1));
  header[62]=0x50;assert(!nps64_wide_header(header,64,profile->bytes));header[62]=0x45;
  header[63]=1;assert(!nps64_wide_header(header,64,profile->bytes));header[63]=0;
  unsigned char swapped[64];
  for(unsigned i=0;i<64;i++)swapped[i]=header[i^1];
  assert(nps64_wide_header(swapped,64,profile->bytes)==profile);
  for(unsigned i=0;i<64;i++)swapped[i]=header[i^3];
  assert(nps64_wide_header(swapped,64,profile->bytes)==profile);
  header[16]^=1;assert(!nps64_wide_header(header,64,profile->bytes));header[16]^=1;
 }
 assert(!nps64_wide_probe("/missing-rom.z64"));
 puts("PASS widescreen profiles: CRC, region, revision, size, byte orders and bounded patch syntax");
}

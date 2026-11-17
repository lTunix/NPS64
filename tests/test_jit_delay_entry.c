#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "jit_delay_entry_generated.h"
int main(void){
 uint64_t branch_unneeded_reg[3]={0x1235,0x9877,0xffff};
 uint64_t branch_unneeded_reg_upper[3]={0xabcd,0x8765,0xffff};
 uint64_t unneeded_reg[4]={0,0xffff,0x3333,0x7777};
 uint64_t unneeded_reg_upper[4]={0,0xffff,0x5555,0xaaaa};
 for(int slen=1;slen<=3;slen++)for(int i=0;i<slen;i++){
  struct masks current={0};
  NPS64_DELAY_MASKS;
  assert(current.u==(i==0?1:(i+1<slen?branch_unneeded_reg[i-1]&unneeded_reg[i+1]:branch_unneeded_reg[i-1])));
  assert(current.uu==(i==0?1:(i+1<slen?branch_unneeded_reg_upper[i-1]&unneeded_reg_upper[i+1]:branch_unneeded_reg_upper[i-1])));
 }
 puts("PASS delay-slot liveness: first-entry redzones, ordinary slots and end-of-block masks");
}

#include "command_ring.hpp"
#include "rdp_device.hpp"
#include <cassert>
#include <chrono>
#include <thread>
#include <cstdio>
namespace RDP {
void CommandProcessor::enqueue_command_direct(unsigned count,const uint32_t *words){
 if(count==1&&words[0]==(uint32_t(Op::MetaIdle)<<24))return;
 assert(count==1+(expected*37)%100&&words[0]==(0x80000000u|expected));
 for(unsigned j=1;j<count;j++)assert(words[j]==(expected*1634147u+j*293u));
 if(expected%257==0)std::this_thread::sleep_for(std::chrono::microseconds(50));
 expected++;
}
}
int main(){
 RDP::CommandProcessor processor;
 {RDP::CommandRing ring;
 for(unsigned phase=0;phase<2;phase++){
  assert(ring.init(&processor,phase?65536:128));
  for(unsigned i=phase*20000;i<(phase+1)*20000;i++){
   uint32_t words[100];unsigned n=1+(i*37)%100;words[0]=0x80000000u|i;
   for(unsigned j=1;j<n;j++)words[j]=i*1634147u+j*293u;
   ring.enqueue_command(n,words);
   if(i%601==0){ring.drain();assert(processor.expected==i+1);}
  }
  ring.drain();assert(processor.expected==(phase+1)*20000);
 }
 }
 assert(processor.expected==40000);
 puts("PASS actual command ring: 40000 ordered payloads, circular wrap, backpressure, >64-word commands, drain, reinit and teardown at 128/65536 words");
}

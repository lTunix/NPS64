/* Vulkan HLE must service a streaming-first ROM without a prior ProcessDList. */
#include "Gfx #1.3.h"
#include "rdp_emit_backend.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
extern "C" int parallelInitiateGFX(GFX_INFO);
extern "C" int parallelRomOpen(void);
GFX_INFO gfx_info{};
static const RdpEmitBackend *backend;
static unsigned oneshot_calls,submissions,redirects;
static unsigned char ram[16],dmem[16],commands[8];
static uint32_t start,current,end;
extern "C" void rdp_emit_set_backend(const RdpEmitBackend *value) { backend=value; }
extern "C" void rdp_emit_hle_process_dlist(void) { oneshot_calls++; }
namespace RDP {
void process_commands() {
    assert(redirects==1 && start==0x700000 && current==start && end==start+8);
    submissions++;
}
void set_hle_cmd_buffer(const uint8_t *p,uint32_t base,uint32_t length) {
    if(p) { assert(p==commands && base==0x700000 && length==8);redirects++; }
    else { assert(base==0 && length==0 && redirects==1);redirects--; }
}
}
static void check_callbacks() {
    assert(backend && backend->get_rdram && backend->get_dmem && backend->get_rdram_size && backend->submit);
    assert(backend->get_rdram()==ram && backend->get_dmem()==dmem);
    assert(backend->get_rdram_size()==0x800000 && oneshot_calls==0);
    backend->submit(commands,0x700000,8);
    assert(redirects==0);
}
int main() {
    gfx_info.RDRAM=ram;gfx_info.DMEM=dmem;
    gfx_info.DPC_START_REG=&start;gfx_info.DPC_CURRENT_REG=&current;gfx_info.DPC_END_REG=&end;
    assert(!backend);
    assert(parallelInitiateGFX(gfx_info));check_callbacks();
    /* A subsequent ROM must rebind even after another graphics backend. */
    backend=nullptr;
    assert(parallelRomOpen());check_callbacks();
    assert(submissions==2 && oneshot_calls==0);
    std::puts("PASS Vulkan HLE backend: cold connect, ROM reopen, memory callbacks and FIFO redirect without ProcessDList");
}

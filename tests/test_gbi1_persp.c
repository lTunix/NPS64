/* Exercise the real F3D walker: 0.95 keeps HALF_1, later GBI 1 uses MOVEWORD. */
#include "rdp_emit_f3d.h"
#include "rdp_emit_f3dex2.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Mid-list ucode swaps are outside this fixture. */
int s2dex1_ucode_match(const unsigned char *r, unsigned size, unsigned ud, unsigned ut)
{ (void)r; (void)size; (void)ud; (void)ut; return 0; }
void gsp_detect_ucode_params(GSPState *s, const unsigned char *r,
                             unsigned size, unsigned ud, unsigned ut)
{ (void)s; (void)r; (void)size; (void)ud; (void)ut; }

static void command(unsigned char *ram, unsigned at, uint32_t a, uint32_t b)
{ memcpy(ram + at, &a, 4); memcpy(ram + at + 4, &b, 4); }

static void check(int extended, int legacy)
{
    unsigned char ram[256] = {0};
    uint32_t storage[128];
    GSPState state;
    RdpFifo fifo;
    gsp_init(&state);
    state.nps64_culldl_legacy = legacy;
    gsp_set_persp_norm(&state, 65535);
    f3d_seg_reset(); f3d_set_rdram(ram); f3d_set_rdram_size(sizeof(ram));
    f3d_set_variant(extended); f3d_set_variant_f3dex(0);
    rdp_fifo_init(&fifo, (unsigned char *)storage, 0, sizeof(storage));
    command(ram, 64, 0xb4000000, 87);
    command(ram, 72, 0xb8000000, 0);
    f3d_run_dl(&state, &fifo, 64, 0, 0);
    assert(state.viewport.persp_norm == ((!extended || legacy) ? 87u : 65535u));

    /* A texrect consumes its HALF commands as coordinates, not perspNorm. */
    gsp_set_persp_norm(&state, 87);
    command(ram, 64, 0xe4004004, 0);
    command(ram, 72, 0xb3000000, 0x00200040);
    command(ram, 80, 0xb4000000, 0x04000400);
    command(ram, 88, 0xb8000000, 0);
    f3d_run_dl(&state, &fifo, 64, 0, 0);
    assert(state.viewport.persp_norm == 87);

    command(ram, 64, 0xbc00000e, 123);
    command(ram, 72, 0xb8000000, 0);
    f3d_run_dl(&state, &fifo, 64, 0, 0);
    assert(state.viewport.persp_norm == 123);
    f3d_set_rdram(NULL); f3d_set_rdram_size(0);
}

int main(void)
{
    check(1, 1); check(1, 0); check(0, 0);
    puts("PASS real F3D walker: 0.95 HALF_1, later GBI 1, Fast3D, texrect tails and MOVEWORD");
    return 0;
}

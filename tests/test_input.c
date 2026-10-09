#include "input_ps4.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    OrbisPadData p={0}; p.connected=1;
    p.leftStick.x=p.leftStick.y=p.rightStick.x=p.rightStick.y=128;
    const struct { uint32_t raw,menu; uint16_t joy; } cases[]={
        {0x4000,BTN_CONFIRM,1u<<0}, /* Cross */
        {0x2000,BTN_BACK,0},        /* Circle */
        {0x8000,BTN_SCAN,1u<<1},    /* Square */
        {0x1000,BTN_HELP,0},        /* Triangle */
        {0x0008,BTN_EXIT,1u<<3},    /* Options / N64 Start */
        {0x0002,0,0}, {0x0004,0,0}, {0x0006,BTN_PAUSE,0},
        {0x0100,0,1u<<12}, {0x0400,BTN_PREV,1u<<10},
        {0x0800,BTN_NEXT,1u<<11}, {0x0010,BTN_UP,1u<<4},
        {0x0040,BTN_DOWN,1u<<5}, {0x0080,BTN_LEFT,1u<<6},
        {0x0020,BTN_RIGHT,1u<<7},
    };
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);i++) {
        p.buttons=cases[i].raw; Input in=input_ps4_decode(&p);
        assert(in.buttons==cases[i].menu && in.joy==cases[i].joy);
        assert(in.raw_buttons==p.buttons && in.connected && in.lx==0 && in.ry==0);
    }
    p.buttons=0; p.leftStick.x=0; p.rightStick.y=255; p.analogButtons.l2=101;
    Input in=input_ps4_decode(&p);
    assert(in.lx==-32768 && in.ry==32766 && in.joy==(1u<<12));
    p.connected=0; in=input_ps4_decode(&p);
    assert(!in.connected && !in.buttons && !in.joy && !in.raw_buttons);
    puts("PASS: ScePad face buttons, menu/N64 actions, chords, axes and disconnect");
    return 0;
}

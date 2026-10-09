#ifndef NPS64_INPUT_PS4_H
#define NPS64_INPUT_PS4_H
#include "platform.h"
#include <orbis/_types/pad.h>

static inline int16_t input_axis(uint8_t value) {
    int n=(int)value-128;
    if(n>-16 && n<16) return 0;
    return n<0 ? (int16_t)(n*256) : (int16_t)(n*258);
}
/* Shared by the PS4 backend and host tests using actual ScePad bit values. */
static inline Input input_ps4_decode(const OrbisPadData *p) {
    Input in={0};
    if(!p->connected) return in;
    in.connected=true; in.raw_buttons=p->buttons;
    in.lx=input_axis(p->leftStick.x); in.ly=input_axis(p->leftStick.y);
    in.rx=input_axis(p->rightStick.x); in.ry=input_axis(p->rightStick.y);
    uint32_t b=p->buttons;
    if(b&ORBIS_PAD_BUTTON_UP) in.buttons|=BTN_UP;
    if(b&ORBIS_PAD_BUTTON_DOWN) in.buttons|=BTN_DOWN;
    if(b&ORBIS_PAD_BUTTON_LEFT) in.buttons|=BTN_LEFT;
    if(b&ORBIS_PAD_BUTTON_RIGHT) in.buttons|=BTN_RIGHT;
    if(b&ORBIS_PAD_BUTTON_CROSS) in.buttons|=BTN_CONFIRM;
    if(b&ORBIS_PAD_BUTTON_CIRCLE) in.buttons|=BTN_BACK;
    if(b&ORBIS_PAD_BUTTON_SQUARE) in.buttons|=BTN_SCAN;
    if(b&ORBIS_PAD_BUTTON_TRIANGLE) in.buttons|=BTN_HELP;
    if(b&ORBIS_PAD_BUTTON_OPTIONS) in.buttons|=BTN_EXIT;
    if(b&ORBIS_PAD_BUTTON_L1) in.buttons|=BTN_PREV;
    if(b&ORBIS_PAD_BUTTON_R1) in.buttons|=BTN_NEXT;
    if((b&(ORBIS_PAD_BUTTON_L3|ORBIS_PAD_BUTTON_R3))==(ORBIS_PAD_BUTTON_L3|ORBIS_PAD_BUTTON_R3)) in.buttons|=BTN_PAUSE;
    /* Libretro B/Y correspond to N64 A/B with this core's default mapping. */
    if(b&ORBIS_PAD_BUTTON_CROSS) in.joy|=1u<<0;
    if(b&ORBIS_PAD_BUTTON_SQUARE) in.joy|=1u<<1;
    if(b&ORBIS_PAD_BUTTON_OPTIONS) in.joy|=1u<<3;
    if(b&ORBIS_PAD_BUTTON_UP) in.joy|=1u<<4;
    if(b&ORBIS_PAD_BUTTON_DOWN) in.joy|=1u<<5;
    if(b&ORBIS_PAD_BUTTON_LEFT) in.joy|=1u<<6;
    if(b&ORBIS_PAD_BUTTON_RIGHT) in.joy|=1u<<7;
    if(b&ORBIS_PAD_BUTTON_L1) in.joy|=1u<<10;
    if(b&ORBIS_PAD_BUTTON_R1) in.joy|=1u<<11;
    if((b&ORBIS_PAD_BUTTON_L2) || p->analogButtons.l2>100) in.joy|=1u<<12;
    return in;
}
#endif

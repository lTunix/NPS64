/* Exercise the actual Ari64 MI-store handler with isolated MMIO state. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define MI_INIT_MODE_REG 0
struct mi {uint32_t regs[1];};
struct cpu {struct mi *mi;unsigned delay_slot;};
struct hot {uint32_t pcaddr,address,wword;unsigned pending_exception;};
static struct mi mi;
static struct cpu cpu={.mi=&mi};
static struct hot hot;
static struct cpu *r4300=&cpu;
static struct hot *state=&hot;
static int mi_window_pending;
static void r4300_write_aligned_word(struct cpu *c,uint32_t address,uint32_t value,uint32_t mask){
    assert(mask==UINT32_MAX);
    if((address&0x1fffffff)!=0x04300000)return;
    uint32_t *mode=&c->mi->regs[0];*mode=(*mode&~0x7f)|(value&0x7f);
    if(value&0x80)*mode&=~0x80;if(value&0x100)*mode|=0x80;
    if(value&0x200)*mode&=~0x100;if(value&0x400)*mode|=0x100;
}
#define UPDATE_COUNT_IN (void)count;
#define UPDATE_COUNT_OUT
#include "jit_mi_store_generated.h"
static void check(uint32_t address,uint32_t write,uint32_t initial,unsigned delay,unsigned exception,int expected){
    mi.regs[0]=initial;hot=(struct hot){.address=address,.wword=write,.pending_exception=exception};mi_window_pending=0;
    write_word_new((int)(0x80000200|delay),0);
    assert(mi_window_pending==expected);assert(hot.pending_exception==(exception||expected));
}
int main(void){
    /* IPL3 abandons a window at a branch; later CLEAR_DP must not revive it. */
    check(0xa4300000,0x800,0x80,0,0,0);
    check(0xa4300000,0x800,0x100,0,0,0);
    check(0xa4300000,0x0,0x180,0,0,0);
    check(0xa4300000,0x10f,0,0,0,1); /* explicit init-repeat still works */
    check(0xa4300000,0x400,0,0,0,1); /* explicit EBUS still works */
    check(0x84300000,0x400,0,0,0,1); /* cached address alias */
    check(0xa4300000,0x80,0x80,0,0,0);
    check(0xa4300000,0x200,0x100,0,0,0);
    check(0xa4300000,0x100,0,1,0,0);
    check(0xa4300000,0x400,0,0,1,0);
    check(0xa4400000,0x500,0x180,0,0,0);
    puts("PASS Ari64 MI stores: stale modes, explicit requests, aliases, clears, delay slots and existing exceptions");
}

/* Fatal-signal diagnostics for the isolated test title.
 * PS4 ABI offsets follow DolphinPS4 port/ps4_crashlog.cpp (69360a1).
 * SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "gpu_crashlog.h"
#if defined(__ORBIS__) || defined(NPS64_CRASHLOG_HOST_TEST)
#include "log.h"
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __ORBIS__
#include <orbis/libkernel.h>
extern char __text_start[];
#else
#include <ucontext.h>
extern char __ehdr_start[];
#endif
static int crash_fd=-1;
static char *literal(char *p,const char *s){while(*s)*p++=*s++;return p;}
static char *hex(char *p,uint64_t value) {
    static const char digits[]="0123456789abcdef";
    p=literal(p,"0x");int shift=60;while(shift && !(value>>shift))shift-=4;
    for(;shift>=0;shift-=4)*p++=digits[(value>>shift)&15];return p;
}
static char *field(char *p,const char *name,uint64_t value){p=literal(p,name);p=hex(p,value);return literal(p,"\n");}
static void crash_handler(int signal,siginfo_t *info,void *context) {
    /* Preopened descriptor, bounded stack buffer, no heap/stdio/log lock.
     * SA_RESETHAND restores the fatal default before re-executing the fault. */
    char line[1536],*p=line;
    uint64_t rip=0,rsp=0,address=0,base;
    p=literal(p,"\n[gpu-crash] fatal signal (hex)\n");p=field(p,"signal=",(unsigned)signal);
#ifdef __ORBIS__
    base=(uintptr_t)__text_start;
    const uint64_t *regs=(const void*)((const char*)context+0x40);
    rip=regs[20];rsp=regs[23];
    memcpy(&address,(const char*)info+24,sizeof(address));
    static const char *names[]={"rdi=","rsi=","rdx=","rcx=","r8=","r9=","rax=","rbx=","rbp=","r10=","r11=","r12=","r13=","r14=","r15="};
    for(unsigned i=0;i<15;i++)p=field(p,names[i],regs[1+i]);
#else
    base=(uintptr_t)__ehdr_start;
    const ucontext_t *uc=context;
    rip=uc->uc_mcontext.gregs[REG_RIP];rsp=uc->uc_mcontext.gregs[REG_RSP];address=(uintptr_t)info->si_addr;
#endif
    p=field(p,"address=",address);p=field(p,"rip=",rip);p=field(p,"rsp=",rsp);p=field(p,"text_base=",base);
    if(rip>=base && rip-base<0x4000000)p=field(p,"rip_elf=",rip-base);
#ifdef __ORBIS__
    OrbisKernelVirtualQueryInfo mapping;
    if(!sceKernelVirtualQuery((void*)(uintptr_t)rsp,0,&mapping,sizeof(mapping))) {
        uintptr_t end=(uintptr_t)mapping.unk02;
        if(rsp>=(uintptr_t)mapping.unk01 && rsp<end) {
            const volatile uint64_t *stack=(const void*)(uintptr_t)rsp;
            size_t words=(end-rsp)/8;if(words>16)words=16;
            p=literal(p,"stack_words (candidates, not a backtrace):\n");
            for(size_t i=0;i<words;i++){p=hex(p,stack[i]);p=literal(p," ");}
            p=literal(p,"\n");
        }
    }
#endif
    size_t left=(size_t)(p-line),offset=0;
    while(left) {
#ifdef __ORBIS__
        size_t count=sceKernelWrite(crash_fd,line+offset,left);
        if(!count || count>left)break;
#else
        ssize_t result=write(crash_fd,line+offset,left);if(result<=0)break;size_t count=(size_t)result;
#endif
        offset+=count;left-=count;
    }
#ifdef __ORBIS__
    sceKernelFsync(crash_fd);
#endif
    /* A breakpoint trap resumes after its instruction. Queue the restored
     * default action as well, so even traps cannot continue after capture. */
    raise(signal);
}
bool nps64_gpu_crashlog_init(const char *path) {
    if(crash_fd>=0)return true;
#ifdef __ORBIS__
    crash_fd=sceKernelOpen(path,O_WRONLY|O_CREAT|O_APPEND,0666);
    const int signals[]={4,5,6,8,10,11,12};const int flags=0x40|0x4; /* FreeBSD SIGINFO | RESETHAND */
#else
    crash_fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0666);
    const int signals[]={SIGILL,SIGTRAP,SIGABRT,SIGFPE,SIGBUS,SIGSEGV,SIGSYS};const int flags=SA_SIGINFO|SA_RESETHAND;
#endif
    if(crash_fd<0)return false;
    struct sigaction action;memset(&action,0,sizeof(action));
    void (*handler)(int,siginfo_t*,void*)=crash_handler;memcpy(&action,&handler,sizeof(handler));
    action.sa_flags=flags;sigemptyset(&action.sa_mask);
    unsigned installed=0;
    for(unsigned i=0;i<sizeof(signals)/sizeof(*signals);i++)if(!sigaction(signals[i],&action,NULL))installed++;
    unsigned count=sizeof(signals)/sizeof(*signals);
    nps64_log("[gpu-crash] handlers=%u/%u; separate preopened log; fatal action preserved\n",installed,count);
    return installed==count;
}
#else
bool nps64_gpu_crashlog_init(const char *path){(void)path;return true;}
#endif

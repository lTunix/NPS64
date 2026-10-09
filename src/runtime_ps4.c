/* PS4 pthread_once ABI workaround, adapted from DolphinPS4 ps4_runtime.cpp.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "log.h"
int32_t sceKernelMprotect(const void *,size_t,int);
/* BSS code cache stays close to the ELF helpers (x64 rel32 branches).
 * PS4 protects 16 KiB pages, while the upstream buffer is 4 KiB aligned. */
int __wrap_mprotect(void *address,size_t length,int protection) {
    const uintptr_t page=0x4000,start=(uintptr_t)address&~(page-1);
    if(!length || (uintptr_t)address>UINTPTR_MAX-(page-1) ||
       length>UINTPTR_MAX-(page-1)-(uintptr_t)address) {errno=EINVAL;return -1;}
    uintptr_t end=((uintptr_t)address+length+page-1)&~(page-1);
    int32_t result=sceKernelMprotect((void*)start,end-start,protection&7);
    if(result) {
        nps64_log("[jit] mprotect address=%p length=%zu prot=%d error=0x%08x\n",(void*)start,(size_t)(end-start),protection,(unsigned)result);
        errno=EACCES;return -1;
    }
    return 0;
}
int __wrap_pthread_once(pthread_once_t *once,void (*init)(void)) {
    int *state=(int*)once;
    if(__atomic_load_n(state,__ATOMIC_ACQUIRE)==2) return 0;
    int expected=0;
    if(__atomic_compare_exchange_n(state,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
        init(); __atomic_store_n(state,2,__ATOMIC_RELEASE); return 0;
    }
    while(__atomic_load_n(state,__ATOMIC_ACQUIRE)!=2) sched_yield();
    return 0;
}

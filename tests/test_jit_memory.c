#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
int __wrap_mprotect(void *,size_t,int);
static uintptr_t got_address;
static size_t got_length;
static int got_protection, calls, denied;
int32_t sceKernelMprotect(const void *address,size_t length,int protection) {
    got_address=(uintptr_t)address;got_length=length;got_protection=protection;calls++;
    return denied ? (int32_t)0x8002000d : 0;
}
void nps64_log(const char *format,...) {(void)format;}
int main(void) {
    assert(__wrap_mprotect((void*)0x12345000,0x2000000,7)==0);
    assert(got_address==0x12344000 && got_length==0x2004000 && got_protection==7);
    assert(__wrap_mprotect((void*)0x20000000,0x4000,3)==0);
    assert(got_length==0x4000 && got_protection==3);
    denied=1;
    assert(__wrap_mprotect((void*)0x1000,0x4000,7)==-1 && errno==EACCES);
    int before=calls;
    assert(__wrap_mprotect((void*)(UINTPTR_MAX-10),100,7)==-1 && errno==EINVAL);
    assert(__wrap_mprotect((void*)0x1000,SIZE_MAX,7)==-1);
    assert(__wrap_mprotect((void*)0x1000,0,7)==-1 && calls==before);
    puts("PASS: JIT PS4 page alignment, permissions, denial and overflow");
}

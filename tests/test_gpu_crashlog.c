#define _GNU_SOURCE
#include "gpu_crashlog.h"
#include <assert.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
int main(void) {
    assert(!nps64_gpu_crashlog_init("build/missing-crashlog-directory/crash.log"));
    const char *path="build/test-gpu-crash.log";unlink(path);
    for(unsigned test=0;test<2;test++) {
    pid_t child=fork();assert(child>=0);
    if(!child) {
        struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));
        assert(nps64_gpu_crashlog_init(path));
        if(test)__asm__ volatile("int3");
        else *(volatile unsigned char*)(uintptr_t)1=1;
        _exit(90);
    }
    int status;assert(waitpid(child,&status,0)==child);
    assert(WIFSIGNALED(status) && WTERMSIG(status)==(test?SIGTRAP:SIGSEGV));
    }
    char text[4096]={0};FILE *f=fopen(path,"rb");assert(f);
    assert(fread(text,1,sizeof(text)-1,f)>0);fclose(f);
    assert(strstr(text,"signal=0xb\n") && strstr(text,"address=0x1\n") && strstr(text,"rip_elf=0x"));
    assert(strstr(text,"signal=0x5\n"));
    puts("PASS fatal capture: real SIGSEGV and CPU breakpoint, fault/ELF PC recorded; both retain fatal termination");
}

/* Prepare capture storage before starting any GPU work. */
#pragma once
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
static inline int nps64_gpu_capture_prepare(const char *root) {
    const char *suffixes[]={"/gpu-probe","/gpu-probe/flight"};
    char path[1024];
    for(unsigned i=0;i<2;i++) {
        int length=snprintf(path,sizeof(path),"%s%s",root,suffixes[i]);
        if(length<0 || (size_t)length>=sizeof(path))return 0;
        if(mkdir(path,0777) && errno!=EEXIST)return 0;
        struct stat st;
        if(stat(path,&st) || !S_ISDIR(st.st_mode))return 0;
    }
    int length=snprintf(path,sizeof(path),"%s/gpu-probe/flight/.write-check",root);
    if(length<0 || (size_t)length>=sizeof(path))return 0;
    FILE *file=fopen(path,"wb");
    if(!file)return 0;
    int written=fputs("capture ready\n",file)>=0;
    int closed=fclose(file)==0;
    int removed=unlink(path)==0;
    return written && closed && removed;
}

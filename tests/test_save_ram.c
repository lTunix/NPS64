#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
int __real_stat(const char *,struct stat *);
int __wrap_stat(const char *p,struct stat *st){int r=__real_stat(p,st);if(!r)st->st_size=0;return r;}
int main(int argc,char **argv){
 const size_t n=296960;unsigned char *src=malloc(n+1),*dst=malloc(n);assert(src&&dst);
 for(size_t i=0;i<n+1;i++)src[i]=(unsigned char)(i*37+11);
 const char *path="build/test-save-ram.srm";unlink(path);memset(dst,0xa5,n);
 assert(save_ram_load(path,dst,n)==NPS64_SAVE_MISSING&&dst[0]==0xa5);
 assert(file_write_atomic(path,src,n));struct stat st;assert(!stat(path,&st)&&st.st_size==0);
 assert(file_read_exact(path,dst,n)&&!memcmp(src,dst,n));
 assert(save_ram_load(path,dst,n)==NPS64_SAVE_LOADED&&!memcmp(src,dst,n));
 assert(file_write_atomic(path,src,n-1));memset(dst,0xa5,n);
 assert(save_ram_load(path,dst,n)==NPS64_SAVE_INVALID);for(size_t i=0;i<n;i++)assert(dst[i]==0xa5);
 assert(file_write_atomic(path,src,n+1));assert(save_ram_load(path,dst,n)==NPS64_SAVE_INVALID);
 for(size_t i=0;i<n;i++)assert(dst[i]==0xa5);
 assert(!file_read_exact(path,dst,n));assert(file_write_atomic(path,src,0));
 assert(save_ram_load(path,dst,n)==NPS64_SAVE_INVALID);
 for(int i=1;i<argc;i++){assert(save_ram_load(argv[i],dst,n)==NPS64_SAVE_LOADED);}
 unlink(path);free(src);free(dst);puts("PASS SRAM: wrong stat size, exact/truncated/oversized/empty/missing, transactional load and existing PS4 copies");return 0;
}

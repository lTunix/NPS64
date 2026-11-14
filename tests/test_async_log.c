#define _POSIX_C_SOURCE 200809L
#include "log.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static int hold_io,entered,fail_write,fail_thread;
static size_t partial;
ssize_t __real_write(int,const void*,size_t);
int __real_pthread_create(pthread_t*,const pthread_attr_t*,void*(*)(void*),void*);
static void nap(void){struct timespec t={0,1000000};nanosleep(&t,NULL);}
ssize_t __wrap_write(int fd,const void *p,size_t n){
 __atomic_store_n(&entered,1,__ATOMIC_RELEASE);
 while(__atomic_load_n(&hold_io,__ATOMIC_ACQUIRE))nap();
 if(__atomic_load_n(&fail_write,__ATOMIC_RELAXED))return 0;
 if(partial&&n>partial)n=partial;
 return __real_write(fd,p,n);
}
int __wrap_pthread_create(pthread_t *t,const pthread_attr_t *a,void *(*f)(void*),void *p){
 return fail_thread?11:__real_pthread_create(t,a,f,p);
}
static void wait_entered(void){for(unsigned i=0;i<5000;i++){if(__atomic_load_n(&entered,__ATOMIC_ACQUIRE))return;nap();}assert(0);}
static void *producer(void *p){unsigned id=(unsigned)(uintptr_t)p;for(unsigned i=0;i<5000;i++)nps64_log("P%u %u\n",id,i);return NULL;}
int main(void){
 char root[]="/tmp/nps-log-XXXXXX";assert(mkdtemp(root));char path[256];snprintf(path,sizeof(path),"%s/log",root);
 assert(nps64_log_init(path));__atomic_store_n(&hold_io,1,__ATOMIC_RELEASE);nps64_log("head\n");wait_entered();
 /* The writer is definitely blocked in the filesystem. Producers must return. */
 for(unsigned i=0;i<1000;i++)nps64_log("M%u\n",i);
 assert(nps64_log_dropped()>0);unsigned drops=nps64_log_dropped();
 __atomic_store_n(&hold_io,0,__ATOMIC_RELEASE);nps64_log_close();
 FILE *f=fopen(path,"rb");assert(f);char line[256];unsigned count=0;
 while(fgets(line,sizeof(line),f))if(strncmp(line,"[log-queue]",11))count++;
 fclose(f);assert(count==1001-drops);unlink(path);
 assert(nps64_log_init(path));pthread_t threads[4];
 for(unsigned i=0;i<4;i++)assert(!pthread_create(&threads[i],NULL,producer,(void*)(uintptr_t)i));
 for(unsigned i=0;i<4;i++)pthread_join(threads[i],NULL);
 nps64_log_close();drops=nps64_log_dropped();
 f=fopen(path,"rb");assert(f);int last[4]={-1,-1,-1,-1};count=0;
 while(fgets(line,sizeof(line),f)){
  if(!strncmp(line,"[log-queue]",11))continue;
  unsigned id,n;int end=0;assert(sscanf(line,"P%u %u%n",&id,&n,&end)==2&&id<4&&n<5000&&!strcmp(line+end,"\n"));
  assert((int)n>last[id]);last[id]=(int)n;count++;
 }
 fclose(f);assert(count+drops==20000);unlink(path);
 partial=7;assert(nps64_log_init(path));nps64_log("partial %u %s\n",42,"preserved");nps64_log_close();
 f=fopen(path,"rb");assert(f&&fgets(line,sizeof(line),f));assert(!strcmp(line,"partial 42 preserved\n"));fclose(f);unlink(path);partial=0;
 assert(nps64_log_init(path));__atomic_store_n(&fail_write,1,__ATOMIC_RELAXED);nps64_log("fail\n");
 for(unsigned i=0;i<5000&&nps64_log_ok();i++)nap();assert(!nps64_log_ok());nps64_log_close();unlink(path);fail_write=0;
 fail_thread=1;assert(nps64_log_init(path));nps64_log("fallback\n");assert(nps64_log_ok());nps64_log_close();
 f=fopen(path,"rb");assert(f&&fgets(line,sizeof(line),f));assert(!strcmp(line,"fallback\n"));fclose(f);unlink(path);fail_thread=0;
 assert(!nps64_log_init("/no-such-nps-directory/file"));nps64_log_close();rmdir(root);
 puts("PASS async log: blocked filesystem cannot block producers, bounded overflow accounting, 4 producers/20000 records, ordering, partial writes, failure, drain/reopen and synchronous fallback");
}

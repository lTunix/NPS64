#include "rdp_worker.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
struct job {mpsc_stack_node_t node;unsigned id;uint64_t demand;};
struct context {pthread_mutex_t lock;pthread_cond_t ready;int start,entered,release;unsigned count,order[5];};
static uint64_t priority(void *opaque,const mpsc_stack_node_t *item){(void)opaque;const struct job *j=(const struct job*)item;return __atomic_load_n(&j->demand,__ATOMIC_RELAXED);}
static void start(void *opaque){struct context*c=opaque;pthread_mutex_lock(&c->lock);while(!c->start)pthread_cond_wait(&c->ready,&c->lock);pthread_mutex_unlock(&c->lock);}
static void work(void *opaque,mpsc_stack_node_t *item){struct context*c=opaque;struct job*j=(struct job*)item;pthread_mutex_lock(&c->lock);if(!j->id){c->entered=1;pthread_cond_broadcast(&c->ready);while(!c->release)pthread_cond_wait(&c->ready,&c->lock);}c->order[c->count++]=j->id;pthread_mutex_unlock(&c->lock);}
static bool done(void *opaque){struct context*c=opaque;pthread_mutex_lock(&c->lock);int result=c->count==5;pthread_mutex_unlock(&c->lock);return result;}
static void run(int prioritized){
 struct context c={.lock=PTHREAD_MUTEX_INITIALIZER,.ready=PTHREAD_COND_INITIALIZER};
 struct job jobs[5]={0};for(unsigned i=0;i<5;i++){jobs[i].id=i;jobs[i].demand=10-i;}jobs[0].demand=100;
 rdp_worker_t*w=prioritized?rdp_worker_new_prioritized(start,work,&c,priority):rdp_worker_new(start,work,&c);assert(w);
 for(unsigned i=0;i<4;i++)rdp_worker_push(w,&jobs[i].node);
 pthread_mutex_lock(&c.lock);c.start=1;pthread_cond_broadcast(&c.ready);while(!c.entered)pthread_cond_wait(&c.ready,&c.lock);pthread_mutex_unlock(&c.lock);
 /* Reprioritize an old queued item and add a newer item while a batch is active. */
 __atomic_store_n(&jobs[1].demand,200,__ATOMIC_RELAXED);jobs[4].demand=300;rdp_worker_push(w,&jobs[4].node);
 pthread_mutex_lock(&c.lock);c.release=1;pthread_cond_broadcast(&c.ready);pthread_mutex_unlock(&c.lock);
 rdp_worker_wait(w,done,&c);rdp_worker_free(w);
 const unsigned fifo[]={0,1,2,3,4},demand[]={0,4,1,2,3};assert(!memcmp(c.order,prioritized?demand:fifo,sizeof(fifo)));
 pthread_cond_destroy(&c.ready);pthread_mutex_destroy(&c.lock);
}
static void drain(void){struct context c={.lock=PTHREAD_MUTEX_INITIALIZER,.ready=PTHREAD_COND_INITIALIZER,.start=1,.release=1};struct job jobs[5]={0};rdp_worker_t*w=rdp_worker_new_prioritized(NULL,work,&c,priority);assert(w);for(unsigned i=0;i<5;i++){jobs[i].id=i;rdp_worker_push(w,&jobs[i].node);}rdp_worker_free(w);assert(c.count==5);pthread_cond_destroy(&c.ready);pthread_mutex_destroy(&c.lock);}
int main(void){for(unsigned i=0;i<50;i++){run(0);run(1);drain();}puts("PASS shader demand worker: FIFO isolation, queued promotion, new arrivals, wait notifications and complete shutdown drain");}

/* Background PNG cover cache. SPDX-License-Identifier: GPL-2.0-or-later */
#include "covers.h"
#include "i18n.h"
#include "cover_match.h"
#include "update_http.h"
#include "platform.h"
#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 2048
#include "third_party/stb_image.h"
#ifndef COVER_ROOT
#define COVER_ROOT "/data/NPS64/covers"
#endif
#define LIMIT (2*1024*1024)
typedef struct {uint32_t *pixels;int w,h,target_w,target_h;} View;
typedef struct {unsigned references;uint32_t *pixels;int w,h;View views[2];} CoverImage;
typedef struct {char name[256];CoverImage *image;uint64_t token,stamp;bool attempted;} Slot;
typedef struct {char name[256];uint64_t token;unsigned epoch;} Job;
static Slot slots[8];static Job queue[8],downloads[8];static unsigned count,download_count,epoch;
static uint64_t clock_id,next_token;
static char misses[128][17];static unsigned miss_count,miss_next;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready=PTHREAD_COND_INITIALIZER,network_ready=PTHREAD_COND_INITIALIZER;
static pthread_t thread,network_thread;static bool started,network_started,stop,paused;
static char *catalog;static char state[96]="Caratulas: disponibles sin conexion al descargarlas";
#ifndef COVER_CATALOG
#define COVER_CATALOG "/app0/cover-catalog.txt"
#endif
static void set_state(const char *text){pthread_mutex_lock(&mutex);snprintf(state,sizeof(state),"%s",text);pthread_mutex_unlock(&mutex);}
static unsigned char *read_png(const char *path,int *bytes) {
    FILE *f=fopen(path,"rb");if(!f)return NULL;unsigned char *data=malloc(LIMIT+1);if(!data){fclose(f);return NULL;}
    size_t n=fread(data,1,LIMIT+1,f);bool ok=!ferror(f)&&n>0&&n<=LIMIT;fclose(f);if(!ok){free(data);return NULL;}*bytes=(int)n;return data;
}
static uint32_t *decode(const unsigned char *data,int bytes,int *w,int *h) {
    int channels;if(!stbi_info_from_memory(data,bytes,w,h,&channels)||*w>1024||*h>1024||*w<=0||*h<=0)return NULL;
    unsigned char *rgba=stbi_load_from_memory(data,bytes,w,h,&channels,4);if(!rgba)return NULL;
    size_t total=(size_t)*w**h;uint32_t *pixels=malloc(total*4);if(pixels)for(size_t i=0;i<total;i++)pixels[i]=((uint32_t)rgba[4*i+3]<<24)|((uint32_t)rgba[4*i]<<16)|((uint32_t)rgba[4*i+1]<<8)|rgba[4*i+2];stbi_image_free(rgba);return pixels;
}

static bool stopping(void){pthread_mutex_lock(&mutex);bool value=stop;pthread_mutex_unlock(&mutex);return value;}
static void image_release(CoverImage *image){
    if(image&&__atomic_sub_fetch(&image->references,1,__ATOMIC_ACQ_REL)==0){
        for(unsigned i=0;i<2;i++)free(image->views[i].pixels);
        free(image->pixels);free(image);
    }
}
static void fit(int w,int h,int sw,int sh,int *dw,int *dh){
    *dw=w;*dh=(int)((int64_t)w*sh/sw);if(*dh>h){*dh=h;*dw=(int)((int64_t)h*sw/sh);}
    if(*dw<1)*dw=1;if(*dh<1)*dh=1;
}
static uint32_t sample(const CoverImage *image,int i,int j,int dw,int dh){
    unsigned sx=(unsigned)((uint64_t)i*(image->w-1)*256/(dw>1?dw-1:1)),sy=(unsigned)((uint64_t)j*(image->h-1)*256/(dh>1?dh-1:1));
    unsigned ix=sx>>8,iy=sy>>8,fx=sx&255,fy=sy&255;
    unsigned nx=ix+1<(unsigned)image->w?ix+1:ix,ny=iy+1<(unsigned)image->h?iy+1:iy;
    uint32_t a=image->pixels[iy*image->w+ix],b=image->pixels[iy*image->w+nx],c=image->pixels[ny*image->w+ix],d=image->pixels[ny*image->w+nx],color=0xff000000;
    for(unsigned shift=0;shift<24;shift+=8){unsigned top=((a>>shift&255)*(256-fx)+(b>>shift&255)*fx),bottom=((c>>shift&255)*(256-fx)+(d>>shift&255)*fx);color|=((top*(256-fy)+bottom*fy)>>16)<<shift;}
    return color;
}
static CoverImage *image_decode(const unsigned char *data,int bytes){
    int w=0,h=0;uint32_t *pixels=decode(data,bytes,&w,&h);if(!pixels)return NULL;
    CoverImage *image=calloc(1,sizeof(*image));if(!image){free(pixels);return NULL;}
    image->references=1;image->pixels=pixels;image->w=w;image->h=h;
    const int widths[]={396,78},heights[]={314,54};
    for(unsigned v=0;v<2;v++){
        View *view=&image->views[v];view->target_w=widths[v];view->target_h=heights[v];
        fit(widths[v],heights[v],w,h,&view->w,&view->h);
        view->pixels=malloc((size_t)view->w*view->h*4);if(!view->pixels)continue;
        for(int j=0;j<view->h;j++)for(int i=0;i<view->w;i++)view->pixels[j*view->w+i]=sample(image,i,j,view->w,view->h);
    }
    return image;
}
static CoverImage *load_local(const char *name){
    char key[17],path[1024],stem[256];int bytes=0;CoverImage *image=NULL;
    snprintf(stem,sizeof(stem),"%s",name);char *dot=strrchr(stem,'.');if(dot)*dot=0;
    if(!strchr(stem,'/')&&!strchr(stem,'\\')){
        snprintf(path,sizeof(path),COVER_ROOT "/%s.png",stem);unsigned char *data=read_png(path,&bytes);
        if(data){image=image_decode(data,bytes);free(data);}
    }
    if(!image){cover_key(name,key);snprintf(path,sizeof(path),COVER_ROOT "/%s.png",key);unsigned char *data=read_png(path,&bytes);if(data){image=image_decode(data,bytes);free(data);}}
    return image;
}
static Slot *job_slot(const Job *job){
    if(stop||job->epoch!=epoch)return NULL;
    for(unsigned i=0;i<8;i++)if(slots[i].token==job->token)return &slots[i];return NULL;
}
static void prune(Job jobs[8],unsigned *n){unsigned out=0;for(unsigned i=0;i<*n;i++)if(job_slot(&jobs[i]))jobs[out++]=jobs[i];*n=out;}
static bool missed(const char *name){char key[17];cover_key(name,key);for(unsigned i=0;i<miss_count;i++)if(!strcmp(misses[i],key))return true;return false;}
static void remember_miss(const char *name){if(missed(name))return;unsigned i=miss_count<128?miss_count++:miss_next++%128;cover_key(name,misses[i]);}
/* Takes ownership of image; callers never publish into a reused cache slot. */
static void finish(const Job *job,CoverImage *image){
    pthread_mutex_lock(&mutex);Slot *slot=job_slot(job);
    if(!stop&&job->epoch==epoch&&!image)remember_miss(job->name);
    if(slot){image_release(slot->image);slot->image=image;slot->attempted=true;image=NULL;
        snprintf(state,sizeof(state),"%s",slot->image?"Caratulas guardadas para usar sin conexion":"Caratula no disponible. Puedes agregar tu propio PNG.");}
    pthread_mutex_unlock(&mutex);image_release(image);
}
static bool pop_job(Job *job,bool network){
    Job *jobs=network?downloads:queue;unsigned *n=network?&download_count:&count;
    pthread_cond_t *condition=network?&network_ready:&ready;
    pthread_mutex_lock(&mutex);
    for(;;){
        while(!stop&&(paused||!*n))pthread_cond_wait(condition,&mutex);
        if(stop){pthread_mutex_unlock(&mutex);return false;}
        *job=jobs[0];memmove(jobs,jobs+1,(--*n)*sizeof(*jobs));
        if(job_slot(job)){pthread_mutex_unlock(&mutex);return true;}
    }
}
static void *worker(void *unused){
    (void)unused;Job job;
    while(pop_job(&job,false)){
        CoverImage *image=load_local(job.name);if(image){finish(&job,image);continue;}
        char filename[256];bool match=cover_match(catalog,job.name,filename,sizeof(filename));
        pthread_mutex_lock(&mutex);bool online=network_started&&match&&!missed(job.name)&&job_slot(&job);
        if(online){prune(downloads,&download_count);if(download_count<8){downloads[download_count++]=job;pthread_cond_signal(&network_ready);}else online=false;}
        pthread_mutex_unlock(&mutex);if(!online)finish(&job,NULL);
    }
    return NULL;
}
static void *network_worker(void *unused){
    (void)unused;Job job;
    while(pop_job(&job,true)){
        char filename[256],url[1024];CoverImage *image=NULL;
        if(cover_match(catalog,job.name,filename,sizeof(filename))&&cover_url(filename,url,sizeof(url))&&!stopping()){
            set_state("Descargando caratulas de Libretro...");Nps64Http http;int status=0;double deadline=platform_time()+6.;
            if(!nps64_http_init(&http)){
                if(!nps64_http_set_timeout(&http,3000000)&&!nps64_http_open(&http,url,&status)&&status==200){
                    unsigned char *data=malloc(LIMIT+1);size_t used=0;bool ok=data!=NULL;
                    while(ok&&!stopping()){
                        if(platform_time()>deadline){ok=false;break;}
                        int n=nps64_http_read(&http,data+used,LIMIT+1-used);
                        if(!n)break;if(n<0||(size_t)n>LIMIT-used){ok=false;break;}used+=(size_t)n;if(used==LIMIT){ok=false;break;}
                    }
                    if(ok&&!stopping()&&platform_time()<=deadline){
                        image=image_decode(data,(int)used);
                        if(image){char key[17],path[1024];cover_key(job.name,key);snprintf(path,sizeof(path),COVER_ROOT "/%s.png",key);file_write_atomic(path,data,used);}
                    }free(data);
                }
                nps64_http_term(&http);
            }
        }
        finish(&job,image);
    }
    return NULL;
}
void covers_init(void){
    if(started)return;mkdir(COVER_ROOT,0777);FILE *f=fopen(COVER_CATALOG,"rb");if(f){catalog=calloc(1,256*1024+1);if(catalog)fread(catalog,1,256*1024,f);fclose(f);}
    stop=false;paused=false;count=download_count=miss_count=miss_next=0;epoch++;
    if(!pthread_create(&network_thread,NULL,network_worker,NULL))network_started=true;
    if(!pthread_create(&thread,NULL,worker,NULL))started=true;
    else{pthread_mutex_lock(&mutex);stop=true;pthread_cond_broadcast(&network_ready);pthread_mutex_unlock(&mutex);
        if(network_started){pthread_join(network_thread,NULL);network_started=false;}free(catalog);catalog=NULL;}
}
void covers_request(const Game *game){
    if(!started)return;pthread_mutex_lock(&mutex);Slot *slot=NULL;
    for(unsigned i=0;i<8;i++)if(!strcmp(slots[i].name,game->name)){slots[i].stamp=++clock_id;pthread_mutex_unlock(&mutex);return;}
    for(unsigned i=0;i<8;i++)if(!slot||slots[i].stamp<slot->stamp)slot=&slots[i];
    image_release(slot->image);*slot=(Slot){.stamp=++clock_id,.token=++next_token};snprintf(slot->name,sizeof(slot->name),"%s",game->name);
    prune(queue,&count);prune(downloads,&download_count);
    if(count<8){Job *job=&queue[count++];snprintf(job->name,sizeof(job->name),"%s",game->name);job->token=slot->token;job->epoch=epoch;pthread_cond_signal(&ready);}
    pthread_mutex_unlock(&mutex);
}
void covers_status(char *out,size_t cap){pthread_mutex_lock(&mutex);snprintf(out,cap,"%s",nps64_tr(state));pthread_mutex_unlock(&mutex);}
void covers_set_paused(bool value){pthread_mutex_lock(&mutex);paused=value;if(!value){pthread_cond_broadcast(&ready);pthread_cond_broadcast(&network_ready);}pthread_mutex_unlock(&mutex);}
void covers_refresh(void){pthread_mutex_lock(&mutex);epoch++;count=download_count=miss_count=miss_next=0;for(unsigned i=0;i<8;i++){image_release(slots[i].image);slots[i]=(Slot){0};}pthread_mutex_unlock(&mutex);}
void covers_draw(const Game *game,int x,int y,int w,int h){
    if(w<=0||h<=0||w>SCREEN_W||h>SCREEN_H)return;
    pthread_mutex_lock(&mutex);CoverImage *image=NULL;
    for(unsigned i=0;i<8;i++)if(!strcmp(slots[i].name,game->name)&&slots[i].image){image=slots[i].image;__atomic_add_fetch(&image->references,1,__ATOMIC_RELAXED);break;}
    pthread_mutex_unlock(&mutex);if(!image)return;
    int dw,dh;fit(w,h,image->w,image->h,&dw,&dh);x+=(w-dw)/2;y+=(h-dh)/2;
    View *view=NULL;for(unsigned i=0;i<2;i++)if(image->views[i].target_w==w&&image->views[i].target_h==h&&image->views[i].pixels)view=&image->views[i];
    int left=x<0?-x:0,top=y<0?-y:0,right=dw,bottom=dh;
    if(x+right>SCREEN_W)right=SCREEN_W-x;if(y+bottom>SCREEN_H)bottom=SCREEN_H-y;
    if(left<right&&top<bottom){uint32_t *frame=platform_framebuffer();
        for(int j=top;j<bottom;j++){
            uint32_t *dst=frame+(y+j)*SCREEN_W+x+left;
            if(view)memcpy(dst,view->pixels+j*dw+left,(size_t)(right-left)*4);
            else for(int i=left;i<right;i++)dst[i-left]=sample(image,i,j,dw,dh);
        }
    }
    image_release(image);
}
void covers_shutdown(void){
    pthread_mutex_lock(&mutex);stop=true;pthread_cond_broadcast(&ready);pthread_cond_broadcast(&network_ready);pthread_mutex_unlock(&mutex);
    if(started){pthread_join(thread,NULL);started=false;}if(network_started){pthread_join(network_thread,NULL);network_started=false;}
    for(unsigned i=0;i<8;i++){image_release(slots[i].image);slots[i]=(Slot){0};}free(catalog);catalog=NULL;count=download_count=0;
}

/* Background PNG cover cache. SPDX-License-Identifier: GPL-2.0-or-later */
#include "covers.h"
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
typedef struct {char name[256];uint32_t *pixels;int w,h;unsigned stamp;bool attempted;} Slot;
static Slot slots[8];static char queue[8][256];static unsigned count,clock_id;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready=PTHREAD_COND_INITIALIZER;
static pthread_t thread;static bool started,stop;
static char *catalog;static char state[96]="Caratulas: disponibles sin conexion al descargarlas";
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
static void *worker(void *unused) {
    (void)unused;for(;;){char name[256];pthread_mutex_lock(&mutex);while(!stop&&!count)pthread_cond_wait(&ready,&mutex);if(stop){pthread_mutex_unlock(&mutex);break;}
        snprintf(name,sizeof(name),"%s",queue[0]);for(unsigned i=1;i<count;i++)memcpy(queue[i-1],queue[i],256);count--;pthread_mutex_unlock(&mutex);
        char key[17],path[1024],filename[256],url[1024];cover_key(name,key);snprintf(path,sizeof(path),COVER_ROOT "/%s.png",key);
        int bytes=0,w=0,h=0;unsigned char *data=NULL;uint32_t *pixels=NULL;
        /* A user-supplied PNG named after the ROM takes precedence over online lookup. */
        char stem[256];snprintf(stem,sizeof(stem),"%s",name);char *dot=strrchr(stem,'.');if(dot)*dot=0;
        if(!strchr(stem,'/')&&!strchr(stem,'\\')){snprintf(path,sizeof(path),COVER_ROOT "/%s.png",stem);data=read_png(path,&bytes);pixels=data?decode(data,bytes,&w,&h):NULL;free(data);}
        snprintf(path,sizeof(path),COVER_ROOT "/%s.png",key);
        if(!pixels){data=read_png(path,&bytes);pixels=data?decode(data,bytes,&w,&h):NULL;free(data);}
        if(!pixels&&!stopping()&&cover_match(catalog,name,filename,sizeof(filename))&&cover_url(filename,url,sizeof(url))) {
            set_state("Descargando caratulas de Libretro...");Nps64Http http;int status=0;
            if(!nps64_http_init(&http)) {
                if(!nps64_http_open(&http,url,&status)&&status==200){data=malloc(LIMIT+1);size_t used=0;bool ok=data!=NULL;
                    while(ok&&!stopping()){int n=nps64_http_read(&http,data+used,LIMIT+1-used);if(!n)break;if(n<0||(size_t)n>LIMIT-used){ok=false;break;}used+=(size_t)n;if(used==LIMIT){ok=false;break;}}
                    if(ok&&!stopping()){pixels=decode(data,(int)used,&w,&h);if(pixels)file_write_atomic(path,data,used);}free(data);
                }nps64_http_term(&http);
            }
        }
        pthread_mutex_lock(&mutex);Slot *slot=NULL;for(unsigned i=0;i<8;i++)if(!strcmp(slots[i].name,name)){slot=&slots[i];break;}
        if(slot){free(slot->pixels);slot->pixels=pixels;slot->w=w;slot->h=h;slot->attempted=true;}else free(pixels);
        snprintf(state,sizeof(state),"%s",pixels?"Caratulas guardadas para usar sin conexion":"Caratula no disponible. Puedes agregar tu propio PNG.");pthread_mutex_unlock(&mutex);
    }return NULL;
}
void covers_init(void) {
    mkdir(COVER_ROOT,0777);FILE *f=fopen("/app0/cover-catalog.txt","rb");if(f){catalog=calloc(1,256*1024+1);if(catalog)fread(catalog,1,256*1024,f);fclose(f);}
    stop=false;if(!pthread_create(&thread,NULL,worker,NULL))started=true;
}
void covers_request(const Game *game) {
    if(!started)return;pthread_mutex_lock(&mutex);Slot *slot=NULL;
    for(unsigned i=0;i<8;i++)if(!strcmp(slots[i].name,game->name)){slots[i].stamp=++clock_id;pthread_mutex_unlock(&mutex);return;}
    if(count==8){pthread_mutex_unlock(&mutex);return;}
    for(unsigned i=0;i<8;i++)if(!slot||slots[i].stamp<slot->stamp)slot=&slots[i];free(slot->pixels);*slot=(Slot){.stamp=++clock_id};snprintf(slot->name,sizeof(slot->name),"%s",game->name);
    snprintf(queue[count++],256,"%s",game->name);pthread_cond_signal(&ready);pthread_mutex_unlock(&mutex);
}
void covers_status(char *out,size_t cap){pthread_mutex_lock(&mutex);snprintf(out,cap,"%s",state);pthread_mutex_unlock(&mutex);}
void covers_refresh(void){pthread_mutex_lock(&mutex);count=0;for(unsigned i=0;i<8;i++){free(slots[i].pixels);slots[i]=(Slot){0};}pthread_mutex_unlock(&mutex);}
void covers_draw(const Game *game,int x,int y,int w,int h) {
    pthread_mutex_lock(&mutex);Slot *slot=NULL;for(unsigned i=0;i<8;i++)if(!strcmp(slots[i].name,game->name)&&slots[i].pixels){slot=&slots[i];break;}
    if(slot){int dw=w,dh=(int)((int64_t)w*slot->h/slot->w);if(dh>h){dh=h;dw=(int)((int64_t)h*slot->w/slot->h);}x+=(w-dw)/2;y+=(h-dh)/2;
        uint32_t *frame=platform_framebuffer();for(int j=0;j<dh;j++)for(int i=0;i<dw;i++)if(x+i>=0&&x+i<SCREEN_W&&y+j>=0&&y+j<SCREEN_H){
            unsigned sx=(unsigned)((uint64_t)i*(slot->w-1)*256/(dw>1?dw-1:1)),sy=(unsigned)((uint64_t)j*(slot->h-1)*256/(dh>1?dh-1:1));unsigned ix=sx>>8,iy=sy>>8,fx=sx&255,fy=sy&255;unsigned nx=ix+1<(unsigned)slot->w?ix+1:ix,ny=iy+1<(unsigned)slot->h?iy+1:iy;
            uint32_t a=slot->pixels[iy*slot->w+ix],b=slot->pixels[iy*slot->w+nx],c=slot->pixels[ny*slot->w+ix],d=slot->pixels[ny*slot->w+nx],color=0xff000000;
            for(unsigned shift=0;shift<24;shift+=8){unsigned top=((a>>shift&255)*(256-fx)+(b>>shift&255)*fx),bottom=((c>>shift&255)*(256-fx)+(d>>shift&255)*fx);color|=((top*(256-fy)+bottom*fy)>>16)<<shift;}frame[(y+j)*SCREEN_W+x+i]=color;
        }}pthread_mutex_unlock(&mutex);
}
void covers_shutdown(void){if(started){pthread_mutex_lock(&mutex);stop=true;pthread_cond_signal(&ready);pthread_mutex_unlock(&mutex);pthread_join(thread,NULL);started=false;}for(unsigned i=0;i<8;i++){free(slots[i].pixels);slots[i]=(Slot){0};}free(catalog);catalog=NULL;count=0;}

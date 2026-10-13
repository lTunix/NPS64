/* NPS64 PS4 frontend. SPDX-License-Identifier: GPL-2.0-or-later */
#include "platform.h"
#include "storage.h"
#include "log.h"
#include "gpu.h"
#include "cpu_game_profile.h"
#include "frame_pacing.h"
#include "version.h"
#ifdef NPS64_HD_UI
#include "library_ui.h"
#include "covers.h"
#endif
#ifdef NPS64_UPDATES
#include "updater.h"
#define PAUSE_CHOICES 7
#else
#define PAUSE_CHOICES 6
#endif
#ifdef NPS64_HLE_GAME
#include "hle_game_geometry.h"
#include "hle_game_material.h"
extern void nps64_gpu_static_init(void);
static bool hle_preview,hle_preview_used,hle_preview_displayed;
static bool hle_continuous,hle_frame_ready,hle_renderer_failed;
static HleRenderer *hle_renderer;
static unsigned long long hle_gpu_frames,hle_cpu_fallbacks,hle_repeated_frames,hle_incomplete_frames;
bool nps64_hle_is_live(void){return hle_continuous;}
bool nps64_hle_sequence_active;
static unsigned char hle_pixels[256*256*4];
#endif
#include <libretro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifndef DATA_ROOT
#define DATA_ROOT "/data/NPS64"
#endif
static const char games_dir[]=DATA_ROOT "/games", saves_dir[]=DATA_ROOT "/saves", system_dir[]=DATA_ROOT "/system";
static Game games[512],current;
static size_t game_count,selected;
static Input pads[4];
static bool running,paused,quit_requested,options_dirty,core_shutdown;
static unsigned slot,pause_selected,theme;
static bool widescreen;
static bool input_test;
#ifdef NPS64_CPU_RDP_PGO
static unsigned diagnostic_workers=6;
#else
static unsigned diagnostic_workers=4;
#endif
static bool diagnostic_hle=true;
static bool filtered_vi=true;
static bool use_jit,use_gpu,native_rgba;
static unsigned requested_gpu=1;
static unsigned requested_cpu=1; /* 0 cached interpreter, 1 checked Ari64 JIT */
extern int nps64_core_jit_supported(void);
extern unsigned nps64_core_workers(void);
extern void nps64_core_perf_take(double *,double *);
extern void nps64_core_game_profile(unsigned) __attribute__((weak));
double nps64_profile_time(void) { return platform_time(); }
static uint64_t core_frames;
static bool first_video;
static bool visible_video;
static double ui_fps,perf_poll_ms,perf_core_ms,perf_draw_ms,perf_flip_ms;
static double perf_rdp_ms,perf_vi_ms,perf_gpu_ms;
static bool ram_writes_allowed=true;
static enum retro_pixel_format pixel_format=RETRO_PIXEL_FORMAT_0RGB1555;
static struct retro_system_av_info av;
static char message[160]="Bienvenido. Copia tus ROMs en /data/NPS64/games";
static unsigned char *native_frame;
static size_t native_capacity;
static unsigned native_width,native_height;
static enum retro_pixel_format native_format;
static uint64_t video_frames;
static double game_fps,video_fps;
static double last_autosave;
static struct { char key[128],value[128]; } options[256];
static size_t option_count;
static const uint32_t accent[]={0xff48dac1,0xffa18aff,0xff62b4ff};
static void menu_draw(void);

static void notify(const char *text) { snprintf(message,sizeof(message),"%s",text); nps64_log("%s\n",text); }
static void core_log(enum retro_log_level level,const char *fmt,...) {
    va_list args; va_start(args,fmt); nps64_log("[core:%d] ",level); nps64_vlog(fmt,args); va_end(args);
}
static const char *override(const char *key) {
    if(!strcmp(key,"parallel-n64-cpucore")) return use_jit?"dynamic_recompiler_ari64":"cached_interpreter";
    if(!strcmp(key,"parallel-n64-parallel-rdp-synchronous")) return "enabled";
    if(!strcmp(key,"parallel-n64-gfxplugin")) return use_gpu?"parallel":"angrylion";
    if(!strcmp(key,"parallel-n64-rspplugin")) return diagnostic_hle?"hle":"cxd4";
    if(!strcmp(key,"parallel-n64-angrylion-multithread"))  {static char workers[4];if(!diagnostic_workers)return "off";snprintf(workers,sizeof(workers),"%u",diagnostic_workers);return workers;}
    if(!strcmp(key,"parallel-n64-angrylion-vioverlay")) return filtered_vi?"Filtered":"Unfiltered";
    if(!strcmp(key,"parallel-n64-parallel-rdp-vi-aa") || !strcmp(key,"parallel-n64-parallel-rdp-vi-bilinear")) return filtered_vi?"enabled":"disabled";
    if(!strcmp(key,"parallel-n64-upscaling")) return "1x";
    return NULL;
}
static bool environment(unsigned cmd,void *data) {
    if(nps64_gpu_environment(cmd,data)) return true;
    switch(cmd) {
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: *(unsigned*)data=0; return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES: {
        const struct retro_variable *v=data; option_count=0;
        for(;v->key && option_count<256;v++) {
            const char *start=strstr(v->value,"; "); if(!start) continue; start+=2;
            size_t len=strcspn(start,"|"); if(len>=128) len=127;
            snprintf(options[option_count].key,128,"%s",v->key);
            memcpy(options[option_count].value,start,len); options[option_count].value[len]=0; option_count++;
        }
        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v=data; v->value=override(v->key); if(v->value) return true;
        for(size_t i=0;i<option_count;i++) if(!strcmp(v->key,options[i].key)) { v->value=options[i].value; return true; }
        return false;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: *(bool*)data=options_dirty; options_dirty=false; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: *(const char**)data=system_dir; return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char**)data=saves_dir; return true;
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: ((struct retro_log_callback*)data)->log=core_log; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        if(*(enum retro_pixel_format*)data>RETRO_PIXEL_FORMAT_RGB565) return false;
        pixel_format=*(enum retro_pixel_format*)data; return true;
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: av=*(struct retro_system_av_info*)data; return true;
    case RETRO_ENVIRONMENT_SET_GEOMETRY: av.geometry=*(struct retro_game_geometry*)data; return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE: *(bool*)data=true; return true;
    case RETRO_ENVIRONMENT_SET_MESSAGE: notify(((struct retro_message*)data)->msg); return true;
    case RETRO_ENVIRONMENT_SHUTDOWN: core_shutdown=true; return true;
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: return true;
    default: return false;
    }
}
static void input_poll(void) { /* Main loop polls once; stable across core callbacks. */ }
static int16_t input_state(unsigned port,unsigned device,unsigned index,unsigned id) {
    if(port>=4 || !pads[port].connected) return 0;
    if(device==RETRO_DEVICE_JOYPAD && id<16) return !!(pads[port].joy&(1u<<id));
    if(device==RETRO_DEVICE_ANALOG && index<2 && id<2) {
        Input *p=&pads[port]; return index==0 ? (id==0?p->lx:p->ly) : (id==0?p->rx:p->ry);
    }
    return 0;
}
static void video_refresh(const void *data,unsigned width,unsigned height,size_t pitch) {
    bool hardware=data==RETRO_HW_FRAME_BUFFER_VALID;
    if(hardware) {
        if(!nps64_gpu_readback(&data,width,height)) {
            if(nps64_gpu_failed()) {core_shutdown=true;notify("Error Vulkan. Consulta nps64.log");}
            return;
        }
        pitch=(size_t)width*4;
    }
    if(!data || !width || !height || width>4096 || height>4096) return;
    size_t bytes=hardware || pixel_format==RETRO_PIXEL_FORMAT_XRGB8888?4:2;
    size_t row_bytes=width*bytes,needed=row_bytes*height;
    if(pitch<row_bytes) return;
    if(needed>native_capacity) {
        void *p=realloc(native_frame,needed);
        if(!p) {core_shutdown=true;notify("Memoria insuficiente para video");return;}
        native_frame=p;native_capacity=needed;
    }
    if(pitch==row_bytes)memcpy(native_frame,data,needed);
    else for(unsigned y=0;y<height;y++)memcpy(native_frame+y*row_bytes,(const unsigned char*)data+y*pitch,row_bytes);
    native_width=width;native_height=height;native_format=hardware?RETRO_PIXEL_FORMAT_XRGB8888:pixel_format;native_rgba=hardware;video_frames++;
    if(!first_video) {
        nps64_log("[load] first_video %ux%u pitch=%zu format=%d\n",width,height,pitch,pixel_format);
        first_video=true;
    }
}
#ifdef NPS64_HLE_GAME
void nps64_hle_cpu_video(const void *data,unsigned width,unsigned height,size_t pitch){video_refresh(data,width,height,pitch);}
#endif
#include "software_scaler.h"
static void game_video_draw(void) {
    uint32_t *out=platform_framebuffer();
    if(!native_width || !native_height) {memset(out,0,SCREEN_W*SCREEN_H*4);return;}
    unsigned width=native_width,height=native_height;
    size_t bytes=native_format==RETRO_PIXEL_FORMAT_XRGB8888?4:2,pitch=width*bytes;
    const void *data=native_frame;
    double ratio=widescreen?16.0/9.0:4.0/3.0;
    unsigned dw=(unsigned)(SCREEN_H*ratio),dh=SCREEN_H;
    if(dw>SCREEN_W) { dw=SCREEN_W; dh=(unsigned)(SCREEN_W/ratio); }
    unsigned left=(SCREEN_W-dw)/2,top=(SCREEN_H-dh)/2;
    /* One division per destination column, instead of one per output pixel. */
    static unsigned xmap[SCREEN_W],map_width,map_dw;
    if(map_width!=width || map_dw!=dw) {
        for(unsigned x=0;x<dw;x++) xmap[x]=x*width/dw;
        map_width=width;map_dw=dw;
    }
    /* Clear borders only; the scaler overwrites the complete game rectangle. */
    if(top) memset(out,0,top*SCREEN_W*4);
    if(top+dh<SCREEN_H) memset(out+(top+dh)*SCREEN_W,0,(SCREEN_H-top-dh)*SCREEN_W*4);
    int vector_scaler=1;
#ifndef __ORBIS__
    if(getenv("NPS64_CPU_SCALAR"))vector_scaler=0;
#endif
    unsigned previous_sy=~0u;
    for(unsigned y=0;y<dh;y++) {
        unsigned sy=y*height/dh;
        uint32_t *dst=out+(y+top)*SCREEN_W+left;
        if(left) {
            for(unsigned x=0;x<left;x++) out[(y+top)*SCREEN_W+x]=0xff000000;
            for(unsigned x=left+dw;x<SCREEN_W;x++) out[(y+top)*SCREEN_W+x]=0xff000000;
        }
        if(sy==previous_sy) {memcpy(dst,dst-SCREEN_W,dw*4);continue;}
        previous_sy=sy;
        const unsigned char *row=(const unsigned char*)data+(size_t)sy*pitch;
        if(bytes==4&&vector_scaler&&nps64_scale_row32(dst,row,width,dw,native_rgba))continue;
        for(unsigned x=0;x<dw;x++) {
            unsigned sx=xmap[x]; uint32_t c;
            if(bytes==4) { memcpy(&c,row+sx*4,4); if(native_rgba)c=(c&0xff00ff00)|((c&255)<<16)|((c>>16)&255); c|=0xff000000; }
            else {
                uint16_t p; memcpy(&p,row+sx*2,2);
                unsigned r=(p>>(native_format==RETRO_PIXEL_FORMAT_RGB565?11:10))&31;
                unsigned g=(p>>5)&(native_format==RETRO_PIXEL_FORMAT_RGB565?63:31),b=p&31;
                c=0xff000000|((r*255/31)<<16)|((g*255/(native_format==RETRO_PIXEL_FORMAT_RGB565?63:31))<<8)|(b*255/31);
            }
            dst[x]=c;
        }
    }
#ifndef __ORBIS__
    if(getenv("NPS64_CPU_FRAME_HASHES")){
        uint64_t hash=1469598103934665603ull;
        for(size_t i=0;i<SCREEN_W*SCREEN_H;i++){hash^=out[i];hash*=1099511628211ull;}
        nps64_log("[cpu-image] frame=%llu value=%016llx\n",(unsigned long long)core_frames,(unsigned long long)hash);
    }
#endif
    if(!visible_video) {
        for(size_t i=0;i<SCREEN_W*SCREEN_H;i+=64) if(out[i]&0x00ffffff) {
            visible_video=true;nps64_log("[load] first_visible_video frame=%llu\n",(unsigned long long)core_frames);break;
        }
    }
}
#ifdef NPS64_HLE_GAME
static void hle_preview_draw(unsigned frame) {
    uint32_t *fb=platform_framebuffer();memset(fb,0,SCREEN_W*SCREEN_H*4);
    for(unsigned y=0;y<512;y++)for(unsigned x=0;x<512;x++) {
        unsigned p=((y/2)*256+x/2)*4;
        fb[(y+104)*SCREEN_W+x+384]=0xff000000u|((uint32_t)hle_pixels[p]<<16)|((uint32_t)hle_pixels[p+1]<<8)|hle_pixels[p+2];
    }
    platform_text(32,30,"NPS64 00.38 - Geometria real en Vulkan",0xffffffff,2);
    char text[120];
    if(nps64_hle_sequence_active)snprintf(text,sizeof(text),"Secuencia Vulkan: frame %u/64. Circulo: terminar.",frame+1);
    else snprintf(text,sizeof(text),"Ultimo frame Vulkan. Circulo: volver a la pausa.");
    platform_text(32,70,text,0xffffffff,1);
    platform_text(32,660,"Prueba de movimiento y materiales; no mide rendimiento del juego.",0xffffffff,1);
}
static double hle_sequence_started;
static uint32_t hle_scaled[960*720];
static bool hle_scaled_dirty;
typedef struct {unsigned address,width,height,pixel_bytes;bool valid;unsigned char pixels[256*256*4];} HleScanout;
static HleScanout hle_scanouts[3];
static unsigned hle_scanout_next;
#ifndef __ORBIS__
/* Keep diagnostics attached to the VI-selected buffer, not the next draw target. */
static struct {HleSceneFrame scene;HleVertex vertices[30000];HleMaterial materials[10000];unsigned char depth[10000],atlas[512*512*4];unsigned long long frame;} hle_capture_scanouts[3];
static unsigned hle_display_cache;
#endif
static HleViState hle_display_vi;
static unsigned hle_display_width,hle_display_height,hle_display_offset;
static bool hle_select_scanout(void){
    const HleViState *vi=nps64_hle_vi_state();
    for(unsigned i=0;i<3;i++){
        HleScanout *b=&hle_scanouts[i];
        if(!b->valid||vi->origin<b->address||vi->origin-b->address>=b->width*b->height*b->pixel_bytes)continue;
        hle_display_vi=*vi;hle_display_width=b->width;hle_display_height=b->height;
#ifndef __ORBIS__
        hle_display_cache=i;
#endif
        hle_display_offset=(vi->origin-b->address)/b->pixel_bytes;
        memcpy(hle_pixels,b->pixels,sizeof(hle_pixels));hle_scaled_dirty=true;return true;
    }
    return false;
}

static void hle_game_draw(void) {
    uint32_t *fb=platform_framebuffer();
    /* Convert each source row once; repeated VI frames reuse the scaled image. */
    if(hle_scaled_dirty){
        int xmap[960],ymap[720];
        const HleViState *vi=&hle_display_vi;
        for(unsigned x=0;x<960;x++){
            xmap[x]=(int)(x*256/960);
            if(vi->width&&hle_display_width){
                float vx=x*(float)vi->width/960;
                float tx=(vx-vi->left)*vi->x_step/1024.f+vi->x_start/1024.f+hle_display_offset%hle_display_width;
                xmap[x]=vx<vi->left+8||vx>=vi->left+vi->active_width-7||tx<0||tx>=hle_display_width?-1:(int)(tx*256/hle_display_width);
            }
        }
        for(unsigned y=0;y<720;y++){
            ymap[y]=(int)(y*256/720);
            if(vi->height&&hle_display_height){
                float vy=y*(float)vi->height/720;
                float ty=(vy-vi->top)*vi->y_step/1024.f+vi->y_start/1024.f+hle_display_offset/hle_display_width;
                ymap[y]=vy<vi->top||vy>=vi->top+vi->active_height||ty<0||ty>=hle_display_height?-1:(int)(ty*256/hle_display_height);
            }
        }
        int previous=-2;
        for(unsigned y=0;y<720;y++){
            int sy=ymap[y];uint32_t *row=hle_scaled+y*960;
            if(sy==previous)memcpy(row,row-960,960*4);
            else for(unsigned x=0;x<960;x++){
                if(sy<0||xmap[x]<0){row[x]=0xff000000;continue;}
                unsigned p=((unsigned)sy*256+(unsigned)xmap[x])*4;
                row[x]=0xff000000u|((uint32_t)hle_pixels[p]<<16)|((uint32_t)hle_pixels[p+1]<<8)|hle_pixels[p+2];
            }
            previous=sy;
        }
        hle_scaled_dirty=false;
    }
    for(unsigned y=0;y<720;y++){
        uint32_t *row=fb+y*SCREEN_W;
        memset(row,0,160*4);memcpy(row+160,hle_scaled+y*960,960*4);memset(row+1120,0,160*4);
    }
}
#ifndef __ORBIS__
/* Paired captures before HUD, from the same completed retro_run; no extra VI. */
static void hle_compare_capture(void) {
    const char *dir=getenv("NPS64_COMPARE_DIR");
    if(!dir||!native_width||!native_height)return;
    if(hle_gpu_frames!=1&&hle_gpu_frames%60)return;
    const HleSceneFrame *scene=&hle_capture_scanouts[hle_display_cache].scene;
    const HleViState *vi=nps64_hle_vi_state();
    nps64_log("[hle-compare] frame=%llu origin=%x framebuffer=%x VI=%ux%u active=%d,%d %dx%d start=%d,%d step=%d,%d\n",
        (unsigned long long)core_frames,vi->origin,vi->framebuffer,vi->width,vi->height,vi->left,vi->top,vi->active_width,vi->active_height,vi->x_start,vi->y_start,vi->x_step,vi->y_step);
    char path[512];
    const void *blocks[5]={scene->vertices,scene->materials,scene->depth,scene->atlas,hle_pixels};
    size_t sizes[5]={scene->count*sizeof(HleVertex),scene->count/3*sizeof(HleMaterial),scene->count/3,512*512*4,256*256*4};
    const char *names[5]={"vertices","materials","depth","atlas","scanout"};
    for(unsigned i=0;i<5;i++){
        snprintf(path,sizeof(path),"%s/frame-%llu-%s.bin",dir,(unsigned long long)core_frames,names[i]);
        FILE *f=fopen(path,"wb");if(!f){perror(path);exit(2);}
        if(fwrite(blocks[i],1,sizes[i],f)!=sizes[i]){fclose(f);exit(2);}if(fclose(f))exit(2);
    }
    snprintf(path,sizeof(path),"%s/frame-%llu.json",dir,(unsigned long long)core_frames);
    FILE *meta=fopen(path,"w");if(!meta){perror(path);exit(2);}
    fprintf(meta,"{\"core_frame\":%llu,\"video_frames\":%llu,\"native_width\":%u,\"native_height\":%u,\"gpu_frames\":%llu,\"filtered_vi\":%s,\"vi_origin\":%u,\"render_target\":%u,\"display_offset_pixels\":%u,\"display_target\":%u,\"scene_frame\":%llu}\n",
            (unsigned long long)core_frames,(unsigned long long)video_frames,
            native_width,native_height,hle_gpu_frames,filtered_vi?"true":"false",vi->origin,vi->framebuffer,hle_display_offset,hle_scanouts[hle_display_cache].address,hle_capture_scanouts[hle_display_cache].frame);
    if(fclose(meta)){perror(path);exit(2);}
    for(unsigned mode=0;mode<2;mode++){
        if(mode)hle_game_draw();else game_video_draw();
        snprintf(path,sizeof(path),"%s/frame-%llu-%s.ppm",dir,
                 (unsigned long long)core_frames,mode?"vulkan":"cpu");
        FILE *f=fopen(path,"wb");if(!f){perror(path);exit(2);}
        fprintf(f,"P6\n960 720\n255\n");
        const uint32_t *fb=platform_framebuffer();
        for(unsigned y=0;y<720;y++)for(unsigned x=0;x<960;x++){
            uint32_t c=fb[y*SCREEN_W+x+160];
            unsigned char rgb[3]={(unsigned char)(c>>16),(unsigned char)(c>>8),(unsigned char)c};
            if(fwrite(rgb,1,3,f)!=3){fclose(f);exit(2);}
        }
        if(fclose(f)){perror(path);exit(2);}
    }
}
#endif
static void hle_game_frame(void) {
    HleSceneFrame scene={0};
    nps64_hle_geometry_log_detail(hle_gpu_frames<3||hle_gpu_frames%60==0);
    scene.vertices=nps64_hle_geometry_scene(native_width,native_height,&scene.count);
    scene.depth=nps64_hle_geometry_depth();scene.materials=nps64_hle_geometry_materials();scene.atlas=nps64_hle_geometry_atlas();
    if(!scene.count){
        if(hle_frame_ready&&!nps64_hle_geometry_commands()){
            hle_frame_ready=hle_select_scanout();
            hle_repeated_frames++;
            if(hle_repeated_frames==1||hle_repeated_frames%60==0)nps64_log("[hle-live] unchanged RDP repeated_frames=%llu\n",hle_repeated_frames);
            return;
        }
        hle_frame_ready=false;hle_cpu_fallbacks++;
        if(hle_cpu_fallbacks==1||hle_cpu_fallbacks%60==0)nps64_log("[hle-live] CPU fallback empty geometry frames=%llu\n",hle_cpu_fallbacks);
        return;
    }
    if(!nps64_hle_geometry_complete()){
        hle_frame_ready=false;hle_cpu_fallbacks++;hle_incomplete_frames++;
        if(hle_incomplete_frames<=3||hle_incomplete_frames%60==0){
            nps64_log("[hle-live] CPU fallback incomplete graphics frames=%llu\n",hle_cpu_fallbacks);
            nps64_hle_geometry_failure_log();
        }
        return;
    }
#ifndef __ORBIS__
    /* Real-ROM test of deferred VI recovery with a deterministic CPU fallback. */
    const char *forced_fallback=getenv("NPS64_TEST_HLE_FALLBACK_FRAME");
    if(forced_fallback&&strtoull(forced_fallback,NULL,10)==core_frames){
        hle_frame_ready=false;hle_cpu_fallbacks++;
        nps64_log("[hle-live] test CPU fallback frame=%llu\n",(unsigned long long)core_frames);return;
    }
#endif
    int status=0;
    if(!hle_renderer)status=nps64_hle_renderer_open(&hle_renderer,&scene);
    if(!status)status=nps64_hle_renderer_frame(hle_renderer,&scene,hle_pixels);
    if(status){
        hle_continuous=false;hle_frame_ready=false;hle_renderer_failed=true;
        nps64_log("[hle-live] renderer failed; CPU mode restored\n");
        notify("Error Vulkan; vuelta a CPU. Consulta nps64.log");return;
    }
    const HleViState *vi=nps64_hle_vi_state();
    unsigned cache=3;
    for(unsigned i=0;i<3;i++)if(hle_scanouts[i].valid&&hle_scanouts[i].address==vi->framebuffer){cache=i;break;}
    if(cache==3){cache=hle_scanout_next;hle_scanout_next=(hle_scanout_next+1)%3;}
    HleScanout *b=&hle_scanouts[cache];b->address=vi->framebuffer;b->width=vi->target_width;b->height=vi->target_height;b->pixel_bytes=vi->pixel_bytes;b->valid=vi->framebuffer&&b->width&&b->height&&b->pixel_bytes;
    memcpy(b->pixels,hle_pixels,sizeof(hle_pixels));
#ifndef __ORBIS__
    if(getenv("NPS64_COMPARE_DIR")){
        memcpy(hle_capture_scanouts[cache].vertices,scene.vertices,scene.count*sizeof(HleVertex));
        memcpy(hle_capture_scanouts[cache].materials,scene.materials,scene.count/3*sizeof(HleMaterial));
        memcpy(hle_capture_scanouts[cache].depth,scene.depth,scene.count/3);
        memcpy(hle_capture_scanouts[cache].atlas,scene.atlas,512*512*4);
        hle_capture_scanouts[cache].frame=core_frames;
        hle_capture_scanouts[cache].scene=(HleSceneFrame){hle_capture_scanouts[cache].vertices,scene.count,hle_capture_scanouts[cache].depth,hle_capture_scanouts[cache].materials,hle_capture_scanouts[cache].atlas};
    }
#endif
    hle_frame_ready=hle_select_scanout();hle_scaled_dirty=true;hle_gpu_frames++;
    if(!hle_frame_ready){hle_cpu_fallbacks++;return;}

#ifndef __ORBIS__
    hle_compare_capture();
#endif
    if(hle_gpu_frames==1||hle_gpu_frames%60==0)nps64_log("[hle-live] GPU frames=%llu vertices=%zu CPU_reference=Angrylion fallback=%llu\n",hle_gpu_frames,scene.count,hle_cpu_fallbacks);
}
static int hle_sequence_next(unsigned frame,HleSceneFrame *scene,void *context) {
    (void)context;
    if(!frame)hle_sequence_started=platform_time();
    if(!frame&&context){*scene=*(const HleSceneFrame *)context;return 1;}
    if(frame){platform_poll(pads);if(pads[0].buttons&BTN_BACK)return 0;}
    nps64_hle_vi_defer_enable(0);
    nps64_hle_geometry_capture_textures(1);
    for(unsigned attempt=0;attempt<6;attempt++) {
        nps64_hle_geometry_begin();retro_run();core_frames++;
        scene->vertices=nps64_hle_geometry_scene(native_width,native_height,&scene->count);
        if(scene->count||core_shutdown)break;
    }
    nps64_hle_geometry_capture_textures(0);platform_audio_clear();
    if(!scene->count||core_shutdown){nps64_log("[hle-game] capture failed frame=%u after 6 VI: vertices=%zu shutdown=%u VI=%ux%u\n",frame,scene->count,core_shutdown,native_width,native_height);return -1;}
    scene->depth=nps64_hle_geometry_depth();scene->materials=nps64_hle_geometry_materials();scene->atlas=nps64_hle_geometry_atlas();
    if((frame<8||frame==63)&&native_format==RETRO_PIXEL_FORMAT_XRGB8888){
        unsigned lit=0;
        for(size_t i=0;i<(size_t)native_width*native_height;i++){uint32_t p;memcpy(&p,native_frame+i*4,4);lit+=(p&0xffffffu)!=0;}
        nps64_log("[hle-game] CPU reference frame=%u lit=%u total=%u\n",frame,lit,native_width*native_height);
    }
    return 1;
}
static void hle_sequence_present(unsigned frame,const unsigned char *pixels,void *context) {
    (void)pixels;(void)context;hle_preview_draw(frame);platform_present();
#ifndef __ORBIS__
    uint64_t hash=1469598103934665603ull;
    for(size_t i=0;i<sizeof(hle_pixels);i++){hash^=pixels[i];hash*=1099511628211ull;}
    nps64_log("[hle-game] image_hash frame=%u value=%016llx\n",frame,(unsigned long long)hash);
#endif
    if(frame==63)nps64_log("[hle-game] sequence elapsed=%.3f seconds; includes CPU reference, Vulkan and display\n",platform_time()-hle_sequence_started);
}
#endif
static size_t audio_batch(const int16_t *data,size_t frames) {
#ifdef NPS64_HLE_GAME
    if(nps64_hle_sequence_active)return frames;
#endif
    platform_audio(data,frames,av.timing.sample_rate); return frames;
}
static void audio_sample(int16_t l,int16_t r) { int16_t samples[2]={l,r}; audio_batch(samples,1); }
static void save_path(char path[PATH_CAP],const char *suffix) { snprintf(path,PATH_CAP,"%s/%s%s",saves_dir,current.key,suffix); }
static bool save_ram(void) {
    if(!ram_writes_allowed) return false;
    size_t n=retro_get_memory_size(RETRO_MEMORY_SAVE_RAM); void *p=retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    if(!p || !n) return true;
    char path[PATH_CAP]; save_path(path,".srm");
    bool ok=file_write_atomic(path,p,n); if(!ok) notify("ERROR al guardar partida; revisa espacio en /data"); return ok;
}
static void state_action(bool load) {
    size_t n=retro_serialize_size(); if(!n || n>64*1024*1024) { notify("Este nucleo no ofrece un estado valido"); return; }
    void *p=malloc(n); if(!p) { notify("Memoria insuficiente para estado"); return; }
    char suffix[32],path[PATH_CAP]; snprintf(suffix,sizeof(suffix),".slot%u.state",slot); save_path(path,suffix);
    bool ok=load ? (state_read(path,current.key,p,n) && retro_unserialize(p,n)) : (retro_serialize(p,n) && state_write(path,current.key,p,n));
    free(p); platform_audio_clear();
#ifdef NPS64_HLE_GAME
    if(ok&&load){hle_frame_ready=false;memset(hle_scanouts,0,sizeof(hle_scanouts));hle_display_vi.width=0;}
#endif
    notify(ok ? (load?"Estado cargado":"Estado guardado") : (load?"Estado vacio, incompatible o corrupto":"No se pudo guardar estado"));
}
static void config_save(void) {
    char text[64]; int n=snprintf(text,sizeof(text),"NPS64CFG2 %u %u %u %u\n",theme,widescreen,slot,filtered_vi);
    if(!file_write_atomic(DATA_ROOT "/settings.cfg",text,(size_t)n)) notify("No se pudo guardar configuracion");
}
static bool start_game(void) {
    current=games[selected]; core_shutdown=false; options_dirty=false; ram_writes_allowed=true;
    core_frames=video_frames=0; first_video=false; visible_video=false;native_width=native_height=0;game_fps=video_fps=0;
    nps64_log("[load] path=%s name=%s\n",current.path,current.name);
    notify("Leyendo ROM. Espera..."); menu_draw();
    if(!platform_present()) return false;
    double started=platform_time();
    nps64_log("[load] hash_begin\n");
    if(!rom_content_key(&current)) {notify("No se pudo leer la ROM completa");return false;}
    nps64_log("[load] hash_ok key=%s seconds=%.3f\n",current.key,platform_time()-started);
    use_gpu=requested_gpu && nps64_gpu_begin();
    if(nps64_core_game_profile) {
        uint8_t header[64];unsigned profile=0;
        FILE *rom=fopen(current.path,"rb");
        if(rom){size_t size=fread(header,1,sizeof(header),rom);fclose(rom);profile=nps64_cpu_profile_header(header,size);}
        nps64_core_game_profile(profile);
        nps64_log("[cpu-profile] %s\n",profile?"zelda-pal-gc reciprocal lanes":"standard lanes");
    }
    struct retro_game_info info={current.path,NULL,0,NULL};
    for(;;) {
        use_jit=requested_cpu && nps64_core_jit_supported();
        nps64_log("[jit] requested=%u executable=%d selected=%s\n",requested_cpu,use_jit,use_jit?"Ari64":"cached interpreter");
        nps64_log("[load] renderer=%s CPU=%s\n",use_gpu?"paraLLEl-RDP Vulkan":"Angrylion",use_jit?"Ari64":"cached");
        retro_init();
        if(!retro_load_game(&info)) {
            retro_deinit();nps64_gpu_close();use_gpu=false;
            notify("No se pudo cargar la ROM. Consulta nps64.log");return false;
        }
        if(!use_gpu || nps64_gpu_start()) break;
        nps64_log("[gpu] startup failed; reloading with Angrylion\n");
        retro_unload_game();nps64_gpu_destroy_context();retro_deinit();nps64_gpu_close();use_gpu=false;
        core_shutdown=false;native_width=native_height=0;
    }
    nps64_log("[load] retro_load_game_ok\n");
    retro_get_system_av_info(&av);
    nps64_log("[load] AV fps=%.3f audio=%.3f\n",av.timing.fps,av.timing.sample_rate);
    for(unsigned i=0;i<4;i++) retro_set_controller_port_device(i,pads[i].connected?RETRO_DEVICE_JOYPAD:RETRO_DEVICE_NONE);
    size_t n=retro_get_memory_size(RETRO_MEMORY_SAVE_RAM); void *p=retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    char path[PATH_CAP]; save_path(path,".srm");
    struct stat st;
    bool bad_save=p && n && stat(path,&st)==0 && !file_read_exact(path,p,n);
    if(bad_save) ram_writes_allowed=false;
    running=true; paused=false; last_autosave=platform_time(); platform_audio_clear();
    notify(bad_save?"SRAM ilegible: archivo protegido. Respaldalo y retiralo via FTP":"L3 + R3: pausa / estados / volver a biblioteca"); return true;
}
static void stop_game(void) {
#ifdef NPS64_HLE_GAME
    hle_continuous=false;hle_frame_ready=false;hle_preview=false;memset(hle_scanouts,0,sizeof(hle_scanouts));hle_display_vi.width=0;
    nps64_hle_geometry_capture_textures(0);
    nps64_hle_geometry_log_detail(1);
    nps64_log("[hle-live] session ended GPU_frames=%llu CPU_fallback=%llu; context reused\n",hle_gpu_frames,hle_cpu_fallbacks);
    hle_gpu_frames=hle_cpu_fallbacks=hle_repeated_frames=hle_incomplete_frames=0;
#endif
    save_ram(); retro_unload_game(); nps64_gpu_destroy_context(); retro_deinit(); nps64_gpu_close();use_gpu=false; running=false; paused=false; platform_audio_clear();
}
static void rect(int x,int y,int w,int h,uint32_t color) {
    uint32_t *fb=platform_framebuffer();
    for(int py=y;py<y+h && py<SCREEN_H;py++) for(int px=x;px<x+w && px<SCREEN_W;px++) if(px>=0 && py>=0) fb[py*SCREEN_W+px]=color;
}
static void game_hud_draw(void) {
    char text[160];
    const char *renderer=use_gpu?"GPU Vulkan":"CPU Angrylion";
#ifdef NPS64_HLE_GAME
    if(hle_continuous)renderer=hle_frame_ready?"Vulkan + ref CPU":"CPU (fallback Vulkan)";
#endif
    double target=av.timing.fps,speed=target>0?100*game_fps/target:0;
    rect(16,12,SCREEN_W-32,44,0xff101820);
    snprintf(text,sizeof(text),"FPS %.1f / %.0f | velocidad %.0f%% | CPU %s | %s",
             game_fps,target,speed,use_jit?"Ari64":"cached",renderer);
    platform_text(24,17,text,0xffeeeeee,1);
    snprintf(text,sizeof(text),"core %.1f ms [RDP %.1f VI %.1f] | dibujo %.1f | flip %.1f ms | video %.1f FPS",
             perf_core_ms,perf_rdp_ms,perf_vi_ms,perf_draw_ms,perf_flip_ms,video_fps);
    platform_text(24,36,text,0xffadbcd2,1);
}
static void menu_draw(void) {
    rect(0,0,SCREEN_W,SCREEN_H,0xff101e30);
    rect(0,0,SCREEN_W,5,accent[theme]);
    platform_text(64,40,"NPS64 by lTunix",accent[theme],4);
    platform_text(64,115,"BIBLIOTECA     |     NINTENDO 64     |     PS4",0xffadbcd2,2);
    if(input_test) {
        platform_text(64,190,"PRUEBA DEL MANDO - acciones de biblioteca suspendidas",accent[theme],2);
        platform_text(64,245,"X: jugar | Cuadrado: buscar ROMs | Circulo: volver",0xffeeeeee,2);
        platform_text(64,285,"Options: salir | Triangulo: cerrar esta prueba",0xffeeeeee,2);
        platform_text(64,325,"En juego: X=A, Cuadrado=B, L2=Z, Options=Start",0xffeeeeee,2);
        platform_text(64,365,"Stick izquierdo: N64 | Stick derecho: botones C",0xffeeeeee,2);
        char values[150];
        snprintf(values,sizeof(values),"ScePad: %08x  acciones: %04x  N64: %04x",pads[0].raw_buttons,pads[0].buttons,pads[0].joy);
        platform_text(64,425,values,accent[theme],2);
        snprintf(values,sizeof(values),"X=%d Circulo=%d Cuadrado=%d Triangulo=%d",!!(pads[0].buttons&BTN_CONFIRM),!!(pads[0].buttons&BTN_BACK),!!(pads[0].buttons&BTN_SCAN),!!(pads[0].buttons&BTN_HELP));
        platform_text(64,465,values,accent[theme],2);
        snprintf(values,sizeof(values),"Izq: %d %d  Der: %d %d  Conectado: %d",pads[0].lx,pads[0].ly,pads[0].rx,pads[0].ry,pads[0].connected);
        platform_text(64,505,values,0xffadbcd2,2);
        return;
    }
#ifdef NPS64_HD_UI
    char updates[160]="";
#ifdef NPS64_UPDATES
    nps64_updater_status(updates,sizeof(updates),NULL);
#endif
    library_ui_draw(games,game_count,selected,accent[theme],message,updates);
    return;
#endif
    if(!game_count) {
        platform_text(64,235,"No hay ROMs validas. Formatos: .z64 .v64 .n64",0xffeeeeee,2);
        platform_text(64,280,"Copia tus juegos en /data/NPS64/games",accent[theme],2);
    }
    size_t first=selected>=8?selected-7:0;
    for(size_t i=first;i<game_count && i<first+9;i++) {
        int y=195+(int)(i-first)*42;
        if(i==selected) rect(48,y-5,1180,36,0xff263b51);
        char label[74]; snprintf(label,sizeof(label),"%s%.67s",i==selected?"> ":"  ",games[i].name);
        platform_text(64,y,label,i==selected?accent[theme]:0xffdee4ed,2);
    }
#ifdef NPS64_UPDATES
    platform_text(64,600,"X: jugar | Cuadrado: ROMs | L1/R1: pagina | L2: actualizaciones",0xffadbcd2,2);
#else
    platform_text(64,600,"X: jugar | Cuadrado: buscar ROMs | L1/R1: pagina",0xffadbcd2,2);
#endif
    platform_text(64,633,"Triangulo: probar mando | Options: salir | Izq/Der: tema",0xffadbcd2,2);
#ifdef NPS64_UPDATES
    char update[144];nps64_updater_status(update,sizeof(update),NULL);
    platform_text(64,565,update,accent[theme],1);
#endif
    platform_text(64,677,message,accent[theme],1);
}
static void pause_draw(void) {
#ifndef NPS64_HD_UI
    rect(275,95,730,510,0xff101e30); rect(275,95,730,5,accent[theme]);
    platform_text(320,128,"PAUSA",accent[theme],3);
#endif
    const char *labels[]={"Continuar","Guardar estado","Cargar estado","Aspecto 4:3 / 16:9","Volver a biblioteca",filtered_vi?"Imagen: precisa (X: rapida)":"Imagen: rapida (X: precisa)"
#ifdef NPS64_UPDATES
        ,"Buscar actualizaciones"
#endif
    };
#ifdef NPS64_HD_UI
    char updates[160]="";
#ifdef NPS64_UPDATES
    nps64_updater_status(updates,sizeof(updates),NULL);
#endif
    library_ui_pause(labels,PAUSE_CHOICES,pause_selected,slot,accent[theme],message,updates);
    return;
#endif
    for(unsigned i=0;i<PAUSE_CHOICES;i++) {
        if(pause_selected==i) rect(310,207+(int)i*42,650,39,0xff263b51);
        platform_text(325,215+(int)i*42,labels[i],pause_selected==i?accent[theme]:0xffeeeeee,2);
    }
    char text[100]; snprintf(text,sizeof(text),"Ranura: %u / 7  |  Izq/Der: cambiar",slot);
    platform_text(320,525,text,0xffadbcd2,2);
    platform_text(320,562,message,accent[theme],1);
#ifdef NPS64_UPDATES
    char update[81];nps64_updater_status(update,sizeof(update),NULL);
    platform_text(320,583,update,accent[theme],1);
#endif
}
int main(void) {
#if defined(__ORBIS__) && defined(NPS64_GPU)
    extern void nps64_boot_trace(const char*);
    nps64_boot_trace("main entered");
#endif
    const char *dirs[]={DATA_ROOT,games_dir,saves_dir,system_dir};
    for(size_t i=0;i<4;i++) { if(mkdir(dirs[i],0777)) { struct stat st; if(stat(dirs[i],&st) || !S_ISDIR(st.st_mode)) { nps64_log("No se puede crear %s\n",dirs[i]); return 1; } } }
    nps64_log_init(DATA_ROOT "/nps64.log");
#if defined(NPS64_HLE_GAME)
    nps64_log("NPS64 00.38 Ari64 / Angrylion + Vulkan game geometry sequence\n");
#elif defined(NPS64_GPU_EXPERIMENTAL)
    nps64_log("NPS64 00.19 EXPERIMENTAL core 890323d Ari64 / Vulkan color raster\n");
#elif defined(NPS64_GPU) && defined(__ORBIS__)
    nps64_log("NPS64 00.13 core 890323d Ari64 / Vulkan quarantined + Angrylion\n");
#elif defined(NPS64_GPU)
    nps64_log("NPS64 00.13 core 890323d Ari64 / Vulkan + Angrylion\n");
#else
#ifdef NPS64_CPU_RDP_PGO
    nps64_log("NPS64 by lTunix " NPS64_VERSION " CPU ONLY core 890323d Ari64 / Angrylion PGO\n");
#else
    nps64_log("NPS64 by lTunix " NPS64_VERSION " CPU ONLY core 890323d Ari64 / Angrylion\n");
#endif
#endif
    FILE *cfg=fopen(DATA_ROOT "/settings.cfg","r");
    if(cfg) {
        char format[20];unsigned t,w,s,f=filtered_vi;
        int count=fscanf(cfg,"%19s %u %u %u %u",format,&t,&w,&s,&f);
        if(count>=4 && t<3 && w<2 && s<8 && f<2 &&
           (!strcmp(format,"NPS64CFG1") || !strcmp(format,"NPS64CFG2"))) {
            theme=t;widescreen=w;slot=s;if(count==5 && !strcmp(format,"NPS64CFG2"))filtered_vi=f!=0;
        }
        fclose(cfg);
    }
    FILE *diag=fopen(DATA_ROOT "/diagnostics.cfg","r");
    if(diag) {
        char format[20]; unsigned workers,hle=1,cpu=1,filter=filtered_vi,gpu=1;
        if(fscanf(diag,"%19s %u %u %u %u %u",format,&workers,&hle,&cpu,&filter,&gpu)>=2 && (workers<=8) && hle<=1 && cpu<=1 && filter<=1 && gpu<=1) {
            if(!strcmp(format,"NPS64DIAG1") || !strcmp(format,"NPS64DIAG2") || !strcmp(format,"NPS64DIAG3") || !strcmp(format,"NPS64DIAG4") || !strcmp(format,"NPS64DIAG5")) {
                diagnostic_workers=workers;diagnostic_hle=hle!=0;requested_cpu=cpu;filtered_vi=filter!=0;requested_gpu=gpu;
                nps64_log("[diag] diagnostics.cfg override: workers=%u\n",workers);
            }
        }
        fclose(diag);
    }
#ifndef __ORBIS__
    /* Host benchmark only; console profile remains explicit and reproducible. */
    const char *gpu=getenv("BENCH_GPU");if(gpu)requested_gpu=strcmp(gpu,"off")!=0;
    const char *rsp=getenv("BENCH_RSP"); if(rsp) diagnostic_hle=!strcmp(rsp,"hle");
    const char *cpu=getenv("BENCH_CPU");if(cpu)requested_cpu=!strcmp(cpu,"jit");
    const char *filter=getenv("BENCH_VI");if(filter)filtered_vi=strcmp(filter,"fast")!=0;
#endif
#ifdef NPS64_HLE_GAME
    diagnostic_hle=true; /* The geometry observer requires the HLE walker. */
#endif
#if (defined(__ORBIS__) && !defined(NPS64_GPU_EXPERIMENTAL)) || defined(NPS64_TEST_GPU_QUARANTINE)
    /* GPU hangs affect the whole console. Ignore old GPU requests until the
       GNM compute path has been verified on hardware. */
    requested_gpu=0;
#ifdef NPS64_HLE_GAME
    nps64_log("[gpu] graphics Vulkan available for continuous game mode; Angrylion reference retained\n");
#elif defined(NPS64_GPU)
    nps64_log("[gpu] PS4 Vulkan quarantined after system freeze; using CPU Angrylion\n");
#endif
#endif
#if !defined(NPS64_GPU) && !defined(NPS64_HLE_GAME)
    requested_gpu=0;
    nps64_log("[renderer] CPU Angrylion; Vulkan not included\n");
#endif
    nps64_log("[diag] cpu_requested=%u rsp=%s workers=%u vi=%s\n",requested_cpu,diagnostic_hle?"hle":"cxd4",diagnostic_workers,filtered_vi?"filtered":"fast");
#ifdef __ORBIS__
    extern const char *ps4rt_heap_source(void);
    extern size_t ps4rt_heap_size(void),ps4rt_direct_available(void),ps4rt_flexible_available(void);
    nps64_log("[heap] source=%s size=%zu direct_available=%zu flexible_available=%zu\n",ps4rt_heap_source(),ps4rt_heap_size(),ps4rt_direct_available(),ps4rt_flexible_available());
#endif
#if defined(__ORBIS__) && defined(NPS64_GPU)
    nps64_boot_trace("frontend log/config ready; platform init begin");
#endif
    if(!platform_init()) { nps64_log("Error inicializando PS4\n"); free(native_frame); return 1; }
#if defined(NPS64_HLE_GAME) && defined(__ORBIS__)
    nps64_gpu_static_init();
    setenv("RADV_PS4_SERIAL_SUBMIT","1",1);
#endif
#if defined(__ORBIS__) && defined(NPS64_GPU)
    nps64_boot_trace("platform init complete");
#endif
    if(retro_api_version()!=RETRO_API_VERSION) { notify("ABI libretro incompatible"); platform_shutdown(); free(native_frame); return 1; }
    retro_set_environment(environment); retro_set_video_refresh(video_refresh);
    retro_set_audio_sample(audio_sample); retro_set_audio_sample_batch(audio_batch);
    retro_set_input_poll(input_poll); retro_set_input_state(input_state);
    game_count=library_scan(games_dir,games,512);
    snprintf(message,sizeof(message),"%zu ROMs encontradas. X: abrir | Cuadrado: actualizar",game_count);
    nps64_log("[library] startup count=%zu\n",game_count);
#ifdef NPS64_UPDATES
    nps64_updater_init();
#endif
#ifdef NPS64_HD_UI
    library_ui_init();
#endif
    uint32_t previous=0; bool connected[4]={false}; double repeat_at=0,deadline=platform_time();
    double perf_since=platform_time(),poll_sum=0,core_sum=0,draw_sum=0,flip_sum=0,gpu_sum=0;
    double rdp_sum=0,vi_sum=0,pace_sum=0,pace_overshoot_sum=0;
    unsigned late_frames=0,pace_clamps=0;
    double core_peak=0,rdp_peak=0;
    unsigned perf_samples=0;uint64_t perf_core_frames=core_frames,perf_video_frames=video_frames;bool perf_game=false;
    while(!quit_requested) {
        double loop_start=platform_time(),core_elapsed=0,gpu_elapsed=0;
        platform_poll(pads); uint32_t held=pads[0].buttons,pressed=held&~previous; previous=held;
        double poll_elapsed=platform_time()-loop_start;
        if(pressed) nps64_log("[input] raw=%08x pressed=%04x running=%d paused=%d test=%d\n",pads[0].raw_buttons,pressed,running,paused,input_test);
        double now=platform_time();
        double draw_start=now;
        if((held&(BTN_UP|BTN_DOWN)) && now>=repeat_at) { pressed|=held&(BTN_UP|BTN_DOWN); repeat_at=now+0.16; }
        if(!(held&(BTN_UP|BTN_DOWN))) repeat_at=now+0.3;
        if(!running) {
            bool was_testing=input_test;
            if(pressed&BTN_HELP) input_test=!input_test;
            if(was_testing && (pressed&BTN_BACK)) input_test=false;
            if(was_testing || input_test) { menu_draw(); goto present_frame; }
            if(game_count && (pressed&BTN_UP)) selected=(selected+game_count-1)%game_count;
            if(game_count && (pressed&BTN_DOWN)) selected=(selected+1)%game_count;
            if(game_count && (pressed&BTN_PREV)) selected=selected>=6?selected-6:0;
            if(game_count && (pressed&BTN_NEXT)) selected=selected+6<game_count?selected+6:game_count-1;
#ifdef NPS64_UPDATES
            if(pressed&BTN_UPDATE) nps64_updater_request();
#endif
            if(pressed&BTN_SCAN) {
#ifdef NPS64_HD_UI
                covers_refresh();
#endif
                game_count=library_scan(games_dir,games,512); selected=0;
                char text[160]; snprintf(text,sizeof(text),"Biblioteca actualizada: %zu ROMs. Pulsa X para abrir",game_count); notify(text);
            }
            if(pressed&(BTN_LEFT|BTN_RIGHT)) { theme=(theme+((pressed&BTN_RIGHT)?1:2))%3; config_save(); }
            if((pressed&BTN_CONFIRM) && game_count) start_game();
            if(pressed&BTN_BACK) notify("Ya estas en la biblioteca. X: abrir | Cuadrado: buscar");
            if(pressed&BTN_EXIT) quit_requested=true;
            if(!running) menu_draw();
        }
        if(running) {
#ifdef NPS64_HLE_GAME
            if(hle_preview) {
                hle_preview_draw(0);
                if(pressed&BTN_BACK)hle_preview=false;
                goto present_frame;
            }
            bool snapshot_requested=false;
#ifndef __ORBIS__
            snapshot_requested=getenv("BENCH_HLE_PREVIEW")!=NULL&&getenv("BENCH_HLE_LIVE")==NULL;
#endif
            if(paused&&(pressed&BTN_HELP)&&!snapshot_requested){
                if(hle_renderer_failed)notify("Vulkan fallo; reinicia la app para reintentar");
                else{
                    hle_continuous=!hle_continuous;hle_frame_ready=false;paused=false;memset(hle_scanouts,0,sizeof(hle_scanouts));hle_display_vi.width=0;
                    platform_audio_clear();
                    nps64_log("[hle-live] mode=%s; audio and controls active; CPU reference retained\n",hle_continuous?"Vulkan":"CPU");
                    notify(hle_continuous?"Vulkan continuo activo (referencia CPU incluida)":"Renderer CPU activo");
                }
            }
            if(paused&&(pressed&BTN_HELP)&&snapshot_requested) {
                if(hle_preview_used)notify("La prueba Vulkan se permite una vez por inicio de la app");
                else {
                    if(!nps64_log_ok()){nps64_log_close();nps64_log_init(DATA_ROOT "/hle-error.log");}
                    notify("Preparando secuencia de geometria Vulkan...");
                    game_video_draw();game_hud_draw();platform_present();
                    nps64_hle_sequence_active=true;
                    HleSceneFrame first={0};
                    int status=hle_sequence_next(0,&first,NULL);
                    if(status==1){
                        hle_preview_used=true;
                        HleSequence sequence={.next=hle_sequence_next,.present=hle_sequence_present,.context=&first,.frames=64};
                        status=nps64_hle_scene_sequence(&sequence,hle_pixels);
                    }else status=2;
                    nps64_hle_sequence_active=false;
                    if(status&&!nps64_log_ok()){nps64_log_close();nps64_log_init(DATA_ROOT "/hle-error.log");}
                    nps64_log("[hle-game] snapshot returned=%d\n",status);
                    if(!status)hle_preview=true;
                    else if(status==2)notify("Sin geometria tras 6 VI. Puedes reintentar en otra escena.");
                    else notify("Fallo Vulkan; detalle en /data/NPS64/nps64.log o hle-error.log");
                }
            }
#endif
            for(unsigned i=0;i<4;i++) if(connected[i]!=pads[i].connected) { connected[i]=pads[i].connected; retro_set_controller_port_device(i,connected[i]?RETRO_DEVICE_JOYPAD:RETRO_DEVICE_NONE); }
            if(pressed&BTN_PAUSE) { paused=!paused; pause_selected=0; platform_audio_clear(); }
            if(paused) {
                if(pressed&BTN_UP) pause_selected=(pause_selected+PAUSE_CHOICES-1)%PAUSE_CHOICES;
                if(pressed&BTN_DOWN) pause_selected=(pause_selected+1)%PAUSE_CHOICES;
                if(pressed&BTN_LEFT) {slot=(slot+7)%8;config_save();}
                if(pressed&BTN_RIGHT) {slot=(slot+1)%8;config_save();}
                if(pressed&BTN_BACK) paused=false;
                if(pressed&BTN_CONFIRM) {
                    if(pause_selected==0) paused=false;
                    else if(pause_selected==1) state_action(false);
                    else if(pause_selected==2) state_action(true);
                    else if(pause_selected==3) { widescreen=!widescreen;config_save(); }
                    else if(pause_selected==4) stop_game();
                    else if(pause_selected==5) {filtered_vi=!filtered_vi;options_dirty=true;config_save();notify(filtered_vi?"Imagen precisa":"Imagen rapida: filtro VI desactivado");}
#ifdef NPS64_UPDATES
                    else if(pause_selected==6) nps64_updater_request();
#endif
                }
            }
            if(running && !paused) {
                if(core_frames<3) nps64_log("[run] frame=%llu begin\n",(unsigned long long)core_frames);
                double core_start=platform_time();
#ifdef NPS64_HLE_GAME
                int defer_vi=hle_continuous&&native_width&&native_height;
#ifndef __ORBIS__
                if(getenv("NPS64_COMPARE_DIR")||getenv("NPS64_HLE_CPU_VI"))defer_vi=0;
#endif
                nps64_hle_vi_defer_enable(defer_vi);
                nps64_hle_geometry_capture_textures(hle_continuous);
                nps64_hle_geometry_begin();
#endif
                retro_run();
                core_elapsed=platform_time()-core_start;
                if(core_frames<3) nps64_log("[run] frame=%llu end video=%d\n",(unsigned long long)core_frames,first_video);
                core_frames++;
#ifdef NPS64_HLE_GAME
                if(hle_continuous&&!core_shutdown){double gpu_start=platform_time();hle_game_frame();gpu_elapsed=platform_time()-gpu_start;}
                int vi_finish=nps64_hle_cpu_vi_finish(!hle_continuous||!hle_frame_ready);
                if(vi_finish==2)video_frames++; /* GPU supplied this VI instead. */
                if(vi_finish==1||vi_finish==2){
                    uint64_t deferred,resolved;nps64_hle_cpu_vi_stats(&deferred,&resolved);
                    if(deferred==1||deferred%120==0||vi_finish==1)nps64_log("[hle-live] CPU VI deferred=%llu fallback_resolved=%llu\n",(unsigned long long)deferred,(unsigned long long)resolved);
                }
#ifndef __ORBIS__
                if(getenv("NPS64_HLE_FRAME_HASHES")&&hle_continuous){
                    const unsigned char *data=hle_frame_ready?hle_pixels:native_frame;
                    size_t bytes=hle_frame_ready?sizeof(hle_pixels):(size_t)native_width*native_height*(native_format==RETRO_PIXEL_FORMAT_XRGB8888?4:2);
                    uint64_t hash=1469598103934665603ull;
                    for(size_t i=0;data&&i<bytes;i++){hash^=data[i];hash*=1099511628211ull;}
                    nps64_log("[hle-hash] frame=%llu GPU=%u bytes=%zu value=%016llx\n",(unsigned long long)core_frames,hle_frame_ready?1:0,bytes,(unsigned long long)hash);
                }
#endif
                nps64_hle_geometry_capture_textures(0);
#endif
                if(now-last_autosave>30) { save_ram();last_autosave=now; }
            }
            if(running) {
#ifdef NPS64_HLE_GAME
                if(hle_continuous&&hle_frame_ready)hle_game_draw();else game_video_draw();
#else
                game_video_draw();
#endif
                game_hud_draw();
                if(paused) pause_draw();
#ifdef NPS64_HLE_GAME
                if(paused)platform_text(32,680,hle_continuous?"Triangulo: volver a CPU y continuar":"Triangulo: activar Vulkan continuo y continuar",0xffffffff,1);
#endif
                else {
                    if(!visible_video) {
                    char boot[160];snprintf(boot,sizeof(boot),"Arrancando: frame %llu | %.1f FPS | log %s",(unsigned long long)core_frames,ui_fps,nps64_log_ok()?"OK":"ERROR");
                    platform_text(64,677,boot,accent[theme],1);
                    }
                }
            }
            else menu_draw();
            if(core_shutdown && running) stop_game();
        }
present_frame:
        ; bool measuring_game=running && !paused;
        if(perf_game!=measuring_game || core_frames<perf_core_frames || video_frames<perf_video_frames) {
            perf_since=platform_time();poll_sum=core_sum=draw_sum=flip_sum=gpu_sum=0;perf_samples=0;rdp_sum=vi_sum=pace_sum=pace_overshoot_sum=0;late_frames=0;pace_clamps=0;core_peak=rdp_peak=0;
            perf_core_frames=core_frames;perf_video_frames=video_frames;perf_game=measuring_game;
        }
        double draw_elapsed=platform_time()-draw_start-core_elapsed;
        double flip_start=platform_time();
        if(!platform_present()) { notify("Error presentando imagen"); break; }
#ifdef NPS64_HLE_GAME
        if(hle_preview&&!hle_preview_displayed){nps64_log("[hle-game] result screen presented\n");hle_preview_displayed=true;}
#endif
        double rdp_time,vi_time;nps64_core_perf_take(&rdp_time,&vi_time);rdp_sum+=rdp_time;vi_sum+=vi_time;
        if(core_elapsed>core_peak)core_peak=core_elapsed;
        if(rdp_time>rdp_peak)rdp_peak=rdp_time;
        gpu_sum+=gpu_elapsed;poll_sum+=poll_elapsed;core_sum+=core_elapsed;draw_sum+=draw_elapsed;flip_sum+=platform_time()-flip_start;perf_samples++;
        double perf_elapsed=platform_time()-perf_since;
        if(perf_elapsed>=1) {
            ui_fps=perf_samples/perf_elapsed;
            game_fps=(core_frames-perf_core_frames)/perf_elapsed;video_fps=(video_frames-perf_video_frames)/perf_elapsed;
            perf_poll_ms=1000*poll_sum/perf_samples;perf_core_ms=1000*core_sum/perf_samples;
            perf_rdp_ms=1000*rdp_sum/perf_samples;perf_vi_ms=1000*vi_sum/perf_samples;
            perf_gpu_ms=1000*gpu_sum/perf_samples;perf_draw_ms=1000*draw_sum/perf_samples;perf_flip_ms=1000*flip_sum/perf_samples;
            nps64_log("[perf] state=%s fps=%.2f vi_fps=%.2f video_fps=%.2f target=%.2f speed_pct=%.1f cpu=%s workers=%u requested_workers=%u vi_mode=%s rdp_ms=%.3f vi_ms=%.3f poll_ms=%.3f core_ms=%.3f draw_ms=%.3f flip_ms=%.3f gpu_ms=%.3f pace_ms=%.3f sleep_over_ms=%.3f late=%u frames=%llu video=%d visible=%d\n",running?(paused?"pause":"game"):"library",ui_fps,game_fps,video_fps,av.timing.fps,av.timing.fps>0?100*game_fps/av.timing.fps:0,use_jit?"Ari64":"cached",nps64_core_workers(),diagnostic_workers,filtered_vi?"filtered":"fast",perf_rdp_ms,perf_vi_ms,perf_poll_ms,perf_core_ms,perf_draw_ms,perf_flip_ms,perf_gpu_ms,1000*pace_sum/perf_samples,1000*pace_overshoot_sum/perf_samples,late_frames,(unsigned long long)core_frames,first_video,visible_video);
            nps64_log("[pacing] debt_clamps=%u core_peak_ms=%.3f rdp_peak_ms=%.3f\n",pace_clamps,1000*core_peak,1000*rdp_peak);
            perf_since=platform_time();poll_sum=core_sum=draw_sum=flip_sum=gpu_sum=0;perf_samples=0;rdp_sum=vi_sum=pace_sum=pace_overshoot_sum=0;late_frames=0;pace_clamps=0;core_peak=rdp_peak=0;perf_core_frames=core_frames;perf_video_frames=video_frames;
        }
        double period=running && !paused && av.timing.fps>1?1.0/av.timing.fps:1.0/60.0;
        double after=platform_time();
        if(deadline+period<after-period)pace_clamps++;
        deadline=nps64_frame_deadline(deadline,after,period);
        double requested_sleep=deadline>after?deadline-after:0;
        if(!requested_sleep)late_frames++;
        platform_sleep(requested_sleep);
        double slept=platform_time()-after;
        pace_sum+=slept;
        if(requested_sleep && slept>requested_sleep)pace_overshoot_sum+=slept-requested_sleep;
    }
#ifdef NPS64_UPDATES
    nps64_updater_shutdown();
#endif
#ifdef NPS64_HD_UI
    library_ui_shutdown();
#endif
    if(running) stop_game();
#ifdef NPS64_HLE_GAME
    nps64_hle_renderer_release(hle_renderer);
#endif
    config_save(); platform_shutdown(); free(native_frame); nps64_log_close(); return 0;
}

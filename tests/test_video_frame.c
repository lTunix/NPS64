#define main frontend_main
#include "../src/frontend.c"
#undef main
#include <assert.h>
static uint32_t fb[SCREEN_W*SCREEN_H];
static unsigned char *staging;
static int fail_readback;
static unsigned snapshots;
uint32_t *platform_framebuffer(void){return fb;}
double platform_time(void){return 0;}
void nps64_log(const char *fmt,...){(void)fmt;}
bool nps64_gpu_failed(void){return fail_readback!=0;}
void nps64_gpu_perf_snapshot(double t){(void)t;snapshots++;}
bool nps64_gpu_readback(const void **p,unsigned w,unsigned h){
 free(staging);staging=NULL;
 if(fail_readback)return false;
 staging=malloc((size_t)w*h*4);assert(staging);
 for(size_t i=0;i<(size_t)w*h;i++)((uint32_t*)staging)[i]=0xff112233;
 *p=staging;return true;
}
int main(void){
 video_refresh(RETRO_HW_FRAME_BUFFER_VALID,2,2,0);
 assert(native_video_pixels()==staging&&native_frame==NULL&&native_capacity==0&&snapshots==0);
 game_video_draw();assert(fb[360*SCREEN_W+640]==0xff332211);
 game_video_draw();assert(fb[360*SCREEN_W+640]==0xff332211);
 video_refresh(RETRO_HW_FRAME_BUFFER_VALID,1280,576,0);
 assert(native_video_pixels()==staging&&native_frame==NULL);game_video_draw();assert(fb[360*SCREEN_W+640]==0xff332211);
 fail_readback=1;video_refresh(RETRO_HW_FRAME_BUFFER_VALID,2,2,0);assert(native_borrowed==NULL&&native_width==0);game_video_draw();assert(fb[0]==0);
 uint32_t cpu[4]={0xffabcdef,0xffabcdef,0xffabcdef,0xffabcdef};pixel_format=RETRO_PIXEL_FORMAT_XRGB8888;
 video_refresh(cpu,2,2,8);memset(cpu,0,sizeof(cpu));assert(native_borrowed==NULL&&native_frame!=NULL);game_video_draw();assert(fb[360*SCREEN_W+640]==0xffabcdef);
 free(native_frame);free(staging);
 puts("PASS frame lifetime: borrowed Vulkan storage, pause reuse, resolution change, failed readback invalidation, owned CPU callback, zero snapshot copies");
}

/* Hardware experiment only, not a complete N64 HLE renderer. */
#include "platform.h"
#include "log.h"
#include <stdlib.h>
#include <sys/stat.h>
extern void nps64_gpu_static_init(void);
extern void nps64_boot_trace(const char*);
extern int nps64_hle_graphics_probe(unsigned char*);
int main(void) {
    mkdir("/data/NPS64",0777);
    nps64_log_init("/data/NPS64/hle-probe.log");
    nps64_boot_trace("00.23 HLE graphics probe main");
    nps64_log("NPS64 00.23 HLE GRAPHICS PROBE; no game renderer and no compute dispatch\n");
    if(!platform_init())return 1;
    platform_text(40,30,"NPS64 00.23 - Comprobando GPU Vulkan...",0xffffffff,2);
    if(!platform_present())return 1;
    nps64_log("[hle-probe] startup screen presented\n");
    nps64_gpu_static_init();
    setenv("RADV_PS4_SERIAL_SUBMIT","1",1);setenv("RADV_PS4_TRACE","1",1);
    static unsigned char pixels[256*256*4];
    int result=nps64_hle_graphics_probe(pixels);
    nps64_log("[hle-probe] returned result=%d; entering display loop\n",result);
    Input pads[4];
    unsigned displayed=0;
    for(;;) {
        uint32_t *fb=platform_framebuffer();
        for(unsigned y=0;y<SCREEN_H;y++)for(unsigned x=0;x<SCREEN_W;x++) {
            if(!result&&x>=384&&x<896&&y>=120&&y<632) {
                unsigned p=(((y-120)/2)*256+(x-384)/2)*4;
                fb[y*SCREEN_W+x]=0xff000000u|((uint32_t)pixels[p]<<16)|((uint32_t)pixels[p+1]<<8)|pixels[p+2];
            }else fb[y*SCREEN_W+x]=0xff102030u;
        }
        platform_text(40,30,"NPS64 00.23 - Lista N64 a Vulkan",0xffffffff,2);
        platform_text(40,70,"F3DEX2: matrices y vertices N64. Lista controlada; sin ROMs.",0xffffffff,1);
        platform_text(40,650,result?"FALLO: consultar /data/NPS64/hle-probe.log":"PASS: lista F3DEX2 a Vulkan; 128 frames verificados",result?0xffff6060:0xff60ff60,1);
        platform_text(40,680,"Circulo: salir. Conservar NPS64 00.13 para jugar.",0xffffffff,1);
        platform_poll(pads);if(pads[0].buttons&(BTN_BACK|BTN_EXIT))break;
        if(!platform_present()){nps64_log("[hle-probe] presentation failed\n");break;}
        if(displayed++==0)nps64_log("[hle-probe] result screen presented\n");
    }
    platform_shutdown();nps64_log_close();return result;
}

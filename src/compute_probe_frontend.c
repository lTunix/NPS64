/* Isolated GPU diagnostic: no emulator core, games or save-data writes.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "platform.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <orbis/libkernel.h>
extern void nps64_gpu_static_init(void);
extern int nps64_compute_probe(const char *,unsigned,unsigned);
int main(void) {
    sceKernelMkdir("/data/NPS64",0777);
    nps64_log_init("/data/NPS64/compute-probe.log");
#ifdef NPS64_MULTIWAVE_PROBE
    nps64_log("NPS64 multiwave probe 00.03; runtime mask; isolated title NPSG00064\n");
#else
    nps64_log("NPS64 compute probe 00.01; isolated title NPSG00064\n");
#endif
    if(!platform_init()){nps64_log("[compute-probe] platform failed\n");return 1;}
    unsigned selected=0,previous=0,passed=0;
    int failed=0,initialized=0;
    const char *names[]={"Storage buffer / 64 hilos","LDS + atomics + barreras / 32 hilos","LDS + atomics + barreras / 64 hilos"
#ifdef NPS64_MULTIWAVE_PROBE
        ,"LDS entre waves / 128 hilos","LDS entre waves / 256 hilos"
#endif
    };
    const unsigned cases=sizeof(names)/sizeof(*names);
    char status[100]="Selecciona prueba y pulsa X. Circulo: salir.";
    for(;;) {
        Input pads[4];platform_poll(pads);
        unsigned held=pads[0].buttons,pressed=held&~previous;previous=held;
        if(pressed&(BTN_BACK|BTN_EXIT))break;
        if(pressed&BTN_UP)selected=(selected+cases-1)%cases;
        if(pressed&BTN_DOWN)selected=(selected+1)%cases;
        if((pressed&BTN_CONFIRM)&&!failed) {
            memset(platform_framebuffer(),0,SCREEN_W*SCREEN_H*4);
            platform_text(32,60,"Prueba GPU en curso. Consultar compute-probe.log",0xffffffff,1);
            if(!platform_present())break;
            nps64_log("[compute-probe] user selected=%u\n",selected);
            if(!initialized){nps64_gpu_static_init();initialized=1;}
            double start=platform_time();
            int result=nps64_compute_probe("/app0/probe.spv",selected,1);
            nps64_log("[compute-probe] result=%d test=%u elapsed_ms=%.3f\n",result,selected,1000*(platform_time()-start));
            if(result){failed=1;snprintf(status,sizeof(status),"FALLO. No repetir. Cierra la prueba y conserva el log.");}
            else {passed|=1u<<selected;snprintf(status,sizeof(status),"PASS prueba %u. Selecciona la siguiente o pulsa Circulo.",selected+1);}
        }
        memset(platform_framebuffer(),0,SCREEN_W*SCREEN_H*4);
#ifdef NPS64_MULTIWAVE_PROBE
        platform_text(32,30,"NPS64 - Vulkan Multiwave Test 00.03",0xffffffff,2);
#else
        platform_text(32,30,"NPS64 - Prueba Vulkan compute 00.01",0xffffffff,2);
#endif
        platform_text(32,90,"Diagnostico GPU; no mide FPS de juegos.",0xffb0b0b0,1);
        for(unsigned i=0;i<cases;i++){
            char line[120];snprintf(line,sizeof(line),"%c %s %s",i==selected?'>':' ',names[i],passed&(1u<<i)?"PASS":"");
            platform_text(32,140+(int)i*40,line,i==selected?0xff60ff60:0xffffffff,1);
        }
        platform_text(32,cases>3?400:300,status,failed?0xffff6060:0xffffffff,1);
        platform_text(32,cases>3?450:350,"Si falla el fence no se garantiza recuperar una GPU bloqueada.",0xffb0b0b0,1);
        if(!platform_present())break;
        platform_sleep(1.0/60);
    }
    nps64_log("[compute-probe] exit passed_mask=%u failed=%d\n",passed,failed);
    platform_shutdown();nps64_log_close();return failed?1:0;
}

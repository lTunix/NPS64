/* Compile/statistics test. No RDP emulator and no command submission APIs.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "platform.h"
#include "log.h"
#include "compile_probe_cases.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <orbis/libkernel.h>
extern void nps64_gpu_static_init(void);
extern int nps64_compile_captured_pipeline(const char *);
int main(void) {
    sceKernelMkdir("/data/NPS64",0777);
    nps64_log_init("/data/NPS64/pipeline-compile.log");
#ifdef NPS64_GUARD_COMPARE
    nps64_log("NPS64 Vulkan Guard Compare 00.04; pipeline=52bbce7140cf109d; no dispatch or queue submission\n");
#else
    nps64_log("NPS64 Vulkan Compile Test 00.02; no dispatch or queue submission\n");
#endif
    if(!platform_init())return 1;
    unsigned previous=0,selected=0,done=0;int initialized=0,failed=0;
    char status[128]="X: compilar grupo. Circulo: salir.";
    for(;;) {
        Input pads[4];platform_poll(pads);unsigned held=pads[0].buttons,pressed=held&~previous;previous=held;
        if(pressed&(BTN_BACK|BTN_EXIT))break;
        if(pressed&(BTN_UP|BTN_DOWN))selected^=1;
        if((pressed&BTN_CONFIRM)&&!failed) {
#ifdef NPS64_GUARD_COMPARE
            if(setenv("NPS64_COMPILE_WITH_GUARD",selected?"1":"0",1))return 1;
#endif
            if(!initialized){nps64_gpu_static_init();initialized=1;}
            unsigned count=0;
            for(unsigned i=0;i<sizeof(compile_groups)/sizeof(*compile_groups);i++) {
                if(compile_groups[i]!=selected)continue;
                memset(platform_framebuffer(),0,SCREEN_W*SCREEN_H*4);
                platform_text(32,60,"Compilando shader. No se ejecuta el kernel.",0xffffffff,1);
                platform_text(32,100,compile_cases[i]+7,0xff60ff60,1);
                if(!platform_present()){failed=1;break;}
                nps64_log("[pipeline-case] begin group=%u file=%s\n",selected,compile_cases[i]);
                double start=platform_time();int result=nps64_compile_captured_pipeline(compile_cases[i]);
                nps64_log("[pipeline-case] end result=%d elapsed_ms=%.3f file=%s\n",result,1000*(platform_time()-start),compile_cases[i]);
                if(result){failed=1;break;}count++;
            }
            if(failed)snprintf(status,sizeof(status),"Fallo de compilacion. Conservar pipeline-compile.log.");
            else {done|=1u<<selected;snprintf(status,sizeof(status),"Grupo %u: %u shaders compilados. Estadisticas guardadas.",selected+1,count);}
        }
        memset(platform_framebuffer(),0,SCREEN_W*SCREEN_H*4);
#ifdef NPS64_GUARD_COMPARE
        platform_text(32,30,"NPS64 - Guard Compare 00.04",0xffffffff,2);
#else
        platform_text(32,30,"NPS64 - Vulkan Compile Test 00.02",0xffffffff,2);
#endif
        platform_text(32,90,"Solo compilacion y recursos; no mide FPS ni ejecuta RDP.",0xffb0b0b0,1);
        for(unsigned i=0;i<2;i++) {
#ifdef NPS64_GUARD_COMPARE
            const char *label=i?"52bb: con comprobacion de recursos":"52bb: sin comprobacion de recursos";
#else
            const char *label=i?"Etapas separadas (23 shaders)":"Ubershader anterior (2 variantes)";
#endif
            char text[128];snprintf(text,sizeof(text),"%c %s %s",selected==i?'>':' ',label,done&(1u<<i)?"PASS":"");
            platform_text(32,160+(int)i*50,text,selected==i?0xff60ff60:0xffffffff,1);
        }
        platform_text(32,320,status,failed?0xffff6060:0xffffffff,1);
        if(!platform_present())break;platform_sleep(1.0/60);
    }
    nps64_log("[pipeline-compile] exit done=%u failed=%d\n",done,failed);
    platform_shutdown();nps64_log_close();return failed?1:0;
}

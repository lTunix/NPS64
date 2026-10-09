/* Independent compute experiment. Does not load a ROM or replace CPU NPS64. */
#include "parallel_compute_probe.h"
#include "platform.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
extern void nps64_gpu_static_init(void);
extern void nps64_boot_trace(const char *message);

static char last_message[512];
static void report(const char *message) {
    snprintf(last_message,sizeof(last_message),"%s",message);
    nps64_log("[parallel-probe] %s\n",message);
    /* Persist the last operation before a possible physical GPU stall. */
    nps64_boot_trace(message);
}
static void draw(unsigned selected,const unsigned results[PARALLEL_PROBE_CASES],const char *status,int failed) {
    uint32_t *fb=platform_framebuffer();
    for(unsigned i=0;i<SCREEN_W*SCREEN_H;i++)fb[i]=0xff102030u;
#ifdef NPS64_PARALLEL_TILE_PROBE
    platform_text(40,30,"NPS64 00.54 - ParaLLEl: captura real de tiles",0xffffffff,2);
    platform_text(40,80,"Despacho aislado del bloqueo 331. Empezar con el caso 2 (64 lanes).",0xffa0c0e0u,1);
#else
    platform_text(40,30,"NPS64 00.52 - ParaLLEl: prueba de compute",0xffffffff,2);
    platform_text(40,80,"Sin ROM: primero verificamos las operaciones GPU que usa el RDP.",0xffa0c0e0u,1);
#endif
    for(unsigned i=0;i<PARALLEL_PROBE_CASES;i++){
        char text[160];snprintf(text,sizeof(text),"%s %u. %s [%s]",i==selected?">":" ",i+1,
            parallel_probe_case_name(i),results[i]==1?"PASS":results[i]==2?"FAIL":"pendiente");
        platform_text(40,145+(int)i*43,text,results[i]==2?0xffff6060u:results[i]==1?0xff60ff60u:0xffffffffu,1);
    }
    platform_text(40,490,status,failed?0xffff6060u:0xff60ff60u,1);
    char short_message[110];snprintf(short_message,sizeof(short_message),"%.100s",last_message);
    platform_text(40,540,short_message,0xffa0c0e0u,1);
    platform_text(40,605,"Arriba/abajo: elegir. X: ejecutar 64 envios y verificar resultados.",0xffffffffu,1);
#ifdef NPS64_PARALLEL_TILE_PROBE
    platform_text(40,640,"Logs: /data/NPS64-Parallel/tile-probe.log y /data/NPS64/boot.log",0xffffffffu,1);
#else
    platform_text(40,640,"Logs: /data/NPS64/parallel-probe.log y /data/NPS64/boot.log",0xffffffffu,1);
#endif
    platform_text(40,675,"Circulo: salir. La app CPU 00.51 permanece instalada por separado.",0xffffffffu,1);
}
int main(void) {
    mkdir("/data/NPS64",0777);
#ifdef NPS64_PARALLEL_TILE_PROBE
    mkdir("/data/NPS64-Parallel",0777);
    nps64_log_init("/data/NPS64-Parallel/tile-probe.log");
    report("NPS64 00.54 ORIGINAL TILE CAPTURE; isolated post-hang inputs; no ROM/core");
#else
    nps64_log_init("/data/NPS64/parallel-probe.log");
    report("NPS64 00.52 PARALLEL COMPUTE GATE; manual cases; no ROM/core");
#endif
    if(!platform_init())return 1;
    setenv("RADV_PS4_SERIAL_SUBMIT","1",1);
    setenv("RADV_PS4_TRACE","1",1);
    ParallelComputeProbe *probe=NULL;
#ifdef NPS64_PARALLEL_TILE_PROBE
    unsigned selected=1,results[PARALLEL_PROBE_CASES]={0},previous=0;
    int failed=0;const char *status="Caso 2 seleccionado: mismo binning con grupo completo de 64.";
#else
    unsigned selected=0,results[PARALLEL_PROBE_CASES]={0},previous=0;
    int failed=0;const char *status="Comenzar con el caso 1: escrituras a buffer.";
#endif
    for(;;) {
        draw(selected,results,status,failed);if(!platform_present())break;
        Input pads[4];platform_poll(pads);
        unsigned pressed=pads[0].buttons&~previous;previous=pads[0].buttons;
        if(pressed&(BTN_BACK|BTN_EXIT))break;
        if(pressed&BTN_UP)selected=(selected+PARALLEL_PROBE_CASES-1)%PARALLEL_PROBE_CASES;
        if(pressed&BTN_DOWN)selected=(selected+1)%PARALLEL_PROBE_CASES;
        if((pressed&BTN_CONFIRM)&&!failed) {
            status="Ejecutando: cada envio debe completar y dar valores exactos.";
            draw(selected,results,status,0);if(!platform_present())break;
            if(!probe){nps64_gpu_static_init();failed=parallel_probe_open(&probe,report);}
            if(!failed)failed=parallel_probe_run(probe,selected,PARALLEL_PROBE_REPEATS);
            results[selected]=failed?2:1;
            unsigned passed=0;for(unsigned i=0;i<PARALLEL_PROBE_CASES;i++)passed+=results[i]==1;
            status=failed?"FAIL: no se enviara mas trabajo GPU; consultar logs.":
#ifdef NPS64_PARALLEL_TILE_PROBE
                passed==PARALLEL_PROBE_CASES?"PASS: ambas variantes producen las mascaras completas de referencia.":
#else
                passed==PARALLEL_PROBE_CASES?"PASS: siete casos completos. Siguiente paso: cargas reales RDP.":
#endif
                "PASS: fence, resultados y guardas correctos. Elegir siguiente caso.";
            report(status);
        }
        platform_sleep(1.0/60.0);
    }
    parallel_probe_close(probe);platform_shutdown();nps64_log_close();return 0;
}

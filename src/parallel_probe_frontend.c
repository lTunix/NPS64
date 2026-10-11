/* Independent compute experiment. Does not load a ROM or replace CPU NPS64. */
#include "parallel_compute_probe.h"
#include "platform.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef NPS64_PARALLEL_TILE_PROBE
#include <pthread.h>
#endif
extern void nps64_gpu_static_init(void);
extern void nps64_boot_trace(const char *message);

static char last_message[512];
static void report(const char *message) {
    snprintf(last_message,sizeof(last_message),"%s",message);
    nps64_log("[parallel-probe] %s\n",message);
    /* Persist the last operation before a possible physical GPU stall. */
    nps64_boot_trace(message);
}
#ifdef NPS64_PARALLEL_TILE_PROBE
typedef struct {ParallelComputeProbe **probe;unsigned selected;int result;} TileTask;
static void *tile_worker(void *argument){
    TileTask *task=argument;
    report("WORKER started; configured stack=8 MiB");
    if(!*task->probe){nps64_gpu_static_init();task->result=parallel_probe_open(task->probe,report);}
    if(!task->result)task->result=parallel_probe_run(*task->probe,task->selected,PARALLEL_PROBE_REPEATS);
    return NULL;
}
static int execute_tile(ParallelComputeProbe **probe,unsigned selected){
    pthread_attr_t attr;int result=pthread_attr_init(&attr);
    if(result){report("FAIL worker attribute initialization");return 1;}
    result=pthread_attr_setstacksize(&attr,8*1024*1024);
    pthread_t thread;TileTask task={probe,selected,0};
    if(!result){report("WORKER create with explicit 8 MiB stack");result=pthread_create(&thread,&attr,tile_worker,&task);}
    pthread_attr_destroy(&attr);
    if(result){char message[128];snprintf(message,sizeof(message),"FAIL worker create/stack result=%d",result);report(message);return 1;}
    result=pthread_join(thread,NULL);
    if(result){report("FAIL worker join");return 1;}
    return task.result;
}
#endif
static void draw(unsigned selected,const unsigned results[PARALLEL_PROBE_CASES],const char *status,int failed) {
    uint32_t *fb=platform_framebuffer();
    for(unsigned i=0;i<SCREEN_W*SCREEN_H;i++)fb[i]=0xff102030u;
#ifdef NPS64_PARALLEL_TILE_PROBE
    platform_text(40,30,"NPS64 00.60 - ParaLLEl: captura real de tiles",0xffffffff,2);
    platform_text(40,80,"Ejecutar caso 7: entradas volatiles y scissor leido por el shader.",0xffa0c0e0u,1);
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
    setenv("NPS64_TILE_COMPILER_TRACE","1",1);
    report("NPS64 00.60 ORIGINAL TILE CAPTURE; isolated post-hang inputs; no ROM/core");
#else
    nps64_log_init("/data/NPS64/parallel-probe.log");
    report("NPS64 00.52 PARALLEL COMPUTE GATE; manual cases; no ROM/core");
#endif
    if(!platform_init())return 1;
    setenv("RADV_PS4_SERIAL_SUBMIT","1",1);
    setenv("RADV_PS4_TRACE","1",1);
    ParallelComputeProbe *probe=NULL;
#ifdef NPS64_PARALLEL_TILE_PROBE
    unsigned selected=6,results[PARALLEL_PROBE_CASES]={0},previous=0;
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
#ifdef NPS64_PARALLEL_TILE_PROBE
            failed=execute_tile(&probe,selected);
#else
            if(!probe){nps64_gpu_static_init();failed=parallel_probe_open(&probe,report);}
            if(!failed)failed=parallel_probe_run(probe,selected,PARALLEL_PROBE_REPEATS);
#endif
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

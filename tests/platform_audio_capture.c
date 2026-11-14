/* Real-core PCM capture for comparing audio modes with identical inputs. */
#define platform_poll benchmark_poll
#define platform_audio benchmark_audio
#define platform_shutdown benchmark_shutdown
#include "platform_bench.c"
#undef platform_poll
#undef platform_audio
#undef platform_shutdown
#include <libretro.h>
#include "plugin/plugin.h"
extern rsp_plugin_functions rsp_audio,rsp_cxd4,rsp_hle;
#include "audio_resampler.h"
static retro_environment_t frontend_environment;
static FILE *capture,*metrics;
static Nps64AudioResampler resampler;
static size_t input_total,output_total;
static bool audio_environment(unsigned command,void *data){
 if(command==RETRO_ENVIRONMENT_GET_VARIABLE){struct retro_variable *v=data;const char *mode=getenv("CAPTURE_AUDIO_MODE");
  if(mode&&!strcmp(v->key,"parallel-n64-send_allist_to_hle_rsp")){v->value=mode;printf("CAPTURE audio option=%s\n",mode);return true;}
 }
 return frontend_environment(command,data);
}
void __real_retro_set_environment(retro_environment_t);
void __wrap_retro_set_environment(retro_environment_t callback){frontend_environment=callback;__real_retro_set_environment(audio_environment);}
static void capture_emit(void *unused,int16_t l,int16_t r){(void)unused;int16_t sample[]={l,r};if(capture)fwrite(sample,4,1,capture);output_total++;}
void platform_audio(const int16_t *s,size_t n,double r){
 benchmark_audio(s,n,r);
 static int first;if(!first){printf("CAPTURE selected audio_rsp_mode=%d actual=%s\n",audio_rsp_mode,rsp_audio.doRspCycles==rsp_cxd4.doRspCycles?"LLE":rsp_audio.doRspCycles==rsp_hle.doRspCycles?"HLE":"unknown");first=1;}
 if(!capture){const char *path=getenv("CAPTURE_PCM");if(path){capture=fopen(path,"wb");char name[1024];snprintf(name,sizeof name,"%s.csv",path);metrics=fopen(name,"w");if(metrics)fprintf(metrics,"batch,rate,frames,zeros,max_step\n");}}
 size_t zeros=0;int jump=0;for(size_t i=0;i<n;i++){zeros+=s[2*i]==0&&s[2*i+1]==0;if(i){int d=abs(s[2*i]-s[2*i-2]);if(d>jump)jump=d;}}
 if(metrics)fprintf(metrics,"%zu,%.9f,%zu,%zu,%d\n",audio_batches,r,n,zeros,jump);
 input_total+=n;nps64_resample(&resampler,s,n,r,capture_emit,NULL);
}
void platform_shutdown(void){if(capture)fclose(capture);if(metrics)fclose(metrics);benchmark_shutdown();printf("CAPTURE input=%zu output48k=%zu\n",input_total,output_total);}

void platform_poll(Input pads[4]) {
 benchmark_poll(pads);
 if(!getenv("CAPTURE_RELOAD"))return;
 if(polls==520||polls==700)pads[0].buttons=BTN_PAUSE;
 if(polls==522||polls==524||polls==526||polls==528||polls==702||polls==704||polls==706||polls==708)pads[0].buttons=BTN_DOWN;
 if(polls==530||polls==532||polls==710)pads[0].buttons=BTN_CONFIRM;
 if(polls==712)pads[0].buttons=BTN_EXIT;
}

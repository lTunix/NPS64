/* Exercise the actual patched libretro backend, including DMA-byte conversion. */
#include <libretro.h>
#include "plugin/audio_libretro/audio_plugin.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static size_t received,nonzero;
static double published_rate;
static size_t receive(const int16_t *samples,size_t frames) {
    received+=frames;
    for(size_t i=0;i<frames*2;i++)nonzero+=samples[i]!=0;
    return frames;
}
static bool environment(unsigned command,void *data) {
    if(command==RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO)
        published_rate=((struct retro_system_av_info*)data)->timing.sample_rate;
    return true;
}
retro_audio_sample_batch_t audio_batch_cb=receive;
retro_environment_t environ_cb=environment;
void retro_get_system_av_info(struct retro_system_av_info *info) {memset(info,0,sizeof(*info));}
int main(void) {
    unsigned char source[960*4];memset(source,0x12,sizeof(source));
    init_audio_libretro();
    set_audio_format_via_libretro(31995,49656530,1552);
    assert(published_rate>31995 && published_rate<31996);
    /* Completed DMA or AI_LEN polls have already supplied the whole field. */
    push_audio_samples_via_libretro(source,sizeof(source));
    nps64_audio_pad_field(960*1552,1552);
    flush_audio_libretro();assert(received==960 && nonzero==1920);
    /* A partial field receives only its deficit as silence. */
    push_audio_samples_via_libretro(source,100*4);
    nps64_audio_pad_field(960*1552,1552);
    flush_audio_libretro();assert(received==1920 && nonzero==2120);
    /* A field with no DMA remains a continuous silent field. */
    nps64_audio_pad_field(960*1552,1552);flush_audio_libretro();assert(received==2880);
    /* Excess samples are not discarded to force a nominal field size. */
    push_audio_samples_via_libretro(source,sizeof(source));
    nps64_audio_pad_field(100*1552,1552);flush_audio_libretro();assert(received==3840);
    init_audio_libretro();received=0;
    for(unsigned field=0;field<10000;field++) {
        nps64_audio_pad_field(993000,1552);flush_audio_libretro();
    }
    assert(received==(uint64_t)993000*10000/1552);
    /* The emergency accumulator drain must not make padding duplicate DMA. */
    init_audio_libretro();received=0;
    for(unsigned i=0;i<10;i++)push_audio_samples_via_libretro(source,sizeof(source));
    nps64_audio_pad_field(960*1552,1552);flush_audio_libretro();assert(received==9600);
    /* AI_LEN quantization alternates short/long fields. Never puncture a
     * continuous nonzero stream with per-field zero padding. */
    init_audio_libretro();received=nonzero=0;
    for(unsigned field=0;field<10000;field++) {
        unsigned frames=(field&1)?532:534;
        push_audio_samples_via_libretro(source,frames*4);
        nps64_audio_pad_field(533*1552,1552);
        flush_audio_libretro();
        assert(nonzero==received*2);
    }
    assert(received==533*10000);
    /* Also allow a first short field to borrow its alignment pair. */
    init_audio_libretro();received=nonzero=0;
    for(unsigned field=0;field<10000;field++) {
        unsigned frames=(field&1)?534:532;
        push_audio_samples_via_libretro(source,frames*4);
        nps64_audio_pad_field(533*1552,1552);
        flush_audio_libretro();assert(nonzero==received*2);
    }
    assert(received==533*10000);
    deinit_audio_libretro();assert(get_audio_sample_rate_libretro()==32040.0);
    puts("PASS actual audio backend: completed DMA, polls, partial/silent fields, excess, fractional DAC budget, emergency drain and rate reset");
}

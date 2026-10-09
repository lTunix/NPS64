#ifndef NPS64_PLATFORM_H
#define NPS64_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define SCREEN_W 1280
#define SCREEN_H 720
enum { BTN_UP=1, BTN_DOWN=2, BTN_LEFT=4, BTN_RIGHT=8, BTN_CONFIRM=16,
       BTN_BACK=32, BTN_SCAN=64, BTN_PAUSE=128, BTN_PREV=256, BTN_NEXT=512,
       BTN_HELP=1024, BTN_EXIT=2048 };
typedef struct { uint32_t buttons,raw_buttons; int16_t lx,ly,rx,ry; uint16_t joy; bool connected; } Input;
bool platform_init(void);
void platform_shutdown(void);
void platform_poll(Input pads[4]);
uint32_t *platform_framebuffer(void);
bool platform_present(void);
void platform_audio(const int16_t *samples, size_t frames, double sample_rate);
void platform_audio_clear(void);
/* Optional core hook, implemented by the PS4 backend. */
void nps64_cpu_worker_enter(unsigned lane);
double platform_time(void);
void platform_sleep(double seconds);
void platform_text(int x,int y,const char *text,uint32_t color,int scale);
#endif

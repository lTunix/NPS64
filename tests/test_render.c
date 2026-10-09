/* Validate the frontend scaler across format, pitch and geometry changes. */
#define main embedded_frontend_main
#include "../src/frontend.c"
#undef main
#include <assert.h>
int main(void) {
    uint32_t *last_frame=platform_framebuffer();assert(last_frame);
    uint32_t rgb[4]={0x00ff0000,0x0000ff00,0x000000ff,0x00ffffff};
    pixel_format=RETRO_PIXEL_FORMAT_XRGB8888;
    video_refresh(rgb,2,2,8);game_video_draw();
    assert(last_frame[0]==0xff000000 && last_frame[SCREEN_W-1]==0xff000000);
    assert(last_frame[160]==0xffff0000);
    assert(last_frame[SCREEN_W/2]==0xff00ff00);
    assert(last_frame[360*SCREEN_W+160]==0xff0000ff);
    assert(last_frame[SCREEN_H*SCREEN_W-161]==0xffffffff);
    /* All duplicated destination rows must retain the same source scanline. */
    for(int y=0;y<360;y++) assert(!memcmp(last_frame+160,last_frame+y*SCREEN_W+160,960*4));
    uint16_t rgb565[8]={0xf800,0x07e0,0x001f,0xaaaa,0xffff,0,0x8410,0xbbbb};
    pixel_format=RETRO_PIXEL_FORMAT_RGB565;widescreen=true;
    video_refresh(rgb565,3,2,8);game_video_draw(); /* non-square source with padding */
    assert(last_frame[0]==0xffff0000 && last_frame[SCREEN_W/2]==0xff00ff00 && last_frame[SCREEN_W-1]==0xff0000ff);
    assert(last_frame[360*SCREEN_W]==0xffffffff && last_frame[360*SCREEN_W+SCREEN_W/2]==0xff000000);
    uint16_t rgb1555[4]={0x7c00,0x03e0,0x001f,0x7fff};
    pixel_format=RETRO_PIXEL_FORMAT_0RGB1555;widescreen=false;
    video_refresh(rgb1555,2,2,4);game_video_draw();
    assert(last_frame[160]==0xffff0000 && last_frame[SCREEN_W/2]==0xff00ff00);
    assert(last_frame[360*SCREEN_W+160]==0xff0000ff);
    uint32_t before=last_frame[160];video_refresh(NULL,2,2,4);game_video_draw();assert(last_frame[160]==before);
    video_refresh(rgb1555,2,2,1);game_video_draw();assert(last_frame[160]==before);
    /* Overlay and pause must not corrupt the retained native source. */
    uint64_t callbacks=video_frames;
    game_hud_draw();assert(!memcmp(native_frame,rgb1555,sizeof(rgb1555)));
    game_video_draw();assert(last_frame[160]==before && video_frames==callbacks);
    memset(rgb1555,0,sizeof(rgb1555));game_video_draw();assert(last_frame[160]==before);
    free(native_frame);native_frame=NULL;native_capacity=0;
    puts("PASS: scaler XRGB8888/RGB565/1555, row duplication, aspect, pitch and geometry changes");
    return 0;
}

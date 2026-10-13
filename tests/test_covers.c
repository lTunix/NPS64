#include "../src/covers.c"
#include <assert.h>
#include <unistd.h>
static uint32_t frame[SCREEN_W*SCREEN_H+2];
uint32_t *platform_framebuffer(void){return frame+1;}
void nps64_log(const char *format,...){(void)format;}
int nps64_http_init(Nps64Http *http){(void)http;return -1;}
int nps64_http_open(Nps64Http *http,const char *url,int *status){(void)http;(void)url;(void)status;assert(0);return -1;}
int nps64_http_read(Nps64Http *http,void *data,size_t bytes){(void)http;(void)data;(void)bytes;assert(0);return -1;}
void nps64_http_term(Nps64Http *http){(void)http;}
int main(void) {
    int w,h;assert(!decode((const unsigned char*)"not a PNG",9,&w,&h));int bytes;unsigned char *png=read_png("build/test-cover.png",&bytes);assert(png);
    uint32_t *pixels=decode(png,bytes,&w,&h);assert(pixels&&w==32&&h==16);assert(pixels[0]==0xffe03020);free(pixels);assert(!decode(png,bytes/2,&w,&h));free(png);
    mkdir(COVER_ROOT,0777);FILE *source=fopen("build/test-cover.png","rb"),*target=fopen(COVER_ROOT "/Custom Game.png","wb");assert(source&&target);int c;while((c=fgetc(source))!=EOF)fputc(c,target);fclose(source);fclose(target);
    covers_init();Game game={0};snprintf(game.name,sizeof(game.name),"Custom Game.n64");covers_request(&game);
    bool done=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);done=slots[0].pixels!=NULL;pthread_mutex_unlock(&mutex);if(done)break;usleep(1000);}assert(done);
    frame[0]=frame[SCREEN_W*SCREEN_H+1]=0x12345678;covers_draw(&game,-10,-10,100,50);covers_draw(&game,1270,710,100,50);assert(frame[0]==0x12345678&&frame[SCREEN_W*SCREEN_H+1]==0x12345678);assert(frame[1]==0xffe03020);
    covers_refresh();covers_request(&game);covers_shutdown();unlink(COVER_ROOT "/Custom Game.png");rmdir(COVER_ROOT);puts("PASS: bounded PNG decode, truncated PNG, custom offline cover, clipped drawing, refresh and worker shutdown");
}

#include "../src/covers.c"
#include <assert.h>
#include <unistd.h>
#include <time.h>
static pthread_mutex_t http_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t http_ready=PTHREAD_COND_INITIALIZER;
static bool http_block=true,http_entered;
static unsigned http_attempts;
static FILE *http_png;
double platform_time(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int nps64_http_set_timeout(Nps64Http *http,unsigned us){(void)http;assert(us==3000000);return 0;}
static uint32_t frame[SCREEN_W*SCREEN_H+2];
static pthread_mutex_t draw_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t draw_ready=PTHREAD_COND_INITIALIZER;
static bool draw_block,draw_entered;
uint32_t *platform_framebuffer(void){pthread_mutex_lock(&draw_mutex);draw_entered=true;pthread_cond_broadcast(&draw_ready);while(draw_block)pthread_cond_wait(&draw_ready,&draw_mutex);pthread_mutex_unlock(&draw_mutex);return frame+1;}
static void *draw_cover(void *game){covers_draw(game,0,0,396,314);return NULL;}
const char *nps64_tr(const char *text){return text;}
void nps64_log(const char *format,...){(void)format;}
int nps64_http_init(Nps64Http *http){(void)http;pthread_mutex_lock(&http_mutex);http_attempts++;http_entered=true;pthread_cond_broadcast(&http_ready);while(http_block)pthread_cond_wait(&http_ready,&http_mutex);bool success=http_attempts>1;pthread_mutex_unlock(&http_mutex);return success?0:-1;}
int nps64_http_open(Nps64Http *http,const char *url,int *status){(void)http;assert(strstr(url,"GoldenEye"));*status=200;http_png=fopen("build/test-cover.png","rb");assert(http_png);return 0;}
int nps64_http_read(Nps64Http *http,void *data,size_t bytes){(void)http;return (int)fread(data,1,bytes,http_png);}
void nps64_http_term(Nps64Http *http){(void)http;if(http_png){fclose(http_png);http_png=NULL;}}
int main(void) {
    int w,h;assert(!decode((const unsigned char*)"not a PNG",9,&w,&h));int bytes;unsigned char *png=read_png("build/test-cover.png",&bytes);assert(png);
    uint32_t *pixels=decode(png,bytes,&w,&h);assert(pixels&&w==32&&h==16);assert(pixels[0]==0xffe03020);free(pixels);assert(!decode(png,bytes/2,&w,&h));free(png);
    mkdir(COVER_ROOT,0777);FILE *source=fopen("build/test-cover.png","rb"),*target=fopen(COVER_ROOT "/Custom Game.png","wb");assert(source&&target);int c;while((c=fgetc(source))!=EOF)fputc(c,target);fclose(source);fclose(target);
    covers_init();Game missing={0};snprintf(missing.name,sizeof(missing.name),"Super Mario 64 (USA).z64");covers_request(&missing);
    bool entered=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&http_mutex);entered=http_entered;pthread_mutex_unlock(&http_mutex);if(entered)break;usleep(1000);}assert(entered);
    Game game={0};snprintf(game.name,sizeof(game.name),"Custom Game.n64");covers_request(&game);
    bool done=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);done=slots[1].image!=NULL;pthread_mutex_unlock(&mutex);if(done)break;usleep(1000);}assert(done);
    frame[0]=frame[SCREEN_W*SCREEN_H+1]=0x12345678;covers_draw(&game,-10,-10,100,50);covers_draw(&game,1270,710,100,50);assert(frame[0]==0x12345678&&frame[SCREEN_W*SCREEN_H+1]==0x12345678);assert(frame[1]==0xffe03020);
    /* The local cover must finish while the preceding download remains blocked. */
    pthread_mutex_lock(&http_mutex);assert(http_block&&http_attempts==1);http_block=false;pthread_cond_broadcast(&http_ready);pthread_mutex_unlock(&http_mutex);
    bool failed=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);failed=miss_count==1;pthread_mutex_unlock(&mutex);if(failed)break;usleep(1000);}assert(failed);
    Game downloadable={0};snprintf(downloadable.name,sizeof(downloadable.name),"007 - GoldenEye (USA).n64");covers_request(&downloadable);
    bool downloaded=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);for(unsigned j=0;j<8;j++)if(!strcmp(slots[j].name,downloadable.name)&&slots[j].image)downloaded=true;pthread_mutex_unlock(&mutex);if(downloaded)break;usleep(1000);}assert(downloaded);
    /* Evict the failed slot and request it again: no repeated network attempt. */
    for(unsigned i=0;i<9;i++){Game other={0};snprintf(other.name,sizeof(other.name),"Unknown %u.n64",i);covers_request(&other);}
    covers_request(&missing);
    bool finished=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);for(unsigned j=0;j<8;j++)if(!strcmp(slots[j].name,missing.name)&&slots[j].attempted)finished=true;pthread_mutex_unlock(&mutex);if(finished)break;usleep(1000);}assert(finished);
    pthread_mutex_lock(&http_mutex);assert(http_attempts==2);pthread_mutex_unlock(&http_mutex);
    covers_refresh();covers_request(&game);
    done=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);done=slots[0].image!=NULL;pthread_mutex_unlock(&mutex);if(done)break;usleep(1000);}assert(done);
    pthread_mutex_lock(&draw_mutex);draw_block=true;draw_entered=false;pthread_mutex_unlock(&draw_mutex);
    pthread_t drawing;assert(!pthread_create(&drawing,NULL,draw_cover,&game));
    bool drawing_now=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&draw_mutex);drawing_now=draw_entered;pthread_mutex_unlock(&draw_mutex);if(drawing_now)break;usleep(1000);}assert(drawing_now);
    /* Refresh releases the slot while the draw retains its own image reference. */
    covers_refresh();covers_request(&game);
    done=false;for(unsigned i=0;i<5000;i++){pthread_mutex_lock(&mutex);done=slots[0].image!=NULL;pthread_mutex_unlock(&mutex);if(done)break;usleep(1000);}assert(done);
    pthread_mutex_lock(&draw_mutex);draw_block=false;pthread_cond_broadcast(&draw_ready);pthread_mutex_unlock(&draw_mutex);pthread_join(drawing,NULL);
    covers_shutdown();unlink(COVER_ROOT "/Custom Game.png");char key[17],path[512];cover_key(downloadable.name,key);snprintf(path,sizeof(path),COVER_ROOT "/%s.png",key);unlink(path);rmdir(COVER_ROOT);puts("PASS: blocked network does not block local covers, next download succeeds after failure, failed downloads are not retried on eviction, bounded PNG decode, truncated PNG, custom offline cover, clipped drawing, concurrent draw/refresh and worker shutdown");
}

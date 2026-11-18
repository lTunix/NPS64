#include "frame_capture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
void nps64_log(const char *format,...){(void)format;}
double platform_time(void){return 42.0;}
static int complete(void){
    for(unsigned i=0;i<10000;i++) {
        int result=nps64_frame_capture_result();if(result)return result;
        struct timespec wait={0,1000000};nanosleep(&wait,NULL);
    }
    assert(!"capture writer timed out");return 0;
}
static void check_pixel(const char *directory,const char *name,unsigned r,unsigned g,unsigned b) {
    char path[2048];snprintf(path,sizeof(path),"%s/%s",directory,name);FILE *f=fopen(path,"rb");assert(f);
    char header[12];assert(fread(header,1,11,f)==11);assert(!memcmp(header,"P6\n4 3\n255\n",11));
    unsigned char pixels[36];assert(fread(pixels,1,sizeof(pixels),f)==sizeof(pixels));assert(fgetc(f)==EOF);fclose(f);
    assert(pixels[0]==r&&pixels[1]==g&&pixels[2]==b);
}
int main(void){
    char root[]="/tmp/nps64-capture-XXXXXX";assert(mkdtemp(root));
    assert(!nps64_frame_capture_arm(root,"rom-test",1281,720,"test"));
    assert(nps64_frame_capture_arm(root,"rom-test",4,3,"test"));
    assert(!nps64_frame_capture_arm(root,"rom-test",4,3,"busy"));
    uint32_t pixels[12];for(unsigned i=0;i<12;i++)pixels[i]=0xff112233;
    for(unsigned i=0;i<16;i++) {
        pixels[0]=0xff112233;nps64_frame_capture_before(pixels,i/2,i/2,320,240,2);
        pixels[0]=0xffaabbcc;nps64_frame_capture_after(pixels);
        memset(pixels,0,sizeof(pixels));for(unsigned j=1;j<12;j++)pixels[j]=0xff112233;
    }
    assert(complete()==1);
    char directory[2048];snprintf(directory,sizeof(directory),"%s/capture-42000000-0",root);
    check_pixel(directory,"00-game.ppm",0x11,0x22,0x33);check_pixel(directory,"00-scanout.ppm",0xaa,0xbb,0xcc);
    check_pixel(directory,"15-game.ppm",0x11,0x22,0x33);
    assert(nps64_frame_capture_arm(root,"rom-test",4,3,"partial"));
    for(unsigned i=0;i<3;i++){pixels[0]=0xff112233;nps64_frame_capture_before(pixels,100+i,99,320,240,2);nps64_frame_capture_after(pixels);}
    nps64_frame_capture_finish();assert(complete()==1);
    snprintf(directory,sizeof(directory),"%s/capture-42000000-1",root);check_pixel(directory,"02-game.ppm",0x11,0x22,0x33);
    assert(nps64_frame_capture_arm(root,"rom-test",4,3,"empty"));nps64_frame_capture_before(pixels,200,199,320,240,2);nps64_frame_capture_finish();
    nps64_frame_capture_shutdown();
    assert(nps64_frame_capture_arm(root,"rom-test",4,3,"shutdown-pending"));
    pixels[0]=0xff112233;nps64_frame_capture_before(pixels,300,299,320,240,2);nps64_frame_capture_after(pixels);
    nps64_frame_capture_shutdown();assert(nps64_frame_capture_result()==1);
    snprintf(directory,sizeof(directory),"%s/capture-42000000-3",root);check_pixel(directory,"00-scanout.ppm",0x11,0x22,0x33);
    assert(!nps64_frame_capture_arm("/dev/null/blocked","rom-test",4,3,"failure"));
    puts("PASS: 16 paired snapshots, duplicate emulated frames retained, color channels, immutable copies, busy/partial/empty/failed captures and joined shutdown");
}

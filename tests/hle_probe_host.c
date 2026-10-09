#include <stdio.h>
#include <stdint.h>
int nps64_hle_graphics_probe(unsigned char*);
int main(void) {
    static unsigned char pixels[256*256*4];
    int result=nps64_hle_graphics_probe(pixels);if(result)return result;
    unsigned red=0,yellow=0,olive=0,lime=0,black=0;
    for(unsigned i=0;i<256*256;i++) {
        unsigned char *p=pixels+i*4;
        if(p[0]==255&&p[1]==0&&p[2]==0&&p[3]==255)red++;
        else if(p[0]==255&&p[1]==255&&p[2]==0&&p[3]==255)yellow++;
        else if(p[0]>=127&&p[0]<=128&&p[1]>=127&&p[1]<=128&&p[2]==0&&p[3]==255)olive++;
        else if(p[0]>=127&&p[0]<=128&&p[1]==255&&p[2]==0&&p[3]==255)lime++;
        else if(p[0]==0&&p[1]==0&&p[2]==0&&p[3]==255)black++;
        else {fprintf(stderr,"unexpected pixel %u: %u,%u,%u,%u\n",i,p[0],p[1],p[2],p[3]);return 1;}
    }
    if(red<4000||yellow<4000||olive<4000||lime<4000||red+yellow+olive+lime!=20808||black!=44728)return 1;
    FILE *f=fopen("/work/build/hle-probe/triangle.ppm","wb");if(!f)return 1;
    fprintf(f,"P6\n256 256\n255\n");for(unsigned i=0;i<65536;i++)fwrite(pixels+i*4,1,3,f);fclose(f);
    printf("PASS full framebuffer: red=%u yellow=%u olive=%u lime=%u black=%u; texture and alpha blend verified\n",red,yellow,olive,lime,black);
    return 0;
}

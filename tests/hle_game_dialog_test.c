#include "hle_game_geometry.h"
#include "hle_game_material.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void nps64_log(const char *format,...){(void)format;}
static unsigned char glyph[8*8*4],pixels[256*256*4];
int main(void){
    nps64_hle_geometry_begin();nps64_hle_geometry_capture_textures(1);
    unsigned char white[4]={255,255,255,255};
    for(unsigned id=0;id<128;id++){
        unsigned x=(id%16)*8,y=(id/16)*8;
        for(unsigned gy=0;gy<8;gy++)for(unsigned gx=0;gx<8;gx++){
            unsigned p=(gy*8+gx)*4;
            glyph[p]=id+64;glyph[p+1]=255-id;glyph[p+2]=id;
            glyph[p+3]=((gx+gy+id)%3)?255:0;
        }
        uint32_t rect[4]={0xe4000000u|((x+7)*4<<12)|(y+7)*4,(x*4<<12)|y*4,0,(4096u<<16)|1024u};
        int base=nps64_hle_rectangle_observe(rect,2,256,256,0);assert(base>=0);
        for(unsigned i=0;i<2;i++){
            nps64_hle_material_publish(base+i,31,0x001c0041,0,HLE_ALPHA_TEST,white,white,128);
            nps64_hle_texture_publish(base+i,glyph,8,8,0,0,0,0,3);
            nps64_hle_sampling_publish(base+i,7,7,0,0,0,0);
            nps64_hle_scissor_publish(base+i,0,0,256,256);
        }
    }
    size_t count;const HleVertex *vertices=nps64_hle_geometry_scene(256,256,&count);
    assert(count==128*6&&nps64_hle_geometry_complete());
    HleSceneFrame scene={vertices,count,nps64_hle_geometry_depth(),nps64_hle_geometry_materials(),nps64_hle_geometry_atlas()};
    HleRenderer *r=NULL;assert(!nps64_hle_renderer_open(&r,&scene));
    assert(!nps64_hle_renderer_frame(r,&scene,pixels));
    for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++){
        unsigned id=(y/8)*16+x/8,lit=x<128&&y<64&&((x%8+y%8+id)%3)!=0;
        const unsigned char *p=pixels+(y*256+x)*4;
        assert(p[0]==(lit?id+64:0)&&p[1]==(lit?255-id:0)&&p[2]==(lit?id:0)&&p[3]==255);
    }
    nps64_hle_renderer_release(r);
    puts("PASS 128 distinct dialog glyphs: RDP COPY rectangles, tight atlas packing, alpha holes, ordered GPU draw and all framebuffer pixels");
}

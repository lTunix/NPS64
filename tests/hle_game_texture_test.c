#include "hle_game_material.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
static unsigned char atlas[512*512*4],pixels[256*256*4];
static HleVertex v[6]={
    {{-1,-1,.5f,1},{1,1,1,1},{0,0}},{{1,-1,.5f,1},{1,1,1,1},{2,0}},{{1,1,.5f,1},{1,1,1,1},{2,2}},
    {{-1,-1,.5f,1},{1,1,1,1},{0,0}},{{1,1,.5f,1},{1,1,1,1},{2,2}},{{-1,1,.5f,1},{1,1,1,1},{0,2}}};
static unsigned address(unsigned pixel,unsigned clamp,unsigned mirror){
    float p=(pixel+.5f)*2/256;
    if(clamp)p=fminf(p,.999f);else if(mirror)p=1-fabsf(fmodf(p,2)-1);else p=p-floorf(p);
    return (unsigned)(p*2);
}
int main(void){
    unsigned char colors[4][4]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255}};
    /* Nonzero atlas origin catches accidental sampling of the checker or cell zero. */
    for(unsigned y=0;y<2;y++)for(unsigned x=0;x<2;x++)memcpy(atlas+((64+y)*512+128+x)*4,colors[y*2+x],4);
    unsigned char depth[2]={0,0};
    unsigned modes[3]={0,3,12};
    for(unsigned mode=0;mode<3;mode++){
        HleMaterial materials[2]={{.rect={128/512.f,64/512.f,2/512.f,2/512.f},.wrap=modes[mode]},{.rect={128/512.f,64/512.f,2/512.f,2/512.f},.wrap=modes[mode]}};
        assert(nps64_hle_textured_scene_probe(v,6,depth,materials,atlas,pixels)==0);
        for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++){
            unsigned sx=address(x,modes[mode]&1,modes[mode]&4),sy=address(y,modes[mode]&2,modes[mode]&8);
            assert(!memcmp(pixels+(y*256+x)*4,colors[sy*2+sx],4));
        }
    }
    /* Manual RDP addressing/filter compared against an integer CPU oracle. */
    for(unsigned mode=0;mode<4;mode++){
        unsigned wrap=32u|(mode==0?0u:mode==1?12u:3u)|(mode==3?64u:0u);
        HleMaterial materials[2]={{.rect={128/512.f,64/512.f,2/512.f,2/512.f},.wrap=wrap,.sampling={1,1,2,2}},
                                 {.rect={128/512.f,64/512.f,2/512.f,2/512.f},.wrap=wrap,.sampling={1,1,2,2}}};
        assert(nps64_hle_textured_scene_probe(v,6,depth,materials,atlas,pixels)==0);
        for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++){
            float sx=(x+.5f)*4/256,sy=(y+.5f)*4/256;
            int bx=(int)floorf(sx),by=(int)floorf(sy),fx=(int)floorf((sx-bx)*32),fy=(int)floorf((sy-by)*32);
            unsigned char expected[4];
            if(mode<3){
                int ix=bx,iy=by;
                if(mode==2){if(ix>1)ix=1;if(iy>1)iy=1;}
                if(mode==1){if(ix&2)ix=~ix;if(iy&2)iy=~iy;}
                ix&=1;iy&=1;memcpy(expected,colors[iy*2+ix],4);
            }else{
                int x0=bx>1?1:bx,y0=by>1?1:by,x1=bx+1>1?1:bx+1,y1=by+1>1?1:by+1;
                for(unsigned c=0;c<4;c++){
                    int a=colors[y0*2+x0][c],b=colors[y0*2+x1][c],d=colors[y1*2+x0][c],e=colors[y1*2+x1][c];
                    expected[c]=fx+fy<32?a+((fx*(b-a)+fy*(d-a)+16)>>5):e+(((32-fx)*(d-e)+(32-fy)*(b-e)+16)>>5);
                }
            }
            const unsigned char *p=pixels+(y*256+x)*4;
            for(unsigned c=0;c<4;c++)assert(abs((int)p[c]-expected[c])<=1);
        }
    }
    /* RDP midpoint averages four samples, unlike triangular interpolation. */
    for(unsigned i=0;i<6;i++){v[i].uv[0]=.25f;v[i].uv[1]=.25f;}
    HleMaterial mid[2]={{.rect={128/512.f,64/512.f,2/512.f,2/512.f},.wrap=32|64|128|3,.sampling={1,1,0,0}},
                        {.rect={128/512.f,64/512.f,2/512.f,2/512.f},.wrap=32|64|128|3,.sampling={1,1,0,0}}};
    HleSceneFrame scene={v,6,depth,mid,atlas};HleRenderer *renderer=NULL;
    assert(!nps64_hle_renderer_open(&renderer,&scene));
    assert(!nps64_hle_renderer_frame(renderer,&scene,pixels));
    const unsigned char *center=pixels+(128*256+128)*4;
    assert(center[0]==128&&center[1]==128&&center[2]==64&&center[3]==255);
    for(unsigned i=0;i<6;i++){v[i].uv[0]=-.1f;v[i].uv[1]=-.1f;}
    for(unsigned mirror=0;mirror<2;mirror++){
        for(unsigned i=0;i<2;i++){mid[i].wrap=32|(mirror?12:0);mid[i].sampling[2]=mid[i].sampling[3]=2;}
        assert(!nps64_hle_renderer_frame(renderer,&scene,pixels));
        assert(!memcmp(pixels+(128*256+128)*4,colors[mirror?0:3],4));
    }
    /* Screen coverage must survive large, varying clip W; texture UV still
     * follows the rational perspective equation, not affine interpolation. */
    static const float xy[6][2]={{-1,-1},{1,-1},{1,1},{-1,-1},{1,1},{-1,1}};
    for(unsigned i=0;i<6;i++){
        float w=xy[i][0]<0?1234.f:2468.f;
        v[i].position[0]=xy[i][0]*w;v[i].position[1]=xy[i][1]*w;
        v[i].position[2]=.5f*w;v[i].position[3]=w;
        v[i].uv[0]=xy[i][0]<0?0:1;v[i].uv[1]=.25f;
    }
    for(unsigned i=0;i<2;i++){mid[i].wrap=32|3;mid[i].sampling[2]=mid[i].sampling[3]=0;}
    assert(!nps64_hle_renderer_frame(renderer,&scene,pixels));
    for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++){
        float fraction=(x+.5f)/256;
        float perspective=fraction/(2-fraction);
        assert(!memcmp(pixels+(y*256+x)*4,colors[perspective>=.5f?1:0],4));
    }
    puts("PASS large varying W: full screen coverage and perspective UV against CPU equation");
    nps64_hle_renderer_release(renderer);
    puts("PASS manual RDP texels, integer mask/mirror/clamp, negative UV, three-point filtering and midpoint average; CPU pixel oracle");
    puts("PASS atlas origin, nearest texels, repeat/clamp/mirror; every framebuffer pixel checked");
}

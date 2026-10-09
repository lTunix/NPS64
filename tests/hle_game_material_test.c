#include "hle_game_geometry.h"
#include "hle_game_material.h"
#include "rdp_emit.h"
#include "rdp_emit_bridge.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
void nps64_log(const char *format,...){(void)format;}
extern void nps64_hle_observe_triangle(const EmitVertex *,const EmitVertex *,const EmitVertex *,const BridgeViewport *,int,int,const uint32_t *,unsigned,const int32_t *);
int main(void){
    /* Bridge s/t are doubled S10.5; rectangles remain ordinary S10.5. */
    EmitVertex a={.x=10*65536,.y=20*65536,.s=8*64*65536,.t=4*64*65536};
    BridgeViewport vp={.vtrans_x=160*65536,.vtrans_y=120*65536,.vscale_x=160*65536,.vscale_y=120*65536};
    int32_t cw[3]={2*65536,3*65536,4*65536};uint32_t cmd[2]={0xce000000,1};unsigned char rgba[16];memset(rgba,255,sizeof(rgba));
    nps64_hle_geometry_begin();nps64_hle_geometry_capture_textures(1);
    nps64_hle_observe_triangle(&a,&a,&a,&vp,1,0,cmd,2,cw);
    assert(nps64_hle_texture_request(cmd,2)==0);
    assert(nps64_hle_texture_request(cmd,2)==-1);
    nps64_hle_texture_publish(0,rgba,2,2,0,0,1,0,16);
    size_t count;const HleVertex *v=nps64_hle_geometry_scene(640,288,&count);
    assert(count==3 && v[0].uv[0]==2 && v[0].uv[1]==2 && v[0].position[3]==2);
    assert(v[0].color[0]==1 && v[0].position[0]==-1.875f);
    assert(!memcmp(nps64_hle_geometry_atlas(),rgba,8));
    assert(!memcmp(nps64_hle_geometry_atlas()+512*4,rgba+8,8));
    /* Identical command words in different draws must consume distinct entries. */
    nps64_hle_observe_triangle(&a,&a,&a,&vp,1,1,cmd,2,cw);
    assert(nps64_hle_texture_request(cmd,2)==1);
    nps64_hle_texture_publish(1,rgba,2,2,0,0,0,0,0);
    assert(!memcmp(nps64_hle_geometry_materials()[0].rect,nps64_hle_geometry_materials()[1].rect,16));
    assert(nps64_hle_geometry_depth()[1]==1);
    for(unsigned i=2;i<66;i++){
        cmd[1]=i;nps64_hle_observe_triangle(&a,&a,&a,&vp,1,0,cmd,2,cw);
        assert(nps64_hle_texture_request(cmd,2)==(int)i);rgba[0]=(unsigned char)i;
        nps64_hle_texture_publish(i,rgba,2,2,0,0,0,0,0);
    }
    assert(nps64_hle_geometry_materials()[64].rect[2]>0);
    assert(nps64_hle_geometry_materials()[65].rect[2]>0);
    nps64_hle_geometry_begin();nps64_hle_observe_triangle(&a,&a,&a,&vp,1,0,cmd,2,cw);
    nps64_hle_texture_publish(0,rgba,129,1,0,0,0,0,0);
    assert(nps64_hle_geometry_materials()[0].rect[2]==0);
    nps64_hle_geometry_capture_textures(0);assert(nps64_hle_texture_request(cmd,2)==-1);
    /* Worker rectangles interleave with triangles already captured by RSP. */
    nps64_hle_geometry_begin();nps64_hle_geometry_capture_textures(1);
    cmd[1]=1;nps64_hle_observe_triangle(&a,&a,&a,&vp,0,1,cmd,2,cw);
    uint32_t second[2]={cmd[0],2};
    nps64_hle_observe_triangle(&a,&a,&a,&vp,0,1,second,2,cw);
    assert(nps64_hle_texture_request(cmd,2)==0);
    unsigned char red[4]={255,0,0,255},blue[4]={0,0,255,255};
    uint32_t direct_prim=(3u<<6)|(7u<<18)|3u;
    nps64_hle_material_publish(0,31,direct_prim,0,0,red,red,0);
    uint32_t rect[4]={0xe4000000u|(120u<<12)|160u,(40u<<12)|80u,(32u<<16)|64u,(1024u<<16)|1024u};
    int base=nps64_hle_rectangle_observe(rect,0,320,240,.25f);assert(base==10000);
    for(unsigned i=0;i<2;i++)nps64_hle_material_publish(base+i,31,direct_prim,0,0,blue,blue,0);
    assert(nps64_hle_texture_request(second,2)==1);
    nps64_hle_material_publish(1,31,direct_prim,0,0,red,red,0);
    v=nps64_hle_geometry_scene(640,288,&count);
    assert(count==12);assert(nps64_hle_geometry_complete());
    const HleMaterial *ordered=nps64_hle_geometry_materials();
    assert(ordered[0].prim[0]==1&&ordered[1].prim[2]==1&&ordered[2].prim[2]==1&&ordered[3].prim[0]==1);
    assert(nps64_hle_geometry_depth()[0]==1&&!nps64_hle_geometry_depth()[1]&&nps64_hle_geometry_depth()[3]==1);
    assert(v[3].uv[0]==1&&v[3].uv[1]==2&&v[4].uv[0]==21&&v[5].uv[1]==22);
    /* Dialog scissor must not shrink the framebuffer projection. */
    nps64_hle_geometry_begin();
    nps64_hle_observe_triangle(&a,&a,&a,&vp,0,0,cmd,2,cw);
    base=nps64_hle_rectangle_observe(rect,0,320,120,0);
    nps64_hle_scissor_publish(base,10,20,30,80);
    v=nps64_hle_geometry_scene(640,288,&count);
    /* The unmatched degenerate triangle follows the rectangle in draw order. */
    assert(v[0].position[1]<-.8333f&&v[0].position[1]>-.8334f);
    assert(nps64_hle_geometry_materials()[0].scissor[3]==80/240.f);
    /* Wide texture occupies adjacent cells, preserving pitch and atlas bounds. */
    static unsigned char wide[80*20*4];memset(wide,17,sizeof(wide));
    nps64_hle_geometry_begin();base=nps64_hle_rectangle_observe(rect,0,320,240,0);
    nps64_hle_texture_publish(base,wide,80,20,0,0,0,0,0);
    assert(nps64_hle_geometry_materials()[base].rect[2]==80/512.f);
    assert(!memcmp(nps64_hle_geometry_atlas()+19*512*4,wide+19*80*4,80*4));
    nps64_hle_texture_publish(base+1,rgba,2,2,0,0,0,0,0);
    assert(nps64_hle_geometry_materials()[base+1].rect[0]==80/512.f);
    /* A dialog's many small glyphs must not reserve entire 64x64 cells. */
    static unsigned char glyph[8*16*4];
    nps64_hle_geometry_begin();base=nps64_hle_rectangle_observe(rect,0,320,240,0);
    for(unsigned i=0;i<180;i++){
        memset(glyph,(unsigned char)i,sizeof(glyph));
        nps64_hle_texture_publish(base,glyph,8,16,0,0,0,0,0);
        const HleMaterial *m=&nps64_hle_geometry_materials()[base];
        assert(m->rect[2]==8/512.f&&m->rect[3]==16/512.f);
        unsigned x=(unsigned)(m->rect[0]*512),y=(unsigned)(m->rect[1]*512);
        assert(x+8<=512&&y+16<=512);
        assert(!memcmp(nps64_hle_geometry_atlas()+(y*512+x)*4,glyph,8*4));
    }
    /* COPY has four samples per X step; its endpoint includes the final pixel. */
    nps64_hle_geometry_begin();base=nps64_hle_rectangle_observe(rect,2,320,240,0);
    v=nps64_hle_geometry_scene(640,288,&count);
    assert(count==6&&v[1].uv[0]==6.25f&&v[2].uv[1]==23);
    rect[0]=(rect[0]&0xffffffu)|0xe5000000u;
    nps64_hle_geometry_begin();base=nps64_hle_rectangle_observe(rect,0,320,240,0);
    v=nps64_hle_geometry_scene(640,288,&count);
    assert(v[1].uv[0]==1&&v[1].uv[1]==22&&v[2].uv[0]==21);
    assert(!nps64_hle_geometry_complete()); /* no material: CPU must supply this frame */
    nps64_hle_geometry_begin();assert(nps64_hle_geometry_complete());
    nps64_hle_geometry_reject();assert(!nps64_hle_geometry_complete());
    puts("PASS ordered command association, deduplication, atlas bounds, UV shift, clip W, rectangle ordering/copy/flip and wide atlas textures");
}

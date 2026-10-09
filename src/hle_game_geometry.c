/* Observe the existing HLE geometry stage without replacing its rasterizer. */
#include "hle_game_geometry.h"
#include "hle_game_material.h"
#include <stdatomic.h>
#include <string.h>
#include "rdp_emit.h"
#include "rdp_emit_bridge.h"
#include "log.h"
#include <math.h>
#include <stdlib.h>
#define MAX_VERTICES 30000
#define CAPTURE_TRIANGLES (MAX_VERTICES/3+2048)
static HleVertex captured[CAPTURE_TRIANGLES*3],scene[MAX_VERTICES];
static size_t used;
static _Atomic size_t published;
static int capture_textures,vi_defer_enabled;
void nps64_hle_vi_defer_enable(int enabled){vi_defer_enabled=enabled;}
int nps64_hle_vi_defer_allowed(void){return capture_textures&&vi_defer_enabled;}
static uint64_t command_hash[MAX_VERTICES/3];
static HleMaterial materials[CAPTURE_TRIANGLES],scene_materials[MAX_VERTICES/3];
static unsigned char scene_depth[MAX_VERTICES/3];
static unsigned rectangle_triangles,draw_serial,request_cursor,draw_order[CAPTURE_TRIANGLES];
static atomic_int scene_built;
static int frame_incomplete;
static unsigned rejected_draws;
void nps64_hle_geometry_reject(void){frame_incomplete=1;rejected_draws++;}
int nps64_hle_geometry_complete(void){return !frame_incomplete;}
static unsigned char atlas[HLE_ATLAS_SIZE*HLE_ATLAS_SIZE*4],tiles[256][128*128*4];
static unsigned tile_width[256],tile_height[256],tile_x[256],tile_y[256],tile_count,matched,unsupported;
static unsigned char occupied[64*64];
static unsigned material_ok,material_unsupported;
static unsigned texture_decodes,texture_cache_hits,rejected_texture[4];
void nps64_hle_texture_cache_event(unsigned hit,unsigned w,unsigned h,unsigned format,unsigned level){
    if(hit==0)texture_decodes++;
    else if(hit==1)texture_cache_hits++;
    else {rejected_texture[0]=w;rejected_texture[1]=h;rejected_texture[2]=format;rejected_texture[3]=level;}
}
static unsigned char material_cycle[CAPTURE_TRIANGLES];
static int log_detail=1;
static atomic_uint commands_seen;
void nps64_hle_command_seen(void){if(capture_textures)atomic_fetch_add_explicit(&commands_seen,1,memory_order_relaxed);}
unsigned nps64_hle_geometry_commands(void){return atomic_load_explicit(&commands_seen,memory_order_relaxed);}
void nps64_hle_geometry_log_detail(int enabled){log_detail=enabled;}
static unsigned char texture_seen[MAX_VERTICES/3];
static float triangle_extent[CAPTURE_TRIANGLES][2];
static uint64_t hash_command(const uint32_t *cmd,unsigned words){uint64_t h=1469598103934665603ull;for(unsigned i=0;i<words;i++){h^=cmd[i];h*=1099511628211ull;}return h;}
void nps64_hle_geometry_capture_textures(int enabled){capture_textures=enabled;}
const HleMaterial *nps64_hle_geometry_materials(void){return scene_built?scene_materials:materials;}
const unsigned char *nps64_hle_geometry_atlas(void){return atlas;}
int nps64_hle_texture_request(const uint32_t *cmd,unsigned words){
    if(!capture_textures)return -1;
    uint64_t h=hash_command(cmd,words);size_t count=atomic_load_explicit(&published,memory_order_acquire);
    /* Commands normally arrive in capture order: avoid a quadratic scan. */
    if(request_cursor<count&&!texture_seen[request_cursor]&&command_hash[request_cursor]==h){
        unsigned i=request_cursor++;texture_seen[i]=1;matched++;draw_order[i]=++draw_serial;return (int)i;
    }
    for(size_t i=0;i<count;i++)if(!texture_seen[i]&&command_hash[i]==h){texture_seen[i]=1;request_cursor=(unsigned)i+1;matched++;draw_order[i]=++draw_serial;return (int)i;}
    return -1;
}
void nps64_hle_texture_publish(unsigned tri,const unsigned char *rgba,unsigned w,unsigned h,unsigned sl,unsigned tl,unsigned ss,unsigned st,unsigned wrap){
    if(tri>=CAPTURE_TRIANGLES||!w||!h||w>128||h>128){unsupported++;return;}
    unsigned tile;
    for(tile=0;tile<tile_count;tile++)if(tile_width[tile]==w&&tile_height[tile]==h&&!memcmp(tiles[tile],rgba,w*h*4))break;
    if(tile==tile_count){
        if(tile_count==256){unsupported++;return;}
        unsigned cw=(w+7)/8,ch=(h+7)/8,origin=4096;
        for(unsigned y=0;y+ch<=64&&origin==4096;y++)for(unsigned x=0;x+cw<=64;x++){
            unsigned free_cells=1;
            for(unsigned dy=0;dy<ch;dy++)for(unsigned dx=0;dx<cw;dx++)if(occupied[(y+dy)*64+x+dx])free_cells=0;
            if(free_cells){origin=y*64+x;break;}
        }
        if(origin==4096){unsupported++;return;}
        tile_x[tile]=origin%64*8;tile_y[tile]=origin/64*8;
        for(unsigned dy=0;dy<ch;dy++)for(unsigned dx=0;dx<cw;dx++)occupied[(origin/64+dy)*64+origin%64+dx]=1;
        tile_count++;tile_width[tile]=w;tile_height[tile]=h;memcpy(tiles[tile],rgba,w*h*4);
        for(unsigned y=0;y<h;y++)memcpy(atlas+((tile_y[tile]+y)*512+tile_x[tile])*4,rgba+y*w*4,w*4);
    }
    materials[tri].rect[0]=tile_x[tile]/512.f;materials[tri].rect[1]=tile_y[tile]/512.f;
    materials[tri].rect[2]=w/512.f;materials[tri].rect[3]=h/512.f;materials[tri].wrap=wrap;
    float scale_s=ss<=10?1.f/(1u<<ss):(float)(1u<<(16-ss));
    float scale_t=st<=10?1.f/(1u<<st):(float)(1u<<(16-st));
    for(unsigned j=0;j<3;j++){captured[tri*3+j].uv[0]=(captured[tri*3+j].uv[0]*scale_s-sl/4.f)/w;captured[tri*3+j].uv[1]=(captured[tri*3+j].uv[1]*scale_t-tl/4.f)/h;}
}
void nps64_hle_sampling_publish(unsigned tri,unsigned cs,unsigned ct,unsigned ms,unsigned mt,unsigned filter,unsigned midpoint){
    if(tri>=CAPTURE_TRIANGLES)return;
    HleMaterial *m=&materials[tri];
    m->sampling[0]=(float)cs;m->sampling[1]=(float)ct;
    m->sampling[2]=ms?(float)(1u<<ms):0;m->sampling[3]=mt?(float)(1u<<mt):0;
    m->wrap|=32u|(filter?64u:0u)|(midpoint?128u:0u);
}
void nps64_hle_material_publish(unsigned tri,uint32_t a,uint32_t b,unsigned cycle,unsigned flags,const unsigned char *prim,const unsigned char *env,unsigned threshold){
    if(tri>=CAPTURE_TRIANGLES)return;
    HleMaterial *m=&materials[tri];m->mux[0]=a;m->mux[1]=b;material_cycle[tri]=(unsigned char)cycle;
    for(unsigned i=0;i<4;i++){m->prim[i]=prim[i]/255.f;m->env[i]=env[i]/255.f;}
    unsigned needs=0;
    if(!nps64_hle_material_decode(a,b,cycle,&needs)){material_unsupported++;return;}
    m->mux[2]=HLE_MATERIAL_VALID|(cycle==1?HLE_TWO_CYCLE:0)|flags|(needs?HLE_NEEDS_TEXTURE:0);m->mux[3]=threshold;
    material_ok++;
}
void nps64_hle_scissor_publish(unsigned tri,float left,float top,float right,float bottom){
    if(tri>=CAPTURE_TRIANGLES)return;
    float w=triangle_extent[tri][0],h=triangle_extent[tri][1];if(w<=0||h<=0)return;
    materials[tri].scissor[0]=left/w;materials[tri].scissor[1]=top/h;
    materials[tri].scissor[2]=right/w;materials[tri].scissor[3]=bottom/h;materials[tri].mux[2]|=HLE_SCISSOR;
}
static unsigned char depth_enabled[CAPTURE_TRIANGLES];
const unsigned char *nps64_hle_geometry_depth(void){return scene_built?scene_depth:depth_enabled;}
static unsigned dropped,textured_triangles;
static unsigned viewport_width,viewport_height;
static HleViState vi_state;
const HleViState *nps64_hle_vi_state(void){return &vi_state;}
void nps64_hle_target_publish(unsigned address,unsigned pixel_bytes){if(capture_textures){vi_state.framebuffer=address;vi_state.pixel_bytes=pixel_bytes;}}
void nps64_hle_vi_publish(unsigned origin,unsigned width,unsigned height,int left,int top,int active_width,int active_height,int x_start,int y_start,int x_step,int y_step){
    if(!capture_textures)return;
    vi_state.origin=origin;vi_state.width=width;vi_state.height=height;
    vi_state.left=left;vi_state.top=top;vi_state.active_width=active_width;vi_state.active_height=active_height;
    vi_state.x_start=x_start;vi_state.y_start=y_start;vi_state.x_step=x_step;vi_state.y_step=y_step;
}

void nps64_hle_geometry_begin(void){atomic_store(&commands_seen,0);rectangle_triangles=draw_serial=request_cursor=0;scene_built=0;frame_incomplete=0;rejected_draws=0;memset(draw_order,0,sizeof(draw_order));memset(occupied,0,sizeof(occupied));used=0;atomic_store(&published,0);dropped=0;textured_triangles=0;viewport_width=viewport_height=0;tile_count=matched=unsupported=material_ok=material_unsupported=0;texture_decodes=texture_cache_hits=0;memset(material_cycle,0,sizeof(material_cycle));memset(rejected_texture,0,sizeof(rejected_texture));memset(materials,0,sizeof(materials));memset(texture_seen,0,sizeof(texture_seen));}
static float clamp(float value,float min,float max){return value<min?min:value>max?max:value;}
int nps64_hle_rectangle_observe(const uint32_t *cmd,unsigned cycle,unsigned width,unsigned height,float z){
    if(!capture_textures||!width||!height||width>4096||height>4096)return -1;
    unsigned op=(cmd[0]>>24)&63;
    if(op!=0x24&&op!=0x25&&op!=0x36)return -1;
    if(rectangle_triangles+2>2048){unsupported++;return -1;}
    float x0=((cmd[1]>>12)&4095)/4.f,y0=(cmd[1]&4095)/4.f;
    float x1=((cmd[0]>>12)&4095)/4.f,y1=(cmd[0]&4095)/4.f;
    /* Copy/fill endpoints include their final pixel; regular cycles exclude it. */
    if(cycle>=2){x1=floorf(x1)+1;y1=floorf(y1)+1;}
    if(x1<=x0||y1<=y0)return -1;
    float s=0,t=0,ds=0,dt=0;
    if(op!=0x36){s=(int16_t)(cmd[2]>>16)/32.f;t=(int16_t)cmd[2]/32.f;ds=(int16_t)(cmd[3]>>16)/1024.f;dt=(int16_t)cmd[3]/1024.f;if(cycle==2)ds/=4;}
    unsigned base=MAX_VERTICES/3+rectangle_triangles;
    static const unsigned corners[6]={0,1,2,0,2,3};
    for(unsigned i=0;i<6;i++){
        unsigned corner=corners[i];float x=corner==1||corner==2?x1:x0,y=corner>=2?y1:y0;
        HleVertex *v=&captured[base*3+i];
        *v=(HleVertex){.position={x,y,z,1},.color={1,1,1,1},
            .uv={s+(op==0x25?y-y0:x-x0)*ds,t+(op==0x25?x-x0:y-y0)*dt}};
    }
    for(unsigned i=0;i<2;i++){
        triangle_extent[base+i][0]=(float)width;triangle_extent[base+i][1]=(float)height;
        depth_enabled[base+i]=0;draw_order[base+i]=++draw_serial;
    }
    rectangle_triangles+=2;scene_built=0;return (int)base;
}
void nps64_hle_geometry_failure_log(void){
    nps64_log("[hle-game] fallback reason: unmatched_draws=%u dropped=%u texture_rejections=%u material_rejections=%u textures=%u last_texture=%ux%u format=%u level=%u\n",
        rejected_draws,dropped,unsupported,material_unsupported,tile_count,rejected_texture[0],rejected_texture[1],rejected_texture[2],rejected_texture[3]);
    for(unsigned n=0;n<used/3+rectangle_triangles;n++){
        unsigned i=n<used/3?n:MAX_VERTICES/3+n-(unsigned)used/3;
        const HleMaterial *m=&materials[i];
        if(!(m->mux[2]&HLE_MATERIAL_VALID)||((m->mux[2]&HLE_NEEDS_TEXTURE)&&!m->rect[2])){
            nps64_log("[hle-game] first rejected draw=%u rectangle=%u mux=%08x:%08x cycle=%u flags=%x texture=%.0fx%.0f\n",
                n,i>=MAX_VERTICES/3,m->mux[0],m->mux[1],material_cycle[i],m->mux[2],m->rect[2]*512,m->rect[3]*512);break;
        }
    }
}
static int compare_order(const void *left,const void *right){
    unsigned a=*(const unsigned *)left,b=*(const unsigned *)right;
    unsigned ka=draw_order[a]?draw_order[a]:UINT32_MAX,kb=draw_order[b]?draw_order[b]:UINT32_MAX;
    return ka<kb?-1:ka>kb?1:a<b?-1:a>b;
}
void nps64_hle_observe_triangle(const EmitVertex *a,const EmitVertex *b,const EmitVertex *c,const BridgeViewport *vp,int textured,int z_buffered,const uint32_t *cmd,unsigned words,const int32_t *clip_w) {
    /* Use geometry coordinates, not VI output dimensions (Angrylion may
     * upscale to 640 pixels or add PAL borders). This is a snapshot estimate;
     * render-target tracking will replace it in the complete renderer. */
    float right=(vp->vtrans_x+fabsf((float)vp->vscale_x))/65536.0f;
    float bottom=(vp->vtrans_y+fabsf((float)vp->vscale_y))/65536.0f;
    if(right>=1&&right<=4096&&(unsigned)right>viewport_width)viewport_width=(unsigned)right;
    if(bottom>=1&&bottom<=4096&&(unsigned)bottom>viewport_height)viewport_height=(unsigned)bottom;
    scene_built=0;
    if(used+3>MAX_VERTICES){dropped++;return;}
    depth_enabled[used/3]=z_buffered!=0;
    triangle_extent[used/3][0]=right;triangle_extent[used/3][1]=bottom;
    command_hash[used/3]=hash_command(cmd,words);
    const EmitVertex *vertices[3]={a,b,c};
    for(unsigned i=0;i<3;i++) {
        const EmitVertex *v=vertices[i];HleVertex *out=&captured[used++];
        out->position[0]=v->x/65536.0f;out->position[1]=v->y/65536.0f;
        out->position[2]=z_buffered?clamp(v->z/(65536.0f*32768),0,1):0;
        out->position[3]=clip_w[i]>0?clip_w[i]/65536.f:1.f;
        out->color[0]=clamp(v->r/(65536.0f*255),0,1);
        out->color[1]=clamp(v->g/(65536.0f*255),0,1);
        out->color[2]=clamp(v->b/(65536.0f*255),0,1);out->color[3]=clamp(v->a/(65536.0f*255),0,1);
        out->uv[0]=v->s/(65536.f*64);out->uv[1]=v->t/(65536.f*64);
    }
    textured_triangles+=textured!=0;
    atomic_store_explicit(&published,used/3,memory_order_release);
}
const HleVertex *nps64_hle_geometry_scene(unsigned width,unsigned height,size_t *count) {
    *count=0;if(!width||!height)return scene;
    unsigned vi_width=width,vi_height=height;
    if(viewport_width)width=viewport_width;
    if(viewport_height)height=viewport_height;
    vi_state.target_width=width;vi_state.target_height=height;
    unsigned ids[CAPTURE_TRIANGLES],total=(unsigned)used/3+rectangle_triangles;
    for(unsigned i=0;i<used/3;i++)ids[i]=i;
    for(unsigned i=0;i<rectangle_triangles;i++)ids[used/3+i]=MAX_VERTICES/3+i;
    if(rectangle_triangles)qsort(ids,total,sizeof(ids[0]),compare_order);
    if(total>MAX_VERTICES/3){dropped+=total-MAX_VERTICES/3;total=MAX_VERTICES/3;}
    *count=(size_t)total*3;
    if(dropped||unsupported||material_unsupported)frame_incomplete=1;
    unsigned depth_count=0,replayed=0,material_ready=0,missing_textures=0;
    for(unsigned i=0;i<total;i++) {
        unsigned raw=ids[i];const HleMaterial *m=&materials[raw];
        scene_materials[i]=*m;scene_depth[i]=depth_enabled[raw];
        float ew=raw>=MAX_VERTICES/3?triangle_extent[raw][0]:(float)width;
        float eh=raw>=MAX_VERTICES/3?triangle_extent[raw][1]:(float)height;
        /* A text scissor is a clip region, not the render-target height.
         * Reuse the 3D viewport for rectangles targeting that same framebuffer. */
        if(raw>=MAX_VERTICES/3&&viewport_height&&viewport_width==(unsigned)ew){
            eh=(float)viewport_height;
            scene_materials[i].scissor[1]*=triangle_extent[raw][1]/eh;
            scene_materials[i].scissor[3]*=triangle_extent[raw][1]/eh;
        }
        for(unsigned j=0;j<3;j++){
            const HleVertex *v=&captured[raw*3+j];HleVertex *out=&scene[i*3+j];*out=*v;
            float w=m->rect[2]>0&&(m->wrap&16)?v->position[3]:1.f;
            out->position[0]=(v->position[0]*2/ew-1)*w;out->position[1]=(v->position[1]*2/eh-1)*w;
            out->position[2]=v->position[2]*w;out->position[3]=w;
        }
        depth_count+=depth_enabled[raw]!=0;
        replayed+=m->rect[2]>0;
        /* Explicit false colors: distinguish overlapping untextured surfaces.
         * Preserve the original captured colors separately for diagnostics. */
        unsigned hash=(unsigned)(i+1)*2654435761u;
        int ready=(m->mux[2]&HLE_MATERIAL_VALID)&&(!(m->mux[2]&HLE_NEEDS_TEXTURE)||m->rect[2]>0);
        material_ready+=ready;
        missing_textures+=(m->mux[2]&HLE_MATERIAL_VALID)&&!ready;
        if(!ready){scene_materials[i].mux[2]&=~HLE_MATERIAL_VALID;frame_incomplete=1;}
        for(unsigned j=0;j<3;j++)if(!ready){
            for(unsigned c=0;c<3;c++)scene[i*3+j].color[c]=m->rect[2]>0?1.f:(64+((hash>>(c*8))&127))/255.0f;
            scene[i*3+j].color[3]=1;
        }
    }
    scene_built=1;
    if(!log_detail)return scene;
    nps64_log("[hle-game] rectangles=%u ordered_draws=%u\n",rectangle_triangles/2,total);
    nps64_log("[hle-game] depth_enabled=%u depth_disabled=%zu; diagnostic triangle colors, not N64 shading\n",depth_count,used/3-depth_count);
    nps64_log("[hle-game] TMEM matched=%u unique_textures=%u replayed_triangles=%u unsupported=%u; material snapshot\n",matched,tile_count,replayed,unsupported);
    nps64_log("[hle-game] materials supported=%u ready=%u unsupported=%u missing_textures=%u; common combiner and basic alpha\n",material_ok,material_ready,material_unsupported,missing_textures);
    if(capture_textures&&used)nps64_log("[hle-game] texels decoded=%u cache_hits=%u last_rejected=%ux%u format=%u level=%u\n",texture_decodes,texture_cache_hits,rejected_texture[0],rejected_texture[1],rejected_texture[2],rejected_texture[3]);
    /* Bounded main-thread diagnostics, once per distinct material per process.
     * Retain raw unsupported muxes to diagnose dark models on the console. */
    static HleMaterial logged[64];static unsigned char logged_cycle[64];static unsigned logged_count;
    if(capture_textures)for(unsigned sample=0;sample<total&&logged_count<64;sample++){
        unsigned i=ids[sample];
        unsigned j;for(j=0;j<logged_count;j++)if(logged[j].mux[0]==materials[i].mux[0]&&logged[j].mux[1]==materials[i].mux[1]&&logged_cycle[j]==material_cycle[i])break;
        if(j<logged_count)continue;
        const HleMaterial *m=&materials[i];const float *shade=captured[i*3].color;
        logged_cycle[logged_count]=material_cycle[i];logged[logged_count++]=*m;
        nps64_log("[hle-game] material sample=%u mux=%08x:%08x cycle=%u flags=%03x shade=%.3f,%.3f,%.3f prim=%.3f,%.3f,%.3f env=%.3f,%.3f,%.3f texture=%.0fx%.0f\n",
            logged_count,m->mux[0],m->mux[1],material_cycle[i],m->mux[2],shade[0],shade[1],shade[2],m->prim[0],m->prim[1],m->prim[2],m->env[0],m->env[1],m->env[2],m->rect[2]*512,m->rect[3]*512);
    }
    nps64_log("[hle-game] triangles=%zu textured=%u dropped=%u extent=%ux%u VI=%ux%u; material snapshot\n",used/3,textured_triangles,dropped,width,height,vi_width,vi_height);
    return scene;
}

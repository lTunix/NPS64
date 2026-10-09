"""Observe successful HLE commands and ordered TMEM state in build copies."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
p=root/'mupen64plus-video-angrylion/rdp_emit_bridge.c'
s=p.read_text()
anchor='#include "rdp_emit_rsp.h"'
assert s.count(anchor)==1
s=s.replace(anchor,anchor+'''
extern void nps64_hle_observe_triangle(const EmitVertex *,const EmitVertex *,const EmitVertex *,const BridgeViewport *,int,int,const uint32_t *,unsigned,const int32_t *);
''')
anchor='    clip_to_emit(&a, v0, vp, &wa);'
assert s.count(anchor)==1
s=s.replace(anchor,'    const int32_t clip_w[3]={v0->cw,v1->cw,v2->cw};\n'+anchor)
call='nps64_hle_observe_triangle(&a,&b,&c,vp,textured,z_buffered,(const uint32_t *)cmd,(unsigned)n,clip_w);'
anchor='    if (n >= 0)\n        return n;'
assert s.count(anchor)==1
s=s.replace(anchor,'    if (n >= 0) {\n        if (n > 0) '+call+'\n        return n;\n    }')
anchor='    return n;\n}'
assert s.count(anchor)==1
s=s.replace(anchor,'    if (n > 0) '+call+'\n'+anchor)
p.write_text(s)
vendor=Path('/work/vendor/parallel-n64/mupen64plus-video-angrylion')
rdp=(vendor/'n64video/rdp.c').read_text()
anchor='    rdp_commands[cmd_id].handler(wid, args);'
assert rdp.count(anchor)==1
hook='''    /* Cache decoded texels only across uninterrupted triangle commands.
     * Every state/load/sync command invalidates it before its handler runs. */
    if(wid==0)nps64_hle_command_seen();
    static unsigned char nps64_cached_rgba[8][128*128*4],nps64_cached_valid[8];
#ifndef __ORBIS__
    /* Native differential test: same renderer, uncached reference decode. */
    if(wid==0 && getenv("NPS64_DISABLE_TEXTURE_CACHE"))memset(nps64_cached_valid,0,sizeof(nps64_cached_valid));
#endif
    if(wid==0 && (cmd_id<0x08 || cmd_id>0x0f))memset(nps64_cached_valid,0,sizeof(nps64_cached_valid));
    if(wid==0 && ((cmd_id>=0x08 && cmd_id<=0x0f)||cmd_id==0x24||cmd_id==0x25||cmd_id==0x36)) {
        int rectangle=cmd_id==0x24||cmd_id==0x25||cmd_id==0x36;
        int index=-1;
        if(rectangle){
            /* A fill of the Z image is not a white color overlay. */
            if(state[wid].fb_address!=state[wid].zb_address)
                index=nps64_hle_rectangle_observe(args,state[wid].other_modes.cycle_type,
                    state[wid].fb_width,state[wid].clip.yl/(4*al_scale),
                    state[wid].other_modes.z_source_sel?state[wid].primitive_z/(65536.f*32768.f):0);
        }else index=nps64_hle_texture_request(args,rdp_commands[cmd_id].length/4);
        if(index<0&&(!rectangle||state[wid].fb_address!=state[wid].zb_address))nps64_hle_geometry_reject();
        if(index>=0) {
            nps64_hle_target_publish(state[wid].fb_address,state[wid].fb_size==2?2:state[wid].fb_size==3?4:0);
            unsigned char prim[4]={state[wid].prim_color.r,state[wid].prim_color.g,state[wid].prim_color.b,state[wid].prim_color.a};
            unsigned char env[4]={state[wid].env_color.r,state[wid].env_color.g,state[wid].env_color.b,state[wid].env_color.a};
            unsigned flags=(state[wid].other_modes.alpha_compare_en?2:0)|(state[wid].other_modes.force_blend?4:0)|
                (state[wid].other_modes.z_compare_en?8:0)|(state[wid].other_modes.z_update_en?16:0)|(state[wid].other_modes.cvg_times_alpha?128:0);
            unsigned cycle=state[wid].other_modes.cycle_type;
            if(state[wid].other_modes.alpha_compare_en&&state[wid].other_modes.dither_alpha_en)cycle=99;
            uint32_t mux0=state[wid].combine_raw0,mux1=state[wid].combine_raw1;
            unsigned threshold=state[wid].blend_color.a;
            if(rectangle&&cycle==2){
                /* COPY samples TEXEL0 directly, with binary texture alpha. */
                mux0=31;mux1=(1u<<6)|(7u<<18)|1u;cycle=0;
                if(flags&2)threshold=128;
            }else if(rectangle&&cmd_id==0x36&&cycle==3){
                uint32_t fill=state[wid].fill_color;
                if(state[wid].fb_size==2){unsigned c=fill&65535;prim[0]=((c>>11)&31)*255/31;prim[1]=((c>>6)&31)*255/31;prim[2]=((c>>1)&31)*255/31;prim[3]=(c&1)?255:0;}
                else {prim[0]=fill>>24;prim[1]=fill>>16;prim[2]=fill>>8;prim[3]=fill;}
                mux0=31;mux1=(3u<<6)|(7u<<18)|3u;cycle=0;flags=0;
            }
            for(unsigned n=0;n<(rectangle?2u:1u);n++){
                nps64_hle_material_publish(index+n,mux0,mux1,cycle,flags,prim,env,threshold);
                nps64_hle_scissor_publish(index+n,state[wid].clip.xh/(4.f*al_scale),state[wid].clip.yh/(4.f*al_scale),state[wid].clip.xl/(4.f*al_scale),state[wid].clip.yl/(4.f*al_scale));
            }
            if(rectangle?cmd_id==0x36:!(cmd_id&2))goto nps64_material_done;
            unsigned tilenum=rectangle?(args[1]>>24)&7:(args[0]>>16)&7;
            const struct tile *tile=&state[wid].tile[tilenum];
            unsigned w=tile->mask_s && !tile->cs ? 1u<<tile->mask_s : ((tile->sh-tile->sl)>>2)+1u;
            unsigned h=tile->mask_t && !tile->ct ? 1u<<tile->mask_t : ((tile->th-tile->tl)>>2)+1u;
            unsigned wrap=(tile->cs||!tile->mask_s?1:0)|(tile->ct||!tile->mask_t?2:0)|(tile->ms?4:0)|(tile->mt?8:0)|(!rectangle&&state[wid].other_modes.persp_tex_en?16:0);
            unsigned char *rgba=nps64_cached_rgba[tilenum];
            if(w<=128 && h<=128 && w && h && tile->format!=1 && (rectangle||!((args[0]>>19)&7))) {
                nps64_hle_texture_cache_event(nps64_cached_valid[tilenum]?1:0,w,h,tile->format,0);
                if(!nps64_cached_valid[tilenum]) {
                for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
                    struct color c={0},c1={0},c2={0},c3={0};
                    if(state[wid].other_modes.en_tlut)
                        fetch_texel_entlut_quadro_nearest(wid,&c,&c1,&c2,&c3,x,y,tilenum,0,0);
                    else fetch_texel(wid,&c,x,y,tilenum);
                    unsigned offset=(y*w+x)*4;
                    rgba[offset]=CLAMP(c.r,0,255);rgba[offset+1]=CLAMP(c.g,0,255);
                    rgba[offset+2]=CLAMP(c.b,0,255);rgba[offset+3]=CLAMP(c.a,0,255);
                }
                nps64_cached_valid[tilenum]=1;
                }
                for(unsigned n=0;n<(rectangle?2u:1u);n++){
                    nps64_hle_texture_publish(index+n,rgba,w,h,tile->sl,tile->tl,tile->shift_s,tile->shift_t,wrap);
                    nps64_hle_sampling_publish(index+n,((tile->sh>>2)-(tile->sl>>2))&1023,((tile->th>>2)-(tile->tl>>2))&1023,
                        tile->mask_s,tile->mask_t,
                        state[wid].other_modes.cycle_type<2&&state[wid].other_modes.sample_type&&state[wid].other_modes.bi_lerp0,
                        state[wid].other_modes.mid_texel);
                }
            }else {
                nps64_hle_texture_cache_event(2,w,h,tile->format,(args[0]>>19)&7);
                nps64_hle_texture_publish(index,rgba,0,0,0,0,0,0,0);
            }
            nps64_material_done:;
        }
    }
'''
rdp=rdp.replace(anchor,hook+anchor)
rdp='extern void nps64_hle_target_publish(unsigned,unsigned);\nextern void nps64_hle_sampling_publish(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);\nextern void nps64_hle_geometry_reject(void);\nextern int nps64_hle_rectangle_observe(const uint32_t *,unsigned,unsigned,unsigned,float);\nextern void nps64_hle_command_seen(void);\n'+rdp
rdp='extern void nps64_hle_texture_cache_event(unsigned,unsigned,unsigned,unsigned,unsigned);\nextern void nps64_hle_scissor_publish(unsigned,float,float,float,float);\nextern int nps64_hle_texture_request(const uint32_t *,unsigned);\nextern void nps64_hle_material_publish(unsigned,uint32_t,uint32_t,unsigned,unsigned,const unsigned char *,const unsigned char *,unsigned);\nextern void nps64_hle_texture_publish(unsigned,const unsigned char *,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);\n'+rdp
(p.parent/'nps64_rdp.c').write_text(rdp)
vi=(vendor/'n64video/vi.c').read_text()
anchor='    vactivelines = v_sync - vstartoffset;'
assert vi.count(anchor)==1
vi=vi.replace(anchor,"    nps64_hle_vi_publish(frame_buffer,PRESCALE_WIDTH,(ispal?V_RES_PAL:V_RES_NTSC)>>1,h_start,v_start,hres,vres,x_start,y_start,x_add,y_add);\n"+anchor)
# Defer only the expensive progressive VI filter, retaining its per-field state.
# RDP rasterization/RDRAM writes still run unchanged. Resolve before CPU fallback.
start=vi.index('    // run filter update in parallel if enabled',vi.index('static bool vi_process_full(void)'))
end=vi.index('\n}\n',start)
tail=vi[start:end]
vi=vi[:start]+"""
    if(al_scale==1 && !ctrl.serrate && !vi_direct && !isblank &&
       !config.vi.hide_overscan && nps64_hle_vi_defer_allowed()) {
        nps64_hle_vi_pending=1;nps64_hle_vi_deferred++;
        return false;
    }
    return nps64_hle_vi_filter_finish();
"""+vi[end:]
helper='static int nps64_hle_vi_pending;\nstatic uint64_t nps64_hle_vi_deferred,nps64_hle_vi_resolved;\nstatic bool nps64_hle_vi_filter_finish(void)\n{\n'+tail+'\n}\n'
helper+="""
int nps64_hle_cpu_vi_finish(int present) {
    if(!nps64_hle_vi_pending)return 0;
    nps64_hle_vi_pending=0;
    if(!present)return 2;
    nps64_hle_vi_resolved++;
    if(nps64_hle_vi_filter_finish()) {
        extern unsigned int screen_width,screen_height;
        extern uint32_t screen_pitch;
        extern const void *screen_pixels;
        nps64_hle_cpu_video(screen_pixels?screen_pixels:prescale,screen_width,screen_height,screen_pitch);
    }
    return 1;
}
void nps64_hle_cpu_vi_stats(uint64_t *deferred,uint64_t *resolved){*deferred=nps64_hle_vi_deferred;*resolved=nps64_hle_vi_resolved;}
"""
vi=vi.replace('static bool vi_process_full(void)',helper+'\nstatic bool vi_process_full(void)',1)
# A mode/session reset invalidates any pending output pointer and field state.
# pending is declared after vi_init; cancellation belongs to vi_close, below it.
vi=vi.replace('static void vi_close(void)\n{','static void vi_close(void)\n{\n    nps64_hle_vi_pending=0;')
vi='extern int nps64_hle_vi_defer_allowed(void);\nextern void nps64_hle_cpu_video(const void *,unsigned,unsigned,size_t);\n'+vi

vi='extern void nps64_hle_vi_publish(unsigned,unsigned,unsigned,int,int,int,int,int,int,int,int);\n'+vi
(p.parent/'nps64_vi.c').write_text(vi)
n64=(vendor/'n64video.c').read_text().replace('#include "n64video/rdp.c"','#include "nps64_rdp.c"').replace('#include "n64video/vi.c"','#include "nps64_vi.c"')
(p.parent/'n64video.c').write_text(n64)

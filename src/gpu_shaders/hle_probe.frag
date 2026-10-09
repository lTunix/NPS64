#version 450
layout(push_constant) uniform Draw { vec4 tint; float depth; uint texture_mode; uint addressing; uint padding; vec4 tile; vec4 prim; vec4 env; uvec4 mux; vec4 sampling; } draw;
layout(location=0) noperspective in vec4 color;
layout(location=1) noperspective in vec3 uv_over_w;
layout(location=2) flat in uint textured;
layout(location=3) flat in vec4 rect;
layout(location=4) flat in uint wrap;
layout(set=0,binding=0) uniform sampler2D checker;
layout(location=0) out vec4 output_color;
/* Select at pipeline creation so Liverpool never dynamically decodes these
 * common material equations. Texture state comes directly from the draw. */
layout(constant_id=0) const uint material_kind=0u;
float address(float value, bool clamp_axis, bool mirror_axis) {
    if (clamp_axis) return clamp(value,0.0,1.0);
    return mirror_axis ? 1.0-abs(mod(value,2.0)-1.0) : fract(value);
}
int axis_coord(int value,int size,float clamp_max,float mask,bool clamp_axis,bool mirror_axis){
    if(clamp_axis)value=clamp(value,0,int(clamp_max));
    if(mask>0.0){
        int period=int(mask);
        if(mirror_axis&&(value&period)!=0)value=~value;
        value=value&(period-1);
    }
    return clamp(value,0,size-1);
}
vec4 atlas_texel(ivec2 pos,ivec2 size){
    ivec2 origin=ivec2(round(draw.tile.xy*512.0));
    pos=ivec2(axis_coord(pos.x,size.x,draw.sampling.x,draw.sampling.z,(draw.addressing&1u)!=0u,(draw.addressing&4u)!=0u),
              axis_coord(pos.y,size.y,draw.sampling.y,draw.sampling.w,(draw.addressing&2u)!=0u,(draw.addressing&8u)!=0u));
    return floor(texelFetch(checker,origin+pos,0)*255.0+0.5);
}
vec4 rdp_sample(vec2 coord){
    ivec2 size=ivec2(round(draw.tile.zw*512.0));
    vec2 raw=coord*vec2(size);
    ivec2 base=ivec2(floor(raw));
    vec4 c00=atlas_texel(base,size);
    if((draw.addressing&64u)==0u)return c00/255.0;
    ivec2 fraction=ivec2(floor(fract(raw)*32.0));
    vec4 c10=atlas_texel(base+ivec2(1,0),size),c01=atlas_texel(base+ivec2(0,1),size);
    vec4 result;
    if((draw.addressing&128u)!=0u&&all(equal(fraction,ivec2(16)))){
        vec4 c11=atlas_texel(base+ivec2(1,1),size);
        result=floor((c00+c10+c01+c11+vec4(2.0))/4.0);
    }else if(fraction.x+fraction.y<32){
        result=c00+floor((float(fraction.x)*(c10-c00)+float(fraction.y)*(c01-c00)+vec4(16.0))/32.0);
    }else{
        vec4 c11=atlas_texel(base+ivec2(1,1),size);
        result=c11+floor((float(32-fraction.x)*(c01-c11)+float(32-fraction.y)*(c10-c11)+vec4(16.0))/32.0);
    }
    return clamp(result,0.0,255.0)/255.0;
}
vec3 rgb(uint code,uint slot,vec4 previous,vec4 texel) {
    if(code==0u)return previous.rgb;
    if(code==1u)return texel.rgb;
    if(code==3u)return draw.prim.rgb;
    if(code==4u)return color.rgb;
    if(code==5u)return draw.env.rgb;
    if(code==6u&&(slot==0u||slot==3u))return vec3(1);
    if(slot==2u){
        if(code==7u)return vec3(previous.a);
        if(code==8u)return vec3(texel.a);
        if(code==10u)return vec3(draw.prim.a);
        if(code==11u)return vec3(color.a);
        if(code==12u)return vec3(draw.env.a);
    }
    return vec3(0);
}
float alpha(uint code,uint slot,vec4 previous,vec4 texel) {
    if(code==0u)return slot==2u?0:previous.a;
    if(code==1u)return texel.a;
    if(code==3u)return draw.prim.a;
    if(code==4u)return color.a;
    if(code==5u)return draw.env.a;
    return code==6u&&slot!=2u?1:0;
}
vec4 combine(uint cycle,vec4 previous,vec4 texel) {
    uint a=draw.mux.x,b=draw.mux.y;
    uint ra=cycle==0u?(a>>20u)&15u:(a>>5u)&15u,rb=cycle==0u?(b>>28u)&15u:(b>>24u)&15u;
    uint rc=cycle==0u?(a>>15u)&31u:a&31u,rd=cycle==0u?(b>>15u)&7u:(b>>6u)&7u;
    uint aa=cycle==0u?(a>>12u)&7u:(b>>21u)&7u,ab=cycle==0u?(b>>12u)&7u:(b>>3u)&7u;
    uint ac=cycle==0u?(a>>9u)&7u:(b>>18u)&7u,ad=cycle==0u?(b>>9u)&7u:b&7u;
    return clamp(vec4((rgb(ra,0u,previous,texel)-rgb(rb,1u,previous,texel))*rgb(rc,2u,previous,texel)+rgb(rd,3u,previous,texel),
        (alpha(aa,0u,previous,texel)-alpha(ab,1u,previous,texel))*alpha(ac,2u,previous,texel)+alpha(ad,3u,previous,texel)),0,1);
}
void main() {
    vec2 uv=uv_over_w.xy/uv_over_w.z;
    vec2 sample_uv=uv;
    if(draw.texture_mode==2u) {
        sample_uv=vec2(address(uv.x,(draw.addressing&1u)!=0u,(draw.addressing&4u)!=0u),address(uv.y,(draw.addressing&2u)!=0u,(draw.addressing&8u)!=0u));
        vec2 half_texel=vec2(0.5)/vec2(textureSize(checker,0));
        sample_uv=clamp(draw.tile.xy+sample_uv*draw.tile.zw,draw.tile.xy+half_texel,draw.tile.xy+draw.tile.zw-half_texel);
    }
    vec4 texel=draw.texture_mode==2u&&(draw.addressing&32u)!=0u?rdp_sample(uv):(draw.texture_mode!=0u?texture(checker,sample_uv):vec4(1.0));
    output_color=color*texel;
    if((draw.mux.z&256u)!=0u){
        /* DECALRGB/DECALRGBA used by the captured Mario face: PRIM-SHADE
         * times TEXEL0 plus SHADE. Avoid dynamic selector decoding here. */
        if(material_kind==1u)output_color=mix(color,draw.prim,texel);
        else if(material_kind==2u)output_color=vec4(texel.rgb*color.rgb,color.a);
        else if(material_kind==3u)output_color=color;
        else if(material_kind==4u)output_color=texel*color;
        else if(material_kind==5u)output_color=vec4(color.rgb,draw.env.a);
        else if(material_kind==6u)output_color=vec4(mix(color.rgb,texel.rgb,texel.a),draw.env.a);
        else if(material_kind==7u)output_color=vec4(texel.rgb,draw.env.a);
        else if(material_kind==8u||material_kind==11u)output_color=texel;
        else if(material_kind==9u)output_color=vec4(texel.rgb,color.a);
        else if(material_kind==10u)output_color=vec4(texel.rgb*color.rgb,texel.a);
        else if(material_kind==12u)output_color=draw.prim;
        else {
        vec4 previous=vec4(0);
        if((draw.mux.z&1u)!=0u)previous=combine(0u,previous,texel);
        output_color=combine(1u,previous,texel);
        }
        if((draw.mux.z&128u)!=0u&&output_color.a<1.0/255.0)discard;
        if((draw.mux.z&2u)!=0u&&output_color.a<float(draw.mux.w)/255.0)discard;
    }
}

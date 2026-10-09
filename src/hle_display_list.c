/* F3DEX2 command layouts checked against the bundled gles2n64 decoder.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "hle_display_list.h"
#include <string.h>

static uint8_t byte_at(const HleDlInput *in, uint32_t a) {
    return in->rdram[a ^ (in->word_swapped ? 3u : 0u)];
}
static uint16_t half(const HleDlInput *in,uint32_t a) {
    return (uint16_t)((uint16_t)byte_at(in,a)<<8|byte_at(in,a+1));
}
static uint32_t word(const HleDlInput *in,uint32_t a) {
    return (uint32_t)half(in,a)<<16|half(in,a+2);
}
static int span(const HleDlInput *in,uint32_t a,size_t n) {
    return a<=in->size && n<=in->size-a;
}
static int address(const HleDlInput *in,const uint32_t segments[16],uint32_t encoded,uint32_t *a,size_t n) {
    unsigned segment=encoded>>24;
    if(segment>=16)return 0;
    uint64_t sum=(uint64_t)segments[segment]+(encoded&0xffffffu);
    if(sum>UINT32_MAX)return 0;
    *a=(uint32_t)sum;return span(in,*a,n);
}
static void identity(float m[16]) {
    memset(m,0,16*sizeof(float));for(unsigned i=0;i<4;i++)m[i*4+i]=1;
}
/* N64 matrices are row vectors, translation is the last row. */
static void multiply(float dst[16],const float a[16],const float b[16]) {
    float tmp[16];
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++) {
        float value=0;for(unsigned k=0;k<4;k++)value+=a[r*4+k]*b[k*4+c];
        tmp[r*4+c]=value;
    }
    memcpy(dst,tmp,sizeof(tmp));
}
static HleDlStatus triangle(HleDlOutput *out,const HleVertex cache[32],const uint8_t valid[32],uint32_t w) {
    unsigned indices[3]={(w>>17)&127,(w>>9)&127,(w>>1)&127};
    for(unsigned i=0;i<3;i++)if(indices[i]>=32||!valid[indices[i]])return HLE_DL_VERTEX;
    if(out->capacity-out->count<3)return HLE_DL_CAPACITY;
    for(unsigned i=0;i<3;i++)out->vertices[out->count++]=cache[indices[i]];
    out->triangles++;return HLE_DL_OK;
}
HleDlStatus hle_decode_f3dex2(const HleDlInput *in,uint32_t start,HleDlOutput *out) {
    out->count=0;out->commands=0;out->triangles=0;out->fault_pc=start;out->fault_opcode=0;
    if(!in->rdram||!out->vertices||!in->texture_width||!in->texture_height||(in->size&3))return HLE_DL_BOUNDS;
    uint32_t segments[16]={0},returns[18],pc;unsigned stack=0,model_level=0;
    float models[18][16],projection[16];identity(models[0]);identity(projection);
    HleVertex cache[32];uint8_t valid[32]={0};
    if(!address(in,segments,start,&pc,8))return HLE_DL_BOUNDS;
    unsigned limit=in->max_commands?in->max_commands:4096;
    while(out->commands<limit) {
        out->fault_pc=pc;if((pc&7)||!span(in,pc,8))return HLE_DL_BOUNDS;
        uint32_t w0=word(in,pc),w1=word(in,pc+4);unsigned op=w0>>24;
        out->fault_opcode=op;out->commands++;pc+=8;
        switch(op) {
        case 0x00:break;
        case 0xdb: {
            if(((w0>>16)&255)!=6)return HLE_DL_OPCODE;
            unsigned offset=w0&65535;if((offset&3)||offset>=64)return HLE_DL_BOUNDS;
            segments[offset/4]=w1&0xffffffu;break;
        }
        case 0xda: {
            uint32_t a;if(!address(in,segments,w1,&a,64)||(a&7))return HLE_DL_BOUNDS;
            float matrix[16];for(unsigned i=0;i<16;i++)
                matrix[i]=(float)(int16_t)half(in,a+i*2)+(float)half(in,a+32+i*2)/65536.0f;
            unsigned flags=(w0&255)^1;
            if(flags&~7u)return HLE_DL_MATRIX;
            float *target=projection;
            if(!(flags&4)) {
                if(flags&1){if(model_level==17)return HLE_DL_MATRIX;memcpy(models[model_level+1],models[model_level],sizeof(models[0]));model_level++;}
                target=models[model_level];
            }
            if(flags&2)memcpy(target,matrix,sizeof(matrix));else multiply(target,matrix,target);
            break;
        }
        case 0xd8: {
            unsigned n=w1>>6;if((w1&63)||!n||n>model_level)return HLE_DL_MATRIX;
            model_level-=n;break;
        }
        case 0x01: {
            unsigned n=(w0>>12)&255,end=(w0>>1)&127;
            if(!n||end<n||end>32)return HLE_DL_VERTEX;
            uint32_t a;if(!address(in,segments,w1,&a,n*16)||(a&7))return HLE_DL_BOUNDS;
            float combined[16];multiply(combined,models[model_level],projection);
            for(unsigned i=0;i<n;i++) {
                HleVertex *v=&cache[end-n+i];uint32_t p=a+i*16;
                float source[4]={(float)(int16_t)half(in,p),(float)(int16_t)half(in,p+2),(float)(int16_t)half(in,p+4),1};
                for(unsigned c=0;c<4;c++){v->position[c]=0;for(unsigned r=0;r<4;r++)v->position[c]+=source[r]*combined[r*4+c];}
                /* Convert N64 clip depth [-w,w] to Vulkan [0,w]. */
                v->position[2]=(v->position[2]+v->position[3])*0.5f;
                for(unsigned c=0;c<4;c++)v->color[c]=byte_at(in,p+12+c)/255.0f;
                v->uv[0]=(int16_t)half(in,p+8)/(32.0f*in->texture_width);
                v->uv[1]=(int16_t)half(in,p+10)/(32.0f*in->texture_height);
                valid[end-n+i]=1;
            }
            break;
        }
        case 0x05:case 0x06: {
            HleDlStatus status=triangle(out,cache,valid,w0);if(status)return status;
            if(op==6){status=triangle(out,cache,valid,w1);if(status)return status;}
            break;
        }
        case 0xde: {
            unsigned branch=(w0>>16)&255;if(branch>1)return HLE_DL_OPCODE;
            uint32_t a;if(!address(in,segments,w1,&a,8))return HLE_DL_BOUNDS;
            if(!branch){if(stack==18)return HLE_DL_LIMIT;returns[stack++]=pc;}
            pc=a;break;
        }
        case 0xdf:
            if(!stack)return HLE_DL_OK;
            pc=returns[--stack];break;
        default:return HLE_DL_OPCODE;
        }
    }
    return HLE_DL_LIMIT;
}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t *p,uint32_t v){put16(p,(uint16_t)(v>>16));put16(p+2,(uint16_t)v);}
size_t hle_display_list_fixture(uint8_t *ram,size_t size) {
    if(size<512)return 0;memset(ram,0,512);
    /* Set segment 1, call sub-list; load matrix, vertices, triangle, return. */
    put32(ram,0xdb060004);put32(ram+4,0x100);
    put32(ram+8,0xde000000);put32(ram+12,0x01000000);
    put32(ram+16,0xdf000000);
    put32(ram+0x100,0xda380003);put32(ram+0x104,0x180);
    put32(ram+0x108,0x01003006);put32(ram+0x10c,0x1c0);
    put32(ram+0x110,0x05000204);put32(ram+0x118,0xdf000000);
    for(unsigned i=0;i<4;i++) {
        int32_t fixed=i==3?65536:13107; /* scale .2, homogeneous w=1 */
        put16(ram+0x180+(i*4+i)*2,(uint16_t)(fixed>>16));
        put16(ram+0x1a0+(i*4+i)*2,(uint16_t)fixed);
    }
    const int16_t positions[3][2]={{-4,-4},{4,-4},{0,4}};
    const int16_t uv[3][2]={{819,819},{7373,819},{4096,7373}};
    for(unsigned i=0;i<3;i++) {
        uint8_t *p=ram+0x1c0+i*16;put16(p,(uint16_t)positions[i][0]);put16(p+2,(uint16_t)positions[i][1]);
        put16(p+8,(uint16_t)uv[i][0]);put16(p+10,(uint16_t)uv[i][1]);memset(p+12,255,4);
    }
    return 512;
}

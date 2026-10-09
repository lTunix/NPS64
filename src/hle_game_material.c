#include "hle_game_material.h"
/* Admit common RGB/alpha equations. Unsupported inputs remain explicit. */
static int rgb(unsigned code,unsigned slot,unsigned *texture){
    if(code==2)return 0; /* TEXEL1 requires a second texture. */
    if(code==1){*texture=1;return 1;}
    if(code<=5)return 1;
    if(slot==0)return code==6||code>=8; /* ONE or ZERO; NOISE omitted. */
    if(slot==1)return code>=8; /* key center / K4 omitted. */
    if(slot==3)return code==6||code==7;
    if(code==8){*texture=1;return 1;}
    return code==7||(code>=10&&code<=12)||code>=16;
}
static int alpha(unsigned code,unsigned slot,unsigned *texture){
    if(code==2)return 0;
    if(code==1)*texture=1;
    return slot==2?(code>=1&&code<=5)||code==7:1;
}
int nps64_hle_material_decode(uint32_t a,uint32_t b,unsigned cycle,unsigned *texture){
    *texture=0;if(cycle>1)return 0;
    for(unsigned c=cycle==1?0:1;c<2;c++){
        unsigned ra=c?(a>>5)&15:(a>>20)&15,rb=c?(b>>24)&15:(b>>28)&15;
        unsigned rc=c?a&31:(a>>15)&31,rd=c?(b>>6)&7:(b>>15)&7;
        unsigned aa=c?(b>>21)&7:(a>>12)&7,ab=c?(b>>3)&7:(b>>12)&7;
        unsigned ac=c?(b>>18)&7:(a>>9)&7,ad=c?b&7:(b>>9)&7;
        if(!rgb(rd,3,texture)||!alpha(ad,3,texture))return 0;
        if(rc<16 && (!rgb(ra,0,texture)||!rgb(rb,1,texture)||!rgb(rc,2,texture)))return 0;
        if(ac!=7 && (!alpha(aa,0,texture)||!alpha(ab,1,texture)||!alpha(ac,2,texture)))return 0;
    }
    return 1;
}

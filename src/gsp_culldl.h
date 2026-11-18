/* GBI 1 culling command formats. SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <stdint.h>
#include <stddef.h>
static inline int nps64_f3dex095(const unsigned char *ram,size_t size,unsigned data){
    static const char prefix[]="RSP Gfx ucode F3D";
    if(!ram||data>=size)return 0;
    size_t end=size-data>0x1000?data+0x1000:size;
    end&=~(size_t)3;
    for(size_t b=data;b+sizeof(prefix)+5<end;b++){
        size_t k;for(k=0;k<sizeof(prefix)-1;k++)if(ram[(b+k)^3]!=(unsigned char)prefix[k])break;
        if(k!=sizeof(prefix)-1)continue;
        if((ram[(b+k)^3]!='E'&&ram[(b+k)^3]!='L')||ram[(b+k+1)^3]!='X'||ram[(b+k+2)^3]=='2')continue;
        size_t limit=b+80<end?b+80:end;
        for(size_t p=b+k+2;p+5<=limit&&ram[p^3];p++)
            if(ram[p^3]==' '&&ram[(p+1)^3]=='0'&&ram[(p+2)^3]=='.'&&ram[(p+3)^3]=='9'&&ram[(p+4)^3]=='5'
               &&(p+5==limit||ram[(p+5)^3]==' '||!ram[(p+5)^3]))return 1;
    }
    return 0;
}
static inline int nps64_culldl_range(uint32_t w0,uint32_t w1,int legacy,int capacity,int *first,int *last){
    if(legacy){*first=(int)((w0&0xffffffu)/40u);*last=(int)((w1&0xffffffu)/40u)-1;}
    else{*first=(int)((w0&0xffffu)/2u);*last=(int)((w1&0xffffu)/2u);}
    return *first>=0&&*last>=*first&&*last<capacity;
}

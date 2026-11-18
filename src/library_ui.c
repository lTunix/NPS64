/* Native 720p icon.jpeg-inspired UI, 3D cartridge carousel with reflections, and modals. GPL-2.0-or-later */
#include "library_ui.h"
#include "cartridge3d.h"
#include "nps64_logo.h"
#include "i18n.h"
#include "platform.h"
#include "covers.h"
#include "cover_match.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef NPS64_ASSET_ROOT
#define NPS64_ASSET_ROOT "/app0"
#endif
#ifndef NPS64_UI_VERSION
#define NPS64_UI_VERSION NPS64_VERSION
#endif
static unsigned char *font;
static float carousel_pos = -1.0f;
static float idle_phase = 0.0f;

static void title(const char *name,char out[256]) {
    cover_title(name,out,256);
    const char *prefix="The Legend of Zelda - ";size_t n=strlen(prefix);
    if(!strncmp(out,prefix,n)){char short_title[256];snprintf(short_title,sizeof(short_title),"Zelda: %s",out+n);snprintf(out,256,"%s",short_title);}
}
#define GLYPH (48*56+1)
static void box(int x,int y,int w,int h,uint32_t color){
    uint32_t *p=platform_framebuffer();
    int x0=x<0?0:x, y0=y<0?0:y, x1=x+w>SCREEN_W?SCREEN_W:x+w, y1=y+h>SCREEN_H?SCREEN_H:y+h;
    for(int j=y0;j<y1;j++)for(int i=x0;i<x1;i++)p[j*SCREEN_W+i]=color;
}
static void box_alpha(int x,int y,int w,int h,uint32_t color,unsigned alpha){
    uint32_t *p=platform_framebuffer();
    int x0=x<0?0:x, y0=y<0?0:y, x1=x+w>SCREEN_W?SCREEN_W:x+w, y1=y+h>SCREEN_H?SCREEN_H:y+h;
    for(int j=y0;j<y1;j++)for(int i=x0;i<x1;i++)p[j*SCREEN_W+i]=nps64_c3d_blend(p[j*SCREEN_W+i],color,alpha);
}
static void rounded_box(int x,int y,int w,int h,int r,uint32_t fill,uint32_t border,int bw){
    uint32_t *p=platform_framebuffer();
    int x0=x<0?0:x, y0=y<0?0:y, x1=x+w>SCREEN_W?SCREEN_W:x+w, y1=y+h>SCREEN_H?SCREEN_H:y+h;
    float hw=w*0.5f, hh=h*0.5f, cx=x+hw, cy=y+hh, rad=(float)r;
    for(int py=y0;py<y1;py++){
        float dy=fabsf((py+0.5f)-cy)-(hh-rad);
        uint32_t *row=p+py*SCREEN_W;
        for(int px=x0;px<x1;px++){
            float dx=fabsf((px+0.5f)-cx)-(hw-rad);
            float d=hypotf(fmaxf(dx,0.0f),fmaxf(dy,0.0f))+fminf(fmaxf(dx,dy),0.0f)-rad;
            if(d>0.5f)continue;
            uint32_t col=(bw>0&&d>-((float)bw))?border:fill;
            if(d>-0.5f){
                unsigned a=(unsigned)((0.5f-d)*255.0f);
                row[px]=nps64_c3d_blend(row[px],col,a);
            }else row[px]=col;
        }
    }
}
static int text_width(const char *str,unsigned size){
    int w=0;
    for(const unsigned char *s=(const unsigned char*)str;*s;s++){
        unsigned c=*s>=32&&*s<128?*s:'?';
        w+=font?font[4+(size*96+c-32)*GLYPH]:(size>1?16:8);
    }
    return w;
}
static void text(int x,int y,const char *str,unsigned size,uint32_t color,int limit) {
    if(!font){platform_text(x,y,str,color,size>1?2:1);return;}
    uint32_t *p=platform_framebuffer();int start=x;
    for(const unsigned char *s=(const unsigned char*)str;*s;s++){
        unsigned c=*s>=32&&*s<128?*s:'?';
        const unsigned char *g=font+4+(size*96+c-32)*GLYPH;
        int advance=g[0];if(x+advance>start+limit)break;
        for(int j=0;j<56;j++)for(int i=0;i<48;i++){
            unsigned a=g[1+j*48+i];if(!a||x+i<0||x+i>=SCREEN_W||y+j<0||y+j>=SCREEN_H)continue;
            uint32_t old=p[(y+j)*SCREEN_W+x+i],result=0xff000000;
            for(unsigned sh=0;sh<24;sh+=8)result|=((((color>>sh&255)*a+(old>>sh&255)*(255-a))/255)<<sh);
            p[(y+j)*SCREEN_W+x+i]=result;
        }
        x+=advance;
    }
}
static void text_centered(int cx,int y,const char *str,unsigned size,uint32_t color,int max_w){
    int w=text_width(str,size);
    if(w>max_w)w=max_w;
    text(cx-w/2,y,str,size,color,max_w);
}

/* Original N64 emblem from icon.jpeg, alpha-blended cleanly without square edges. */
static void draw_n64_emblem(int cx,int cy,int dst_size){
    uint32_t *fb=platform_framebuffer();
    int x0=cx-dst_size/2, y0=cy-dst_size/2;
    for(int j=0;j<dst_size;j++){
        int py=y0+j; if(py<0||py>=SCREEN_H)continue;
        int sy=(j*NPS64_LOGO_H)/dst_size;
        uint32_t *row=fb+py*SCREEN_W;
        for(int i=0;i<dst_size;i++){
            int px=x0+i; if(px<0||px>=SCREEN_W)continue;
            int sx=(i*NPS64_LOGO_W)/dst_size;
            uint32_t c=nps64_logo_argb[sy*NPS64_LOGO_W+sx];
            unsigned a=(c>>24)&255u;
            if(a>2u)row[px]=nps64_c3d_blend(row[px],c,a);
        }
    }
}

/* Small vector glyphs, sampled at 4x4 for smooth contours at 720p. */
typedef enum {KEY_CROSS,KEY_CIRCLE,KEY_SQUARE,KEY_TRIANGLE,KEY_LEFT,KEY_RIGHT,KEY_L1,KEY_R1,KEY_L2,KEY_R2,KEY_L3,KEY_R3,KEY_OPTIONS} ButtonIcon;
static float segment_distance(float x,float y,float ax,float ay,float bx,float by) {
    float dx=bx-ax,dy=by-ay,t=((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy);
    if(t<0)t=0;if(t>1)t=1;
    return hypotf(x-ax-t*dx,y-ay-t*dy);
}
static int button_icon(int x,int y,ButtonIcon key,uint32_t color) {
    bool shoulder=key>=KEY_L1;int width=key==KEY_OPTIONS?96:shoulder?40:26,height=26;
    if(key==KEY_CROSS)color=0xff80c8ff;
    if(key==KEY_CIRCLE)color=0xffff8d8d;
    if(key==KEY_SQUARE)color=0xffe6a0d4;
    if(key==KEY_TRIANGLE)color=0xff6ee7b7;
    static unsigned char masks[KEY_OPTIONS+1][104*26];static bool ready[KEY_OPTIONS+1];
    uint32_t *fb=platform_framebuffer();
    for(int py=0;py<height;py++)for(int px=0;px<width;px++) {
        unsigned coverage=masks[key][py*104+px];
        if(!ready[key]){coverage=0;
        for(unsigned sy=0;sy<4;sy++)for(unsigned sx=0;sx<4;sx++) {
            float u=px+(sx+.5f)/4,v=py+(sy+.5f)/4,d;
            if(shoulder) {
                float qx=fabsf(u-width*.5f)-(width*.5f-6),qy=fabsf(v-13)-7;
                d=fabsf(hypotf(fmaxf(qx,0),fmaxf(qy,0))+fminf(fmaxf(qx,qy),0)-4);
            } else if(key==KEY_CIRCLE)d=fabsf(hypotf(u-13,v-13)-9);
            else if(key==KEY_SQUARE)d=fabsf(fmaxf(fabsf(u-13),fabsf(v-13))-8);
            else if(key==KEY_TRIANGLE)d=fminf(segment_distance(u,v,13,5,5,20),fminf(segment_distance(u,v,5,20,21,20),segment_distance(u,v,21,20,13,5)));
            else if(key==KEY_CROSS)d=fminf(segment_distance(u,v,6,6,20,20),segment_distance(u,v,6,20,20,6));
            else {
                float tip=key==KEY_LEFT?5:21,tail=26-tip;
                d=fminf(segment_distance(u,v,tip,13,tail,13),fminf(segment_distance(u,v,tip,13,13,5),segment_distance(u,v,tip,13,13,21)));
            }
            if(d<1.1f)coverage++;
        }
        masks[key][py*104+px]=(unsigned char)coverage;
        }
        if(!coverage||x+px<0||x+px>=SCREEN_W||y+py<0||y+py>=SCREEN_H)continue;
        uint32_t *pixel=&fb[(y+py)*SCREEN_W+x+px],old=*pixel,result=0xff000000;
        for(unsigned shift=0;shift<24;shift+=8)result|=(((color>>shift&255)*coverage+(old>>shift&255)*(16-coverage))/16)<<shift;
        *pixel=result;
    }
    ready[key]=true;
    if(shoulder) {
        static const char *names[]={"L1","R1","L2","R2","L3","R3","OPTIONS"};
        text(x+10,y+1,names[key-KEY_L1],0,color,key==KEY_OPTIONS?80:30);
    }
    return width;
}
static int hint(int x,int y,ButtonIcon key,const char *label,uint32_t color) {
    x+=button_icon(x,y,key,color)+8;text(x,y+1,label,0,color,1200-x);
    return x+text_width(label,0)+24;
}

static void draw_obsidian_stage(uint32_t accent,int floor_y){
    uint32_t *fb=platform_framebuffer();
    for(int y=0;y<SCREEN_H;y++){
        uint32_t row_col;
        if(y<floor_y){
            float t=(float)y/(float)floor_y;
            unsigned r=(unsigned)(18+t*6), g=(unsigned)(21+t*6), b=(unsigned)(25+t*8);
            row_col=0xff000000u|(r<<16)|(g<<8)|b;
        }else{
            float t=(float)(y-floor_y)/(float)(SCREEN_H-floor_y);
            unsigned r=(unsigned)(14-t*4), g=(unsigned)(17-t*5), b=(unsigned)(21-t*6);
            row_col=0xff000000u|(r<<16)|(g<<8)|b;
        }
        uint32_t *row=fb+y*SCREEN_W;
        for(int x=0;x<SCREEN_W;x++)row[x]=row_col;
    }
    /* Subtle radial ambient glow behind hero cartridge (inspired by icon.jpeg) */
    int gcx=SCREEN_W/2, gcy=265;
    for(int y=110;y<floor_y;y++){
        int dy=y-gcy;
        uint32_t *row=fb+y*SCREEN_W;
        for(int x=360;x<920;x++){
            int dx=x-gcx;
            int d2=dx*dx+dy*dy*2;
            if(d2<280*280){
                unsigned a=(unsigned)((1.0f-(float)d2/(280.0f*280.0f))*34.0f);
                row[x]=nps64_c3d_blend(row[x],accent,a);
            }
        }
    }
    box(0,0,SCREEN_W,3,accent);
    box_alpha(0,floor_y,SCREEN_W,1,0xff475569,110);
}

void library_ui_init(void){
    FILE *f=fopen(NPS64_ASSET_ROOT "/ui-font.bin","rb");
    if(f){size_t bytes=4+4*96*GLYPH;font=malloc(bytes);if(font&&(fread(font,1,bytes,f)!=bytes||memcmp(font,"UIF1",4))){free(font);font=NULL;}fclose(f);}
    covers_init();
}

static void library_sidebar(bool settings,bool focus,unsigned selected,uint32_t accent) {
    draw_n64_emblem(44,54,44);
    text(74,28,"NPS64",2,0xfff3f6fb,115);text(75,64,"by lTunix",0,0xff94a5bf,115);
    const char *labels[]={nps64_tr("Biblioteca"),nps64_tr("Configuracion")};
    for(unsigned i=0;i<2;i++) {
        int y=159+(int)i*58;bool active=focus?selected==i:settings==(i==1);
        if(active){rounded_box(16,y,162,46,8,focus?0xff243447:0xff1b2634,accent,1);box(16,y+6,3,34,accent);}
        text(32,y+8,labels[i],0,active?0xffffffff:0xff94a5bf,145);
    }
    text(31,297,"NINTENDO 64",0,0xff94a5bf,150);
    text(31,642,"Version " NPS64_UI_VERSION,0,0xff94a5bf,155);
}

void library_ui_settings(const char *heading,const char *subtitle,const char *const *labels,unsigned count,unsigned selected,bool focus,unsigned side,uint32_t accent,const char *message) {
    box(0,0,SCREEN_W,SCREEN_H,0xff121519);box(0,0,SCREEN_W,3,accent);box(0,0,190,SCREEN_H,0xff161b22);
    library_sidebar(true,focus,side,accent);
    text(232,33,heading,2,0xfff3f6fb,1000);text(234,88,subtitle,0,0xff94a5bf,985);
    for(unsigned i=0;i<count;i++) {
        int y=155+(int)i*(count>8?42:48);bool active=!focus && i==selected;
        rounded_box(232,y,1004,count>8?38:42,8,active?0xff1f2b3a:0xff161c24,active?accent:0xff232b36,1);
        if(active)box(232,y+5,3,count>8?28:32,accent);
        text(250,y+5,labels[i],1,active?0xffffffff:0xffd4deeb,960);
    }
    int hx=hint(234,574,KEY_CROSS,nps64_tr("Cambiar / abrir"),0xff94a5bf);
    hx+=button_icon(hx,574,KEY_LEFT,0xff94a5bf)+4;
    hx=hint(hx,574,KEY_RIGHT,nps64_tr("Ajustar"),0xff94a5bf);
    hint(hx,574,KEY_CIRCLE,nps64_tr("Volver"),0xff94a5bf);
    box(190,646,1090,74,0xff161b22);text(232,656,message,0,accent,990);
    text(232,686,nps64_tr("La biblioteca siempre ocupa 16:9; el formato del juego se guarda por separado."),0,0xff94a5bf,990);
}

void library_ui_draw(const Game *games,size_t count,size_t selected,uint32_t accent,const char *message,const char *updates,bool sidebar_focus,unsigned sidebar_selected) {
    (void)sidebar_focus;(void)sidebar_selected;
    const int floor_y=424;
    draw_obsidian_stage(accent,floor_y);

    /* Minimalist icon.jpeg-inspired header */
    draw_n64_emblem(66,48,54);
    text(104,22,"NPS64",2,0xfff3f6fb,160);
    text(106,56,"by lTunix",0,0xff8b99ad,140);

    char pill[96];
    snprintf(pill,sizeof(pill),"v%s  |  %zu %s",NPS64_UI_VERSION,count,nps64_tr("juegos"));
    int pill_w=text_width(pill,0)+36; if(pill_w<220)pill_w=220;
    int pill_x=1240-pill_w;
    rounded_box(pill_x,26,pill_w,36,10,0xff181e27,0xff2b3646,1);
    text_centered(pill_x+pill_w/2,31,pill,0,0xffcbd5e1,pill_w-20);

    if(updates&&*updates){
        int uw=text_width(updates,0)+36; if(uw<240)uw=240;
        int ux=pill_x-uw-14;
        rounded_box(ux,26,uw,36,10,0xff1c2636,accent,1);
        text_centered(ux+uw/2,31,updates,0,accent,uw-20);
    }

    if(count){
        if(carousel_pos<0.0f||fabsf(carousel_pos-(float)selected)>8.0f)carousel_pos=(float)selected;
        else carousel_pos+=((float)selected-carousel_pos)*0.24f;
        idle_phase+=0.045f;

        /* Request covers for visible neighborhood */
        int center_idx=(int)selected;
        for(int off=-2;off<=2;off++){
            int idx=center_idx+off;
            if(idx>=0&&(size_t)idx<count)covers_request(&games[idx]);
        }

        /* Draw outer-to-inner so center hero cartridge sits cleanly on top */
        const int order[5]={-2,2,-1,1,0};
        for(int pass=0;pass<2;pass++){ /* pass 0: reflections, pass 1: solid cartridges */
            bool mirror=(pass==0);
            for(int k=0;k<5;k++){
                int idx=center_idx+order[k];
                if(idx<0||(size_t)idx>=count)continue;
                float rel=(float)idx-carousel_pos;
                float abs_rel=fabsf(rel);
                if(abs_rel>2.6f)continue;
                float cx=640.0f+rel*248.0f;
                float scale=176.0f-abs_rel*34.0f;
                if(scale<88.0f)scale=88.0f;
                float cy=330.0f-abs_rel*12.0f+(abs_rel<0.3f?sinf(idle_phase)*3.0f:0.0f);
                float yaw=-rel*0.34f+(abs_rel<0.2f?sinf(idle_phase*0.7f)*0.07f:0.0f);
                float pitch=0.14f;
                float opacity=mirror?0.32f:(1.0f-abs_rel*0.18f);
                if(opacity<0.22f)opacity=0.22f;

                if(!mirror){
                    int sw=(int)(scale*0.78f);
                    box_alpha((int)cx-sw,floor_y-2,sw*2,5,0xff040608,135);
                }

                int cw=0,ch=0;void *handle=NULL;
                const uint32_t *cover=covers_acquire(&games[idx],&cw,&ch,&handle);
                uint32_t shell=nps64_c3d_shell_color(games[idx].name);
                nps64_c3d_draw_cartridge(platform_framebuffer(),cx,cy,scale,yaw,pitch,shell,accent,cover,cw,ch,mirror,(float)floor_y,opacity);
                covers_release(handle);
            }
        }

        /* Active game title + metadata */
        char gtitle[256];title(games[selected].name,gtitle);
        text_centered(640,468,gtitle,strlen(gtitle)>28?1:2,0xfff8fafc,980);
        const char *rname=games[selected].name;
        const char *region=strstr(rname,"(USA)")||strstr(rname,"(U)")?"USA":strstr(rname,"(Japan)")||strstr(rname,"(J)")?nps64_tr("Japon"):strstr(rname,"(Europe)")||strstr(rname,"(E)")?nps64_tr("Europa"):nps64_tr("Otra region");
        char meta[128];snprintf(meta,sizeof(meta),"Nintendo 64   |   %s   |   %zu / %zu",region,selected+1,count);
        text_centered(640,516,meta,0,0xff94a3b8,600);

        /* 3 rounded color-bordered action cards directly inspired by icon.jpeg (auto-sized so Spanish/English never clips) */
        const uint32_t card_colors[3]={0xff3b82f6,0xff22c55e,0xff64748b};
        const ButtonIcon card_icons[3]={KEY_CROSS,KEY_R2,KEY_L2};
        const char *card_labels[3]={nps64_tr("Jugar ahora"),nps64_tr("Ajustes del juego"),nps64_tr("Configuracion")};
        int card_w[3], total_w=0;
        const int gap=22;
        for(int i=0;i<3;i++){
            int icon_w=card_icons[i]>=KEY_L1?40:26;
            int w=20+icon_w+14+text_width(card_labels[i],0)+24;
            if(w<220)w=220;
            card_w[i]=w;
            total_w+=w;
        }
        total_w+=gap*2;
        int cx_card=(SCREEN_W-total_w)/2;
        for(int i=0;i<3;i++){
            int icon_w=card_icons[i]>=KEY_L1?40:26;
            rounded_box(cx_card,554,card_w[i],72,14,0xff171c23,card_colors[i],2);
            button_icon(cx_card+20,577,card_icons[i],card_colors[i]);
            text(cx_card+20+icon_w+14,578,card_labels[i],0,0xfff1f5f9,card_w[i]-icon_w-44);
            cx_card+=card_w[i]+gap;
        }
    }else{
        text_centered(640,220,nps64_tr("Tu proxima aventura empieza aqui"),2,0xfff3f6fb,980);
        text_centered(640,275,nps64_tr("Copia tus juegos en /data/NPS64/games"),1,0xff94a5bf,900);
        text_centered(640,316,nps64_tr("Formatos .n64, .z64 y .v64"),1,accent,900);
    }

    /* Bottom status & hints bar */
    box(0,646,SCREEN_W,74,0xff14181f);
    int hx=hint(44,653,KEY_LEFT,nps64_tr("Navegar"),0xffcbd5e1);
    hx=hint(hx,653,KEY_SQUARE,nps64_tr("Buscar ROMs"),0xffcbd5e1);
    hx=hint(hx,653,KEY_L1,"L1",0xffcbd5e1);
    hx=hint(hx,653,KEY_R1,nps64_tr("Pagina"),0xffcbd5e1);
    hint(hx,653,KEY_OPTIONS,nps64_tr("Salir"),0xffcbd5e1);
    char cov[96];covers_status(cov,sizeof(cov));
    text(44,683,message&&*message?message:cov,0,accent,1180);
}

void library_ui_modal(const Nps64UpdaterInfo *info,uint32_t accent){
    if(!info)return;
    box_alpha(0,0,SCREEN_W,SCREEN_H,0xff040609,192);
    int mw=590, mh=276, mx=(SCREEN_W-mw)/2, my=(SCREEN_H-mh)/2;
    rounded_box(mx,my,mw,mh,14,0xff171d26,0xff334155,2);
    box(mx+16,my,mw-32,3,accent);

    if(info->state==NPS64_UPDATER_AVAILABLE){
        text(mx+32,my+24,nps64_tr("Actualizacion disponible"),2,0xfff8fafc,mw-64);
        char line[180];
        snprintf(line,sizeof(line),"%s %s   ->   %s %s",nps64_tr("Version actual:"),NPS64_UI_VERSION,nps64_tr("Nueva version:"),info->tag);
        text(mx+32,my+84,line,0,accent,mw-64);
        double mb=(double)info->size/(1024.0*1024.0);
        snprintf(line,sizeof(line),"%s %s (%.1f MB)",nps64_tr("Paquete:"),info->name,mb);
        text(mx+32,my+118,line,0,0xffcbd5e1,mw-64);
        text(mx+32,my+154,nps64_tr("Quieres descargar la actualizacion ahora?"),0,0xff94a3b8,mw-64);

        rounded_box(mx+32,my+204,220,46,10,0xff1e3a5f,0xff3b82f6,2);
        button_icon(mx+50,my+214,KEY_CROSS,0xffffffff);
        text(mx+88,my+214,nps64_tr("Descargar"),0,0xffffffff,150);

        rounded_box(mx+272,my+204,200,46,10,0xff1e242e,0xff64748b,2);
        button_icon(mx+290,my+214,KEY_CIRCLE,0xffffffff);
        text(mx+328,my+214,nps64_tr("Mas tarde"),0,0xffe2e8f0,130);
    }else if(info->state==NPS64_UPDATER_DOWNLOADING){
        text(mx+32,my+24,nps64_tr("Descargando actualizacion..."),2,0xfff8fafc,mw-64);
        text(mx+32,my+82,info->status,0,0xffcbd5e1,mw-64);
        rounded_box(mx+32,my+126,mw-64,24,8,0xff0f141b,0xff2b3646,1);
        int fill_w=(int)((mw-68)*(info->percent>100?100:info->percent)/100);
        if(fill_w>8)rounded_box(mx+34,my+128,fill_w,20,6,accent,accent,0);
        char pct[64];
        snprintf(pct,sizeof(pct),"%u%%  (%.1f / %.1f MB)",info->percent,(double)info->received/(1024.0*1024.0),(double)info->size/(1024.0*1024.0));
        text(mx+32,my+162,pct,0,0xff94a3b8,mw-64);

        rounded_box(mx+32,my+204,210,46,10,0xff2b1e24,0xffef4444,2);
        button_icon(mx+50,my+214,KEY_CIRCLE,0xffffffff);
        text(mx+88,my+214,nps64_tr("Cancelar"),0,0xffffffff,140);
    }else if(info->state==NPS64_UPDATER_READY){
        text(mx+32,my+24,nps64_tr("Actualizacion lista"),2,0xff4ade80,mw-64);
        text(mx+32,my+96,info->status,0,0xffe2e8f0,mw-64);
        rounded_box(mx+32,my+204,210,46,10,0xff1a3326,0xff22c55e,2);
        button_icon(mx+50,my+214,KEY_CROSS,0xffffffff);
        text(mx+88,my+214,nps64_tr("Aceptar"),0,0xffffffff,140);
    }else{
        text(mx+32,my+24,nps64_tr("No se pudo actualizar"),2,0xfff87171,mw-64);
        text(mx+32,my+96,info->status,0,0xffe2e8f0,mw-64);
        rounded_box(mx+32,my+204,210,46,10,0xff1e3a5f,0xff3b82f6,2);
        button_icon(mx+50,my+214,KEY_CROSS,0xffffffff);
        text(mx+88,my+214,nps64_tr("Reintentar"),0,0xffffffff,140);
        rounded_box(mx+262,my+204,190,46,10,0xff1e242e,0xff64748b,2);
        button_icon(mx+280,my+214,KEY_CIRCLE,0xffffffff);
        text(mx+318,my+214,nps64_tr("Cerrar"),0,0xffe2e8f0,120);
    }
}

void library_ui_shutdown(void){covers_shutdown();free(font);font=NULL;}

void library_ui_pause(const char *const *labels,unsigned count,unsigned selected,unsigned slot,uint32_t accent,const char *message,const char *updates) {
    rounded_box(282,80,716,568,12,0xff121519,0xff2b3646,2);
    box(298,80,684,3,accent);
    text(326,103,nps64_tr("Pausa"),2,0xfff3f6fb,620);
    for(unsigned i=0;i<count;i++){
        int y=144+(int)i*(count>10?32:count>7?36:46);
        if(i==selected){rounded_box(318,y,644,count>10?28:count>7?32:42,8,0xff1f2b3a,accent,1);box(318,y+4,3,count>10?20:count>7?24:34,accent);}
        text(336,y+6,labels[i],1,i==selected?0xffffffff:0xffcbd7e7,610);
    }
    char line[128];snprintf(line,sizeof(line),nps64_tr("Ranura %u / 7"),slot);text(326,533,line,0,0xff94a5bf,150);
    button_icon(496,530,KEY_LEFT,0xff94a5bf);hint(530,530,KEY_RIGHT,nps64_tr("Cambiar ranura"),0xff94a5bf);
    int hx=hint(326,605,KEY_CROSS,nps64_tr("Elegir"),0xff94a5bf);hint(hx,605,KEY_CIRCLE,nps64_tr("Continuar"),0xff94a5bf);
    text(326,574,message,0,0xff94a5bf,630);if(updates&&*updates)text(326,552,updates,0,accent,630);
}

void library_ui_cheats(const char *game,const char *const *rows,unsigned count,unsigned selected,unsigned first,unsigned total,uint32_t accent,const char *message){
    library_ui_settings(nps64_tr("Trucos"),game,rows,count,selected,false,1,accent,message);
    char page[64];snprintf(page,sizeof page,"%u-%u / %u",total?first+1:0,total?first+count:0,total);text(1040,91,page,0,0xff94a5bf,190);
    box(190,570,1090,72,0xff121519);
    int x=hint(234,578,KEY_CROSS,nps64_tr("Activar / desactivar"),0xff94a5bf);
    x=hint(x,578,KEY_SQUARE,nps64_tr("Desactivar todos"),0xff94a5bf);hint(x,578,KEY_CIRCLE,nps64_tr("Volver"),0xff94a5bf);
    hint(234,610,KEY_L1,nps64_tr("Pagina anterior"),0xff94a5bf);hint(505,610,KEY_R1,nps64_tr("Pagina siguiente"),0xff94a5bf);
    box(190,680,1090,40,0xff161b22);text(232,686,nps64_tr("R2 en juego: boton GameShark. Algunos cambios requieren reiniciar el juego."),0,0xff94a5bf,990);
}

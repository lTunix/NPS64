/* Native 720p library, antialiased type and cached covers. GPL-2.0-or-later */
#include "library_ui.h"
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
static void title(const char *name,char out[256]) {cover_title(name,out,256);const char *prefix="The Legend of Zelda - ";size_t n=strlen(prefix);if(!strncmp(out,prefix,n)){char short_title[256];snprintf(short_title,sizeof(short_title),"Zelda: %s",out+n);snprintf(out,256,"%s",short_title);}}
#define GLYPH (48*56+1)
static void box(int x,int y,int w,int h,uint32_t color){uint32_t *p=platform_framebuffer();for(int j=y;j<y+h&&j<SCREEN_H;j++)for(int i=x;i<x+w&&i<SCREEN_W;i++)if(i>=0&&j>=0)p[j*SCREEN_W+i]=color;}
static void text(int x,int y,const char *str,unsigned size,uint32_t color,int limit) {
    if(!font){platform_text(x,y,str,color,size>1?2:1);return;}uint32_t *p=platform_framebuffer();int start=x;
    for(const unsigned char *s=(const unsigned char*)str;*s;s++){unsigned c=*s>=32&&*s<128?*s:'?';const unsigned char *g=font+4+(size*96+c-32)*GLYPH;int advance=g[0];if(x+advance>start+limit)break;
        for(int j=0;j<56;j++)for(int i=0;i<48;i++){unsigned a=g[1+j*48+i];if(!a||x+i<0||x+i>=SCREEN_W||y+j<0||y+j>=SCREEN_H)continue;uint32_t old=p[(y+j)*SCREEN_W+x+i],result=0xff000000;for(unsigned sh=0;sh<24;sh+=8)result|=((((color>>sh&255)*a+(old>>sh&255)*(255-a))/255)<<sh);p[(y+j)*SCREEN_W+x+i]=result;}x+=advance;
    }
}
/* Small vector glyphs, sampled at 4x4 for smooth contours at 720p.
 * Coverage is cached once per glyph; redraw only blends the small mask.
 * No allocation or file access in the menu draw path. */
typedef enum {KEY_CROSS,KEY_CIRCLE,KEY_SQUARE,KEY_LEFT,KEY_RIGHT,KEY_L1,KEY_R1,KEY_L2,KEY_R2,KEY_L3,KEY_R3} ButtonIcon;
static float segment_distance(float x,float y,float ax,float ay,float bx,float by) {
    float dx=bx-ax,dy=by-ay,t=((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy);
    if(t<0)t=0;if(t>1)t=1;
    return hypotf(x-ax-t*dx,y-ay-t*dy);
}
static int button_icon(int x,int y,ButtonIcon key,uint32_t color) {
    bool shoulder=key>=KEY_L1;int width=shoulder?40:26,height=26;
    if(key==KEY_CROSS)color=0xff80c8ff;
    if(key==KEY_CIRCLE)color=0xffff8d8d;
    if(key==KEY_SQUARE)color=0xffe6a0d4;
    static unsigned char masks[KEY_R3+1][40*26];static bool ready[KEY_R3+1];
    uint32_t *fb=platform_framebuffer();
    for(int py=0;py<height;py++)for(int px=0;px<width;px++) {
        unsigned coverage=masks[key][py*40+px];
        if(!ready[key]){coverage=0;
        for(unsigned sy=0;sy<4;sy++)for(unsigned sx=0;sx<4;sx++) {
            float u=px+(sx+.5f)/4,v=py+(sy+.5f)/4,d;
            if(shoulder) {
                float qx=fabsf(u-width*.5f)-(width*.5f-6),qy=fabsf(v-13)-7;
                d=fabsf(hypotf(fmaxf(qx,0),fmaxf(qy,0))+fminf(fmaxf(qx,qy),0)-4);
            } else if(key==KEY_CIRCLE)d=fabsf(hypotf(u-13,v-13)-9);
            else if(key==KEY_SQUARE)d=fabsf(fmaxf(fabsf(u-13),fabsf(v-13))-8);
            else if(key==KEY_CROSS)d=fminf(segment_distance(u,v,6,6,20,20),segment_distance(u,v,6,20,20,6));
            else {
                float tip=key==KEY_LEFT?5:21,tail=26-tip;
                d=fminf(segment_distance(u,v,tip,13,tail,13),fminf(segment_distance(u,v,tip,13,13,5),segment_distance(u,v,tip,13,13,21)));
            }
            if(d<1.1f)coverage++;
        }
        masks[key][py*40+px]=(unsigned char)coverage;
        }
        if(!coverage||x+px<0||x+px>=SCREEN_W||y+py<0||y+py>=SCREEN_H)continue;
        uint32_t *pixel=&fb[(y+py)*SCREEN_W+x+px],old=*pixel,result=0xff000000;
        for(unsigned shift=0;shift<24;shift+=8)result|=(((color>>shift&255)*coverage+(old>>shift&255)*(16-coverage))/16)<<shift;
        *pixel=result;
    }
    ready[key]=true;
    if(shoulder) {
        static const char *names[]={"L1","R1","L2","R2","L3","R3"};
        text(x+10,y+1,names[key-KEY_L1],0,color,30);
    }
    return width;
}
static int hint(int x,int y,ButtonIcon key,const char *label,uint32_t color) {
    x+=button_icon(x,y,key,color)+8;text(x,y+1,label,0,color,1000-x+230);
    int width=0;
    for(const unsigned char *c=(const unsigned char*)label;*c;c++)width+=font?font[4+((*c>=32&&*c<128?*c:'?')-32)*GLYPH]:8;
    return x+width+24;
}
void library_ui_init(void){FILE *f=fopen(NPS64_ASSET_ROOT "/ui-font.bin","rb");if(f){size_t bytes=4+4*96*GLYPH;font=malloc(bytes);if(font&&(fread(font,1,bytes,f)!=bytes||memcmp(font,"UIF1",4))){free(font);font=NULL;}fclose(f);}covers_init();}
static void library_sidebar(bool settings,bool focus,unsigned selected,uint32_t accent) {
    text(28,28,"NPS64",3,0xfff3f6fb,160);text(31,88,"by lTunix",0,0xff94a5bf,155);
    const char *labels[]={"Biblioteca","Configuracion"};
    for(unsigned i=0;i<2;i++) {
        int y=159+(int)i*58;bool active=focus?selected==i:settings==(i==1);
        if(active){box(20,y,158,46,focus?0xff29465d:0xff1e3448);box(20,y,3,46,accent);}
        text(32,y+8,labels[i],0,active?0xffffffff:0xff94a5bf,145);
    }
    text(31,297,"NINTENDO 64",0,0xff94a5bf,150);
    text(31,610,"EN DESARROLLO",0,0xff94a5bf,150);text(31,642,"Version " NPS64_UI_VERSION,0,0xff94a5bf,155);
}
void library_ui_settings(const char *heading,const char *subtitle,const char *const *labels,unsigned count,unsigned selected,bool focus,unsigned side,uint32_t accent,const char *message) {
    box(0,0,SCREEN_W,SCREEN_H,0xff0a101c);box(0,0,SCREEN_W,3,accent);box(0,0,190,SCREEN_H,0xff101a2a);
    library_sidebar(true,focus,side,accent);
    text(232,33,heading,2,0xfff3f6fb,1000);text(234,88,subtitle,0,0xff94a5bf,985);
    for(unsigned i=0;i<count;i++) {
        int y=155+(int)i*48;bool active=!focus && i==selected;
        box(232,y,1004,42,active?0xff20354b:0xff111d2d);if(active)box(232,y,3,42,accent);
        text(250,y+5,labels[i],1,active?0xffffffff:0xffd4deeb,960);
    }
    int hx=hint(234,574,KEY_CROSS,"Cambiar / abrir",0xff94a5bf);
    hx+=button_icon(hx,574,KEY_LEFT,0xff94a5bf)+4;
    hx=hint(hx,574,KEY_RIGHT,"Ajustar",0xff94a5bf);
    hint(hx,574,KEY_CIRCLE,"Volver",0xff94a5bf);
    box(190,646,1090,74,0xff101a2a);text(232,656,message,0,accent,990);
    text(232,686,"La biblioteca siempre ocupa 16:9; el formato del juego se guarda por separado.",0,0xff94a5bf,990);
}
void library_ui_draw(const Game *games,size_t count,size_t selected,uint32_t accent,const char *message,const char *updates,bool sidebar_focus,unsigned sidebar_selected) {
    box(0,0,1280,720,0xff0a101c);box(0,0,1280,3,accent);box(0,0,190,720,0xff101a2a);
    library_sidebar(false,sidebar_focus,sidebar_selected,accent);
    text(232,33,"Tu biblioteca",2,0xfff3f6fb,700);char line[160];snprintf(line,sizeof(line),"%zu juegos  /  Tus historias, siempre contigo",count);text(234,80,line,0,0xff94a5bf,900);
    covers_status(line,sizeof(line));text(234,109,line,0,0xff7286a2,980);
    size_t first=selected/6*6;
    if(count){const Game *game=&games[selected];covers_request(game);char name[256];title(game->name,name);
        box(232,145,422,340,0xff162235);box(245,158,396,314,0xff101b2d);text(365,285,"N64",3,0xff2c435d,240);covers_draw(game,245,158,396,314);
        text(234,501,name,strlen(name)>25?1:2,0xfff3f6fb,420);hint(234,553,KEY_CROSS,"Jugar ahora",accent);
        button_icon(234,595,KEY_L3,0xff94a5bf);text(281,596,"+",0,0xff94a5bf,20);
        hint(303,595,KEY_R3,"Menu en partida",0xff94a5bf);
        for(size_t i=first;i<count&&i<first+6;i++){int y=143+(int)(i-first)*75;bool active=i==selected;box(686,y,550,66,active?0xff20354b:0xff111d2d);if(active)box(686,y,3,66,accent);
            covers_request(&games[i]);box(703,y+6,78,54,0xff213148);covers_draw(&games[i],703,y+6,78,54);title(games[i].name,name);text(799,y+7,name,1,active?0xffffffff:0xffd4deeb,412);
            const char *name=games[i].name;const char *region=strstr(name,"(USA)")||strstr(name,"(U)")?"USA":strstr(name,"(Japan)")||strstr(name,"(J)")?"Japon":strstr(name,"(Europe)")||strstr(name,"(E)")?"Europa":"Otra region";
            snprintf(line,sizeof(line),"Nintendo 64   |   %s",region);text(800,y+37,line,0,0xff94a5bf,413);}
        snprintf(line,sizeof(line),"%zu / %zu",selected+1,count);text(687,607,line,0,0xff94a5bf,130);
        button_icon(810,603,KEY_L1,0xff94a5bf);hint(857,603,KEY_R1,"Pagina",0xff94a5bf);
    }else {text(233,230,"Tu proxima aventura empieza aqui",2,0xfff3f6fb,1000);text(234,287,"Copia tus juegos en /data/NPS64/games",1,0xff94a5bf,980);text(234,328,"Formatos .n64, .z64 y .v64",1,accent,980);}
    box(190,646,1090,74,0xff101a2a);
    int hx=hint(230,651,KEY_CROSS,"Jugar",0xffdbe4f0);
    hx=hint(hx,651,KEY_SQUARE,"Buscar ROMs",0xffdbe4f0);
    hx=hint(hx,651,KEY_L2,"Configuracion",0xffdbe4f0);
    hx=hint(hx,651,KEY_R2,"Juego",0xffdbe4f0);
    hint(hx,651,KEY_LEFT,"Menu",0xffdbe4f0);
    text(230,678,updates&&*updates?updates:message,0,accent,1000);
}
void library_ui_shutdown(void){covers_shutdown();free(font);font=NULL;}
void library_ui_pause(const char *const *labels,unsigned count,unsigned selected,unsigned slot,uint32_t accent,const char *message,const char *updates) {
    box(282,80,716,568,0xff050a12);box(290,86,700,552,0xff111d2d);box(290,86,700,3,accent);
    text(326,103,"Pausa",2,0xfff3f6fb,620);
    for(unsigned i=0;i<count;i++){int y=158+(int)i*(count>7?40:46);if(i==selected){box(318,y,644,count>7?36:42,0xff20354b);box(318,y,3,count>7?36:42,accent);}text(336,y+6,labels[i],1,i==selected?0xffffffff:0xffcbd7e7,610);}
    char line[128];snprintf(line,sizeof(line),"Ranura %u / 7",slot);text(326,533,line,0,0xff94a5bf,150);
    button_icon(496,530,KEY_LEFT,0xff94a5bf);hint(530,530,KEY_RIGHT,"Cambiar ranura",0xff94a5bf);
    int hx=hint(326,605,KEY_CROSS,"Elegir",0xff94a5bf);hint(hx,605,KEY_CIRCLE,"Continuar",0xff94a5bf);
    text(326,574,message,0,0xff94a5bf,630);if(updates&&*updates)text(326,552,updates,0,accent,630);
}

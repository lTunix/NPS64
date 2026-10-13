/* Native 720p library, antialiased type and cached covers. GPL-2.0-or-later */
#include "library_ui.h"
#include "platform.h"
#include "covers.h"
#include "cover_match.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
void library_ui_init(void){FILE *f=fopen("/app0/ui-font.bin","rb");if(f){size_t bytes=4+4*96*GLYPH;font=malloc(bytes);if(font&&(fread(font,1,bytes,f)!=bytes||memcmp(font,"UIF1",4))){free(font);font=NULL;}fclose(f);}covers_init();}
void library_ui_draw(const Game *games,size_t count,size_t selected,uint32_t accent,const char *message,const char *updates) {
    box(0,0,1280,720,0xff0a101c);box(0,0,1280,3,accent);box(0,0,190,720,0xff101a2a);
    text(28,28,"NPS64",3,0xfff3f6fb,160);text(31,88,"by lTunix",0,0xff94a5bf,155);
    box(20,159,150,46,0xff1e3448);box(20,159,3,46,accent);text(38,168,"Biblioteca",1,0xffeef4fa,128);
    text(31,231,"NINTENDO 64",0,0xff94a5bf,150);text(31,610,"EN DESARROLLO",0,0xff94a5bf,150);text(31,642,"Version " NPS64_VERSION,0,0xff94a5bf,150);
    text(232,33,"Tu biblioteca",2,0xfff3f6fb,700);char line[160];snprintf(line,sizeof(line),"%zu juegos  /  Tus historias, siempre contigo",count);text(234,80,line,0,0xff94a5bf,900);
    covers_status(line,sizeof(line));text(234,109,line,0,0xff7286a2,980);
    size_t first=selected/6*6;
    if(count){const Game *game=&games[selected];covers_request(game);char name[256];title(game->name,name);
        box(232,145,422,340,0xff162235);box(245,158,396,314,0xff101b2d);text(365,285,"N64",3,0xff2c435d,240);covers_draw(game,245,158,396,314);
        text(234,501,name,strlen(name)>25?1:2,0xfff3f6fb,420);text(234,553,"X  Jugar ahora",1,accent,410);text(234,595,"L3 + R3 abre el menu en partida",0,0xff94a5bf,415);
        for(size_t i=first;i<count&&i<first+6;i++){int y=143+(int)(i-first)*75;bool active=i==selected;box(686,y,550,66,active?0xff20354b:0xff111d2d);if(active)box(686,y,3,66,accent);
            covers_request(&games[i]);box(703,y+6,78,54,0xff213148);covers_draw(&games[i],703,y+6,78,54);title(games[i].name,name);text(799,y+7,name,1,active?0xffffffff:0xffd4deeb,412);
            const char *name=games[i].name;const char *region=strstr(name,"(USA)")||strstr(name,"(U)")?"USA":strstr(name,"(Japan)")||strstr(name,"(J)")?"Japon":strstr(name,"(Europe)")||strstr(name,"(E)")?"Europa":"Otra region";
            snprintf(line,sizeof(line),"Nintendo 64   |   %s",region);text(800,y+37,line,0,0xff94a5bf,413);}
        snprintf(line,sizeof(line),"%zu / %zu     L1 / R1  Cambiar pagina",selected+1,count);text(687,607,line,0,0xff94a5bf,550);
    }else {text(233,230,"Tu proxima aventura empieza aqui",2,0xfff3f6fb,1000);text(234,287,"Copia tus juegos en /data/NPS64/games",1,0xff94a5bf,980);text(234,328,"Formatos .n64, .z64 y .v64",1,accent,980);}
    box(190,646,1090,74,0xff101a2a);text(230,652,"X  Jugar   Cuadrado  Buscar ROMs   L2  Actualizaciones   Triangulo  Mando   Options  Salir",0,0xffdbe4f0,1000);
    text(230,678,updates&&*updates?updates:message,0,accent,1000);
}
void library_ui_shutdown(void){covers_shutdown();free(font);font=NULL;}
void library_ui_pause(const char *const *labels,unsigned count,unsigned selected,unsigned slot,uint32_t accent,const char *message,const char *updates) {
    box(282,80,716,568,0xff050a12);box(290,86,700,552,0xff111d2d);box(290,86,700,3,accent);
    text(326,103,"Pausa",2,0xfff3f6fb,620);
    for(unsigned i=0;i<count;i++){int y=168+(int)i*50;if(i==selected){box(318,y,644,44,0xff20354b);box(318,y,3,44,accent);}text(336,y+6,labels[i],1,i==selected?0xffffffff:0xffcbd7e7,610);}
    char line[128];snprintf(line,sizeof(line),"Ranura %u / 7     Izquierda / Derecha para cambiar",slot);text(326,533,line,0,0xff94a5bf,630);
    text(326,574,message,0,0xff94a5bf,630);if(updates&&*updates)text(326,601,updates,0,accent,630);
}

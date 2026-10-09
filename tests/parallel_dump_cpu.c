/* Replay original ParaLLEl RDP dump through Angrylion. Capture BEFORE
 * importing post-render GPU DRAM, so GPU output cannot become the CPU result. */
#include "n64video.h"
#include "vdac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static uint8_t *ram,*hidden,dmem[4096];
static size_t hidden_size;
static uint32_t dp[8],vi[16],*dp_ptr[8],*vi_ptr[16],mi;
static unsigned frame,capture,post_vi;
static uint32_t color_address,depth_address,color_width=320,color_bytes=2,scissor_height=240;
static struct {uint32_t begin,end;} protected[512];static unsigned protected_count;
static void protect(uint32_t address,uint32_t bytes){
    if(address>=8*1024*1024||bytes>8*1024*1024-address)return;
    for(unsigned i=0;i<protected_count;i++)if(protected[i].begin==address){if(protected[i].end<address+bytes)protected[i].end=address+bytes;return;}
    if(protected_count==512)exit(2);
    protected[protected_count].begin=address;protected[protected_count++].end=address+bytes;
}
static int gpu_output(uint32_t address){for(unsigned i=0;i<protected_count;i++)if(address>=protected[i].begin&&address<protected[i].end)return 1;return 0;}
static const char *directory,*targets;
static void interrupt(void){}
void msg_error(const char *s,...){va_list a;va_start(a,s);vfprintf(stderr,s,a);va_end(a);exit(2);}
void msg_warning(const char *s,...){(void)s;}
void msg_debug(const char *s,...){(void)s;}
void vdac_init(struct n64video_config *c){(void)c;}
void vdac_read(struct frame_buffer *b,bool alpha){(void)b;(void)alpha;}
struct rgba *vdac_acquire(uint32_t w,uint32_t h,uint32_t *p,bool read){(void)w;(void)h;(void)p;(void)read;return NULL;}
void vdac_sync(bool invalid){(void)invalid;}
void vdac_close(void){}
int aleck64_e90_overlay(void *a,int b,int c,int d){(void)a;(void)b;(void)c;(void)d;return 0;}
void vdac_write(struct frame_buffer *b){
 if(!capture||!b->width||!b->height)return;
 char path[512];snprintf(path,sizeof(path),"%s/frame-%u.ppm",directory,frame);
 FILE *out=fopen(path,"wb");if(!out)exit(2);
 fprintf(out,"P6\n%u %u\n255\n",b->width,b->height);
 for(unsigned y=0;y<b->height;y++)for(unsigned x=0;x<b->width;x++){
  struct rgba p=b->pixels[y*b->pitch+x];unsigned char rgb[]={p.r,p.g,p.b};fwrite(rgb,1,3,out);
 }
 fclose(out);printf("REPLAY frame=%u width=%u height=%u VI=%08x origin=%08x; captured before post-render DRAM\n",frame,b->width,b->height,vi[0],vi[1]);
}
static uint32_t word(FILE *f){uint32_t w;if(fread(&w,4,1,f)!=1){fprintf(stderr,"Truncated dump\n");exit(2);}return w;}
static void read_bytes(FILE *f,void *p,size_t n){if(fread(p,1,n,f)!=n){fprintf(stderr,"Truncated bytes\n");exit(2);}}
static int selected(unsigned n){for(const char *s=targets;*s;){char *e;unsigned t=strtoul(s,&e,10);if(e==s)break;if(t==n)return 1;s=*e==','?e+1:e;}return 0;}
int main(int argc,char **argv){
 if(argc!=4)return 2;directory=argv[2];targets=argv[3];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 char magic[8];read_bytes(f,magic,8);if(memcmp(magic,"RDPDUMP2",8))return 2;
 uint32_t size=word(f),gpu_hidden_size=word(f);if(size!=8*1024*1024||gpu_hidden_size>size)return 2;
 ram=calloc(1,size);if(!ram)return 2;
 for(unsigned i=0;i<8;i++)dp_ptr[i]=&dp[i];for(unsigned i=0;i<16;i++)vi_ptr[i]=&vi[i];
 struct n64video_config cfg;n64video_config_init(&cfg);
 cfg.gfx.rdram=ram;cfg.gfx.rdram_size=size;cfg.gfx.dmem=dmem;cfg.gfx.dp_reg=dp_ptr;cfg.gfx.vi_reg=vi_ptr;
 cfg.gfx.mi_intr_reg=&mi;cfg.gfx.mi_intr_cb=interrupt;cfg.parallel=true;cfg.num_workers=6;
 cfg.dp.compat=DP_COMPAT_HIGH;cfg.upscale=1;cfg.vi.mode=VI_MODE_NORMAL;
 cfg.vi.vi_dedither=true;cfg.vi.vi_blur=true; /* Match ParaLLEl default VI filters. */
 n64video_init(&cfg);hidden=n64video_hidden_store(&hidden_size);
 for(;;){
  unsigned type=word(f);
  if(type==6)break;
  if(type==1||type==8){
   unsigned offset=word(f),length=word(f);size_t capacity=type==1?size:hidden_size;
   if(offset>capacity||length>capacity-offset)return 2;
   if(post_vi){
    uint8_t *data=malloc(length);if(!data)return 2;read_bytes(f,data,length);
    /* Retain CPU-rendered colour/depth/coverage across frames. Import other
     * CPU game-data updates, but NEVER replace those buffers with GPU results. */
    if(type==1)for(unsigned i=0;i<length;i++)if(!gpu_output(offset+i))ram[offset+i]=data[i];
    free(data);
   }else read_bytes(f,(type==1?ram:hidden)+offset,length);
  }else if(type==2){
   post_vi=0;(void)word(f);unsigned n=word(f);if(n>44||n<2)return 2;uint32_t commands[44];read_bytes(f,commands,n*4);
   unsigned op=commands[0]>>24&63;
   if(op==0x3f){color_address=commands[1]&0xffffff;color_width=(commands[0]&1023)+1;color_bytes=(commands[0]>>19&3)==3?4:2;}
   if(op==0x3e)depth_address=commands[1]&0xffffff;
   if(op==0x2d)scissor_height=((commands[1]&4095)+3)/4;
   if((op>=8&&op<=15)||op==0x24||op==0x25||op==0x36){
    protect(color_address,color_width*scissor_height*color_bytes);
    if(depth_address)protect(depth_address,color_width*scissor_height*2);
   }
   n64video_set_hle_cmd_buffer(commands,0,n*4);dp[DP_START]=dp[DP_CURRENT]=dp[DP_STATUS]=0;dp[DP_END]=n*4;
   n64video_process_list();n64video_set_hle_cmd_buffer(NULL,0,0);
  }else if(type==3){
   unsigned index=word(f),value=word(f);if(index>=VI_NUM_REG)return 2;vi[index]=value;
   /* scanout emits the complete VI state before GPU post-render DRAM.
    * Capture as soon as YScale is set, before the following memory updates. */
   if(index==VI_Y_SCALE){capture=selected(frame);n64video_update_screen();capture=0;post_vi=1;}
  }else if(type==4){frame++;post_vi=0;}
  else if(type==5){
   uint32_t sync[]={0x29000000,0};n64video_set_hle_cmd_buffer(sync,0,8);
   dp[DP_START]=dp[DP_CURRENT]=dp[DP_STATUS]=0;dp[DP_END]=8;
   n64video_process_list();n64video_set_hle_cmd_buffer(NULL,0,0);
  }else if(type!=7&&type!=9)return 2;
 }
 const char *check=getenv("NPS64_DUMP_CHECK_RGB");
 if(check){unsigned address,expected;uint32_t actual;if(sscanf(check,"%u:%x",&address,&expected)!=2||address>size-4)return 2;memcpy(&actual,ram+address,4);if((actual&0xffffff00u)!=(expected&0xffffff00u)){fprintf(stderr,"GPU post-render data replaced CPU colour: %08x\n",actual);return 2;}}
 n64video_close();free(ram);fclose(f);printf("PASS replayed %u RDP frames\n",frame);return 0;
}

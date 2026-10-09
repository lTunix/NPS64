#pragma once
#include "hle_display_list.h"
void nps64_hle_geometry_begin(void);
void nps64_hle_geometry_log_detail(int enabled);
void nps64_hle_command_seen(void);
unsigned nps64_hle_geometry_commands(void);
int nps64_hle_rectangle_observe(const uint32_t *command,unsigned cycle,unsigned width,unsigned height,float depth);
const HleVertex *nps64_hle_geometry_scene(unsigned width,unsigned height,size_t *count);
const unsigned char *nps64_hle_geometry_depth(void);
int nps64_hle_scene_probe(const HleVertex *,size_t,const unsigned char *,unsigned char *pixels);

void nps64_hle_geometry_reject(void);
int nps64_hle_geometry_complete(void);
void nps64_hle_geometry_failure_log(void);

typedef struct {unsigned origin,framebuffer,width,height,target_width,target_height,pixel_bytes;int left,top,active_width,active_height,x_start,y_start,x_step,y_step;} HleViState;
const HleViState *nps64_hle_vi_state(void);
void nps64_hle_vi_publish(unsigned origin,unsigned width,unsigned height,int left,int top,int active_width,int active_height,int x_start,int y_start,int x_step,int y_step);
void nps64_hle_target_publish(unsigned address,unsigned pixel_bytes);

void nps64_hle_vi_defer_enable(int enabled);
int nps64_hle_vi_defer_allowed(void);
int nps64_hle_cpu_vi_finish(int present);
void nps64_hle_cpu_vi_stats(uint64_t *deferred,uint64_t *resolved);

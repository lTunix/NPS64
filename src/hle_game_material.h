#pragma once
#include "hle_display_list.h"
#include <stdint.h>
#define HLE_ATLAS_SIZE 512
enum {HLE_MATERIAL_VALID=256,HLE_TWO_CYCLE=1,HLE_ALPHA_TEST=2,HLE_BLEND=4,HLE_Z_COMPARE=8,HLE_Z_WRITE=16,HLE_NEEDS_TEXTURE=32,HLE_SCISSOR=64,HLE_ALPHA_COVERAGE=128};
typedef struct {float rect[4];uint32_t wrap;float prim[4],env[4];uint32_t mux[4];float scissor[4],sampling[4];} HleMaterial;
int nps64_hle_material_decode(uint32_t a,uint32_t b,unsigned cycle,unsigned *needs_texture);
void nps64_hle_material_publish(unsigned triangle,uint32_t a,uint32_t b,unsigned cycle,unsigned flags,const unsigned char *prim,const unsigned char *env,unsigned threshold);
void nps64_hle_scissor_publish(unsigned triangle,float left,float top,float right,float bottom);
void nps64_hle_geometry_capture_textures(int enabled);
int nps64_hle_texture_request(const uint32_t *command,unsigned words);
void nps64_hle_texture_publish(unsigned triangle,const unsigned char *rgba,unsigned width,unsigned height,
                              unsigned sl,unsigned tl,unsigned shift_s,unsigned shift_t,unsigned wrap);
const HleMaterial *nps64_hle_geometry_materials(void);
const unsigned char *nps64_hle_geometry_atlas(void);
int nps64_hle_textured_scene_probe(const HleVertex *,size_t,const unsigned char *,const HleMaterial *,const unsigned char *,unsigned char *pixels);
typedef struct {const HleVertex *vertices;size_t count;const unsigned char *depth;const HleMaterial *materials;const unsigned char *atlas;} HleSceneFrame;
typedef struct HleRenderer HleRenderer;
/* One process-owned PS4 context, reused across frames and game sessions.
 * Each successful frame returns after its fence; count=0 clears the image. */
int nps64_hle_renderer_open(HleRenderer **renderer,const HleSceneFrame *scene);
int nps64_hle_renderer_frame(HleRenderer *,const HleSceneFrame *,unsigned char *pixels);
void nps64_hle_renderer_release(HleRenderer *);
typedef struct {
    /* next: 1 valid frame, 0 finish/cancel, -1 failure. Called after GPU fence. */
    int (*next)(unsigned frame,HleSceneFrame *,void *context);
    void (*present)(unsigned frame,const unsigned char *pixels,void *context);
    void *context;
    unsigned frames;
} HleSequence;
int nps64_hle_scene_sequence(const HleSequence *,unsigned char *pixels);

void nps64_hle_sampling_publish(unsigned triangle,unsigned clamp_s,unsigned clamp_t,unsigned mask_s,unsigned mask_t,unsigned filter,unsigned midpoint);

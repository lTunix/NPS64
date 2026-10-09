#pragma once
#include <stddef.h>
#include <stdint.h>

/* Initial F3DEX2 subset. Input is RDRAM, output is expanded GPU triangles.
 * Texture state, lighting and other microcodes are deliberately not accepted. */
typedef struct { float position[4], color[4], uv[2]; } HleVertex;
typedef enum { HLE_DL_OK, HLE_DL_BOUNDS, HLE_DL_OPCODE, HLE_DL_VERTEX,
               HLE_DL_CAPACITY, HLE_DL_LIMIT, HLE_DL_MATRIX } HleDlStatus;
typedef struct {
    const uint8_t *rdram;
    size_t size;
    int word_swapped;
    unsigned texture_width, texture_height, max_commands;
} HleDlInput;
typedef struct {
    HleVertex *vertices;
    size_t capacity, count;
    unsigned commands, triangles;
    uint32_t fault_pc, fault_opcode;
} HleDlOutput;
HleDlStatus hle_decode_f3dex2(const HleDlInput *, uint32_t start, HleDlOutput *);
/* Deterministic fixture encoded in the same byte layout as N64 RDRAM. */
size_t hle_display_list_fixture(uint8_t *rdram, size_t size);

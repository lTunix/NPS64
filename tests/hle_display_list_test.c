#include "hle_display_list.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(24-i*8));
}
int main(void) {
    uint8_t ram[512],swapped[512];HleVertex vertices[6],original[3];
    HleDlInput in={ram,sizeof(ram),0,256,256,64};
    HleDlOutput out={.vertices=vertices,.capacity=6};
    assert(hle_display_list_fixture(ram,sizeof(ram))==512);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OK);
    assert(out.commands==7&&out.triangles==1&&out.count==3);
    assert(fabsf(vertices[0].position[0]+.8f)<.0001f);
    assert(fabsf(vertices[1].position[0]-.8f)<.0001f);
    assert(fabsf(vertices[2].position[1]-.8f)<.0001f);
    assert(vertices[0].position[2]==.5f&&vertices[0].position[3]==1);
    assert(vertices[0].color[0]==1&&vertices[0].color[3]==1);
    assert(fabsf(vertices[0].uv[0]-.1f)<.0001f);
    memcpy(original,vertices,sizeof(original));
    for(unsigned i=0;i<512;i++)swapped[i^3]=ram[i];
    in.rdram=swapped;in.word_swapped=1;
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OK);
    assert(!memcmp(original,vertices,sizeof(original)));
    in.rdram=ram;in.word_swapped=0;
    /* TRI2, including a reversed second triangle. */
    put32(ram+0x110,0x06000204);put32(ram+0x114,0x00040200);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OK&&out.count==6);
    assert(!memcmp(vertices+3,original+2,sizeof(HleVertex)));
    out.capacity=3;assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_CAPACITY);out.capacity=6;
    /* Vertex index not loaded; malformed vertex-cache span; out-of-range DMA. */
    hle_display_list_fixture(ram,sizeof(ram));put32(ram+0x110,0x05000206);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_VERTEX);
    hle_display_list_fixture(ram,sizeof(ram));put32(ram+0x108,0x01003004);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_VERTEX);
    hle_display_list_fixture(ram,sizeof(ram));put32(ram+0x10c,0x1f0);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_BOUNDS);
    hle_display_list_fixture(ram,sizeof(ram));put32(ram+0x100,0xff000000);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OPCODE&&out.fault_pc==0x100&&out.fault_opcode==255);
    assert(hle_decode_f3dex2(&in,1,&out)==HLE_DL_BOUNDS);
    /* Self branch terminates by budget, recursive calls by stack limit. */
    put32(ram,0xde010000);put32(ram+4,0);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_LIMIT&&out.commands==64);
    put32(ram,0xde000000);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_LIMIT&&out.commands==19);
    /* Signed texture coordinates, colors and projection matrix translation. */
    hle_display_list_fixture(ram,sizeof(ram));ram[0x1c8]=0xff;ram[0x1c9]=0xe0;ram[0x1cc]=128;
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OK);
    assert(vertices[0].uv[0]==-1.0f/256&&fabsf(vertices[0].color[0]-128.0f/255)<.00001f);
    /* Matrix .2 * position, translated +.25 in x (last row). */
    ram[0x1b8]=0x40;ram[0x1b9]=0;
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OK);
    assert(fabsf(vertices[0].position[0]+.55f)<.0001f);
    /* Projection load flag after XOR; same transform for identity model. */
    put32(ram+0x100,0xda380007);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_OK);
    assert(fabsf(vertices[0].position[0]+.55f)<.0001f);
    /* Pop without matching push must fail. */
    put32(ram+0x100,0xd8380002);put32(ram+0x104,64);
    assert(hle_decode_f3dex2(&in,0,&out)==HLE_DL_MATRIX);
    puts("PASS F3DEX2: endian layouts, matrix/depth/UV/color, TRI1/TRI2, segments, sublists and malformed input limits");
    return 0;
}

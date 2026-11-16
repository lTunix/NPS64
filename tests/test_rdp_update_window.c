#include "rdp_update_window.h"
#include <assert.h>
#include <stdio.h>

/* Reference implementation of shaders/binning.h bin_primitive(). */
static int ref_bin_primitive(int32_t xh, int32_t xm, int32_t xl,
        int16_t yh, int16_t ym, int16_t yl,
        int32_t dxhdy, int32_t dxmdy, int32_t dxldy, uint8_t flags,
        uint32_t xlo, uint32_t ylo, uint32_t xhi, uint32_t yhi,
        int32_t lo_x, int32_t lo_y, int32_t hi_x, int32_t hi_y, int32_t scaling)
{
    int32_t sc_ylo = scaling * (int32_t)(ylo >> 2);
    int32_t sc_yhi = scaling * (int32_t)((yhi + 3u) >> 2) - 1;
    if (lo_y < sc_ylo) lo_y = sc_ylo;
    if (hi_y > sc_yhi) hi_y = sc_yhi;
    int32_t start_y = lo_y * 4;
    int32_t end_y = hi_y * 4 + 3;
    if (scaling * (int32_t)yh > start_y) start_y = scaling * (int32_t)yh;
    if (scaling * (int32_t)yl - 1 < end_y) end_y = scaling * (int32_t)yl - 1;
    if (end_y < start_y) return 0;

    int32_t ym_s = scaling * (int32_t)ym;
    int32_t ys[4] = {
        start_y, end_y,
        ym_s - 1 < start_y ? start_y : (ym_s - 1 > end_y ? end_y : ym_s - 1),
        ym_s < start_y ? start_y : (ym_s > end_y ? end_y : ym_s)
    };
    int flip = (flags & 1u) != 0;
    int32_t min_x = 0x7fffffff, max_x = -0x7fffffff - 1, max_abs = 0;
    for (int i = 0; i < 4; i++) {
        int32_t l, r;
        nps64_rdp_interp_x(xh, xm, xl, yh, ym, dxhdy, dxmdy, dxldy, ys[i], flip, scaling, &l, &r);
        if (l < min_x) min_x = l;
        if (r > max_x) max_x = r;
        int64_t al = l < 0 ? -(int64_t)l : (int64_t)l;
        int64_t ar = r < 0 ? -(int64_t)r : (int64_t)r;
        int32_t m = (int32_t)(al > ar ? al : ar);
        if (m > max_abs) max_abs = m;
    }
    if (max_abs > 2047 * scaling) {
        min_x = 0;
        max_x = 0x7fffffff;
    }
    int32_t x_bias = (flags & 0x80u) ? 4 : 3;
    int32_t sc_xlo = scaling * (int32_t)(xlo >> 2);
    int32_t sc_xhi = scaling * (int32_t)((xhi + (uint32_t)x_bias) >> 2) - 1;
    if (sc_xlo <= sc_xhi) {
        if (min_x < sc_xlo) min_x = sc_xlo;
        if (min_x > sc_xhi) min_x = sc_xhi;
        if (max_x < sc_xlo) max_x = sc_xlo;
        if (max_x > sc_xhi) max_x = sc_xhi;
    }
    if (min_x < lo_x) min_x = lo_x;
    if (max_x > hi_x) max_x = hi_x;
    return min_x <= max_x;
}

int main(void)
{
    unsigned long cases=0, tile_cases=0;
    for(unsigned width=1;width<=1024;width+=17)
    for(unsigned height=1;height<=1024;height+=19)
    for(unsigned first=0;first<=height;first+=7) {
        Nps64RdpUpdateWindow w=nps64_rdp_update_window(0x1000,0x200000,width,height,first,1);
        unsigned skipped=(w.color-0x1000)/(2*width);
        assert(w.depth-0x200000==w.color-0x1000);
        assert(!(w.color&3)&&!(w.depth&3));
        assert(skipped<=first&&w.height+skipped==height);
        for(unsigned row=first;row<height;row++) {
            /* Every original color/depth location in the touched rows is
             * represented by the reduced dispatch, with the same address. */
            unsigned local=row-skipped;
            assert(local<w.height);
            assert(w.color+local*width*2==0x1000+row*width*2);
            assert(w.depth+local*width*2==0x200000+row*width*2);
        }
        cases++;
    }
    for(unsigned format=0;format<=3;format++) {
        if(format==1)continue;
        Nps64RdpUpdateWindow w=nps64_rdp_update_window(0x1000,0x1000,320,240,160,format);
        assert(w.color==0x1000&&w.depth==0x1000&&w.height==240);
    }
    Nps64RdpUpdateWindow w=nps64_rdp_update_window(0x1002,0x200000,320,240,160,1);
    assert(w.color==0x1002&&w.height==240);
    w=nps64_rdp_update_window(0x1000,0x1000,321,240,161,1);
    assert(w.color==w.depth&&w.height==80);
    w=nps64_rdp_update_window(0x1000,0x200000,320,240,240,1);
    assert(w.color==0x1000&&w.height==240);

    uint64_t rng = 0x9e3779b97f4a7c15ull;
    for (unsigned p = 0; p < 1500; p++) {
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int32_t xh = (int32_t)(rng >> 32) >> 6;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int32_t xm = (int32_t)(rng >> 32) >> 6;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int32_t xl = (int32_t)(rng >> 32) >> 6;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int32_t dxhdy = (int32_t)(rng >> 32) >> 10;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int32_t dxmdy = (int32_t)(rng >> 32) >> 10;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int32_t dxldy = (int32_t)(rng >> 32) >> 10;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        int16_t yh = (int16_t)((int32_t)(rng & 0x7ff) - 64);
        int16_t yl = (int16_t)(yh + (int16_t)((rng >> 12) & 0x3ff));
        int16_t ym = (int16_t)(yh + (int16_t)((rng >> 24) & 0x3ff) - 32);
        uint8_t flags = (uint8_t)(((rng >> 40) & 1u) | (((rng >> 41) & 1u) << 7));
        uint32_t xlo = (uint32_t)((rng >> 42) & 0x3ff);
        uint32_t xhi = xlo + (uint32_t)((rng >> 52) & 0x7ff);
        uint32_t ylo = (uint32_t)((rng >> 16) & 0x1ff);
        uint32_t yhi = ylo + (uint32_t)((rng >> 28) & 0x3ff);

        int32_t min_sub = (int32_t)yh > (int32_t)ylo ? (int32_t)yh : (int32_t)ylo;
        int32_t max_sub = (int32_t)yl - 1 < (int32_t)yhi - 1 ? (int32_t)yl - 1 : (int32_t)yhi - 1;
        int32_t min_line = min_sub >> 2;
        int32_t max_line = max_sub >> 2;
        if (max_line < min_line) continue;

        uint32_t first_row = (uint32_t)(min_line > 1 ? min_line - 1 : 0);
        uint32_t deduced_height = (uint32_t)(max_line >= 0 ? max_line + 1 : 0);
        if (!deduced_height || deduced_height > 240u) deduced_height = 240u;
        uint32_t first_col = 1024u, end_col = 0u;
        nps64_rdp_accumulate_x_bounds(xh, xm, xl, yh, ym, yl, dxhdy, dxmdy, dxldy,
                flags, xlo, ylo, xhi, yhi, 2u, &first_col, &end_col);

        for (unsigned scale = 1; scale <= 2; scale++) {
            uint32_t fb_w = 320u * scale, fb_h = deduced_height * scale;
            uint32_t meta_y = scale == 1 ? 4u : 8u;
            Nps64RdpTileWindow tw = nps64_rdp_tile_window(fb_w, fb_h, first_col, end_col, first_row, scale, 8u, meta_y);
            assert(tw.first_meta_x * 8u <= tw.first_tile_x);
            assert((tw.first_meta_x + tw.count_meta_x) * 8u >= tw.first_tile_x + tw.count_tile_x);
            assert(tw.first_meta_y * meta_y <= tw.first_tile_y);
            assert((tw.first_meta_y + tw.count_meta_y) * meta_y >= tw.first_tile_y + tw.count_tile_y);

            uint32_t tiles_x = (fb_w + 7u) / 8u, tiles_y = (fb_h + 7u) / 8u;
            for (uint32_t ty = 0; ty < tiles_y; ty++) {
                for (uint32_t tx = 0; tx < tiles_x; tx++) {
                    int32_t lo_x = (int32_t)(tx * 8u), lo_y = (int32_t)(ty * 8u);
                    int32_t hi_x = lo_x + 7 < (int32_t)fb_w - 1 ? lo_x + 7 : (int32_t)fb_w - 1;
                    int32_t hi_y = lo_y + 7 < (int32_t)fb_h - 1 ? lo_y + 7 : (int32_t)fb_h - 1;
                    if (ref_bin_primitive(xh, xm, xl, yh, ym, yl, dxhdy, dxmdy, dxldy,
                            flags, xlo, ylo, xhi, yhi, lo_x, lo_y, hi_x, hi_y, (int32_t)scale)) {
                        assert(tx >= tw.first_tile_x && tx < tw.first_tile_x + tw.count_tile_x);
                        assert(ty >= tw.first_tile_y && ty < tw.first_tile_y + tw.count_tile_y);
                    }
                    tile_cases++;
                }
            }
        }
    }
    printf("PASS: %lu active-row windows and %lu 2D tile-binning oracle checks\n",cases,tile_cases);
}

#ifndef NPS64_RDP_UPDATE_WINDOW_H
#define NPS64_RDP_UPDATE_WINDOW_H
#include <stdint.h>
/* Pre/post domain updates operate on linear memory, not image coordinates.
 * Keep whole words and preserve the renderer's original upper bound. */
typedef struct {
    uint32_t color, depth, height;
} Nps64RdpUpdateWindow;
static inline Nps64RdpUpdateWindow nps64_rdp_update_window(uint32_t color,
        uint32_t depth, uint32_t width, uint32_t height, uint32_t first_row,
        unsigned pixel_size_log2)
{
    Nps64RdpUpdateWindow w = {color, depth, height};
    /* Restrict the first implementation to aligned 16-bit buffers. Other
     * formats keep the established path, including 32-bit color/depth aliases. */
    if (pixel_size_log2 != 1 || ((color | depth) & 3) || !width ||
        first_row >= height || width > 1024 || height > 1024)
        return w;
    if (width & 1u)
        first_row &= ~1u;
    uint32_t offset = first_row * width * 2;
    w.color += offset;
    w.depth += offset;
    w.height -= first_row;
    return w;
}

typedef struct {
    uint32_t first_tile_x, count_tile_x;
    uint32_t first_tile_y, count_tile_y;
    uint32_t first_meta_x, count_meta_x;
    uint32_t first_meta_y, count_meta_y;
} Nps64RdpTileWindow;

static inline int32_t nps64_rdp_clamp_i32(int64_t v)
{
    if (v < -2147483647 - 1) return -2147483647 - 1;
    if (v > 2147483647) return 2147483647;
    return (int32_t)v;
}

static inline void nps64_rdp_interp_x(int32_t xh, int32_t xm, int32_t xl,
        int16_t yh, int16_t ym, int32_t dxhdy, int32_t dxmdy, int32_t dxldy,
        int32_t y, int flip, int32_t scaling, int32_t *out_left, int32_t *out_right)
{
    int64_t yh_base = (int64_t)(yh & ~3) * scaling;
    int64_t ym_base = (int64_t)ym * scaling;
    int64_t xh_val = (int64_t)scaling * xh + (int64_t)(y - yh_base) * dxhdy;
    int64_t xm_val = (int64_t)scaling * xm + (int64_t)(y - yh_base) * dxmdy;
    int64_t xl_val = (int64_t)scaling * xl + (int64_t)(y - ym_base) * dxldy;
    if (y < scaling * (int32_t)ym)
        xl_val = xm_val;
    int32_t xh_s = nps64_rdp_clamp_i32(xh_val) >> 15;
    int32_t xl_s = nps64_rdp_clamp_i32(xl_val) >> 15;
    *out_left = flip ? xh_s : xl_s;
    *out_right = flip ? xl_s : xh_s;
}

static inline int nps64_rdp_primitive_x_bounds_scaled(
        int32_t xh, int32_t xm, int32_t xl, int16_t yh, int16_t ym, int16_t yl,
        int32_t dxhdy, int32_t dxmdy, int32_t dxldy, uint8_t flags,
        uint32_t xlo, uint32_t ylo, uint32_t xhi, uint32_t yhi,
        int32_t scaling, uint32_t *first_col, uint32_t *end_col)
{
    int32_t lo_y = scaling * (int32_t)(ylo >> 2);
    int32_t hi_y = scaling * (int32_t)((yhi + 3u) >> 2) - 1;
    int32_t start_y = lo_y * 4;
    int32_t end_y = hi_y * 4 + 3;
    if (scaling * (int32_t)yh > start_y) start_y = scaling * (int32_t)yh;
    if (scaling * (int32_t)yl - 1 < end_y) end_y = scaling * (int32_t)yl - 1;
    if (end_y < start_y)
        return 0;

    int32_t ym_s = scaling * (int32_t)ym;
    int32_t ys[4] = {
        start_y,
        end_y,
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
    int32_t sc_lo = scaling * (int32_t)(xlo >> 2);
    int32_t sc_hi = scaling * (int32_t)((xhi + (uint32_t)x_bias) >> 2) - 1;
    if (sc_lo > sc_hi) {
        *first_col = 0;
        *end_col = 1024u;
        return 1;
    }
    if (min_x < sc_lo) min_x = sc_lo;
    if (min_x > sc_hi) min_x = sc_hi;
    if (max_x < sc_lo) max_x = sc_lo;
    if (max_x > sc_hi) max_x = sc_hi;
    if (max_x < min_x || max_x < 0)
        return 0;
    if (min_x < 0) min_x = 0;
    uint32_t c0 = (uint32_t)min_x / (uint32_t)scaling;
    uint32_t c1 = (uint32_t)max_x / (uint32_t)scaling + 1u;
    if (c1 > 1024u) c1 = 1024u;
    *first_col = c0;
    *end_col = c1;
    return 1;
}

static inline void nps64_rdp_accumulate_x_bounds(
        int32_t xh, int32_t xm, int32_t xl, int16_t yh, int16_t ym, int16_t yl,
        int32_t dxhdy, int32_t dxmdy, int32_t dxldy, uint8_t flags,
        uint32_t xlo, uint32_t ylo, uint32_t xhi, uint32_t yhi,
        unsigned upscaling, uint32_t *first_col, uint32_t *end_col)
{
    uint32_t c0 = 0, c1 = 0;
    if (nps64_rdp_primitive_x_bounds_scaled(xh, xm, xl, yh, ym, yl, dxhdy, dxmdy, dxldy,
            flags, xlo, ylo, xhi, yhi, 1, &c0, &c1)) {
        if (c0 < *first_col) *first_col = c0;
        if (c1 > *end_col) *end_col = c1;
    }
    if (upscaling > 1u && nps64_rdp_primitive_x_bounds_scaled(xh, xm, xl, yh, ym, yl,
            dxhdy, dxmdy, dxldy, flags, xlo, ylo, xhi, yhi, (int32_t)upscaling, &c0, &c1)) {
        if (c0 < *first_col) *first_col = c0;
        if (c1 > *end_col) *end_col = c1;
    }
}

static inline Nps64RdpTileWindow nps64_rdp_tile_window(
        uint32_t fb_width, uint32_t fb_height,
        uint32_t first_col_unscaled, uint32_t end_col_unscaled,
        uint32_t first_row_unscaled, unsigned scale,
        uint32_t meta_tiles_x, uint32_t meta_tiles_y)
{
    uint32_t total_tiles_x = (fb_width + 7u) / 8u;
    uint32_t total_tiles_y = (fb_height + 7u) / 8u;
    uint32_t total_meta_x = meta_tiles_x ? (total_tiles_x + meta_tiles_x - 1u) / meta_tiles_x : 0u;
    uint32_t total_meta_y = meta_tiles_y ? (total_tiles_y + meta_tiles_y - 1u) / meta_tiles_y : 0u;
    Nps64RdpTileWindow w = {0, total_tiles_x, 0, total_tiles_y, 0, total_meta_x, 0, total_meta_y};
    if (!scale || !total_tiles_x || !total_tiles_y || !meta_tiles_x || !meta_tiles_y ||
        fb_width > 2048u || fb_height > 2048u)
        return w;

    uint32_t first_px_y = first_row_unscaled * scale;
    if (first_px_y < fb_height) {
        uint32_t first_ty = first_px_y / 8u;
        if (first_ty < total_tiles_y) {
            w.first_tile_y = first_ty;
            w.count_tile_y = total_tiles_y - first_ty;
            w.first_meta_y = first_ty / meta_tiles_y;
            w.count_meta_y = total_meta_y - w.first_meta_y;
        }
    }

    if (first_col_unscaled < end_col_unscaled && first_col_unscaled * scale < fb_width) {
        uint32_t first_tx = (first_col_unscaled * scale) / 8u;
        uint32_t end_px_x = end_col_unscaled * scale;
        if (end_px_x > fb_width) end_px_x = fb_width;
        uint32_t end_tx = (end_px_x + 7u) / 8u;
        if (end_tx > total_tiles_x) end_tx = total_tiles_x;
        if (first_tx < end_tx) {
            uint32_t first_mx = first_tx / meta_tiles_x;
            uint32_t end_mx = (end_tx + meta_tiles_x - 1u) / meta_tiles_x;
            if (end_mx > total_meta_x) end_mx = total_meta_x;
            if (first_mx < end_mx) {
                w.first_tile_x = first_tx;
                w.count_tile_x = end_tx - first_tx;
                w.first_meta_x = first_mx;
                w.count_meta_x = end_mx - first_mx;
            }
        }
    }
    return w;
}
#endif

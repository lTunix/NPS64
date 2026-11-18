/* Low-overhead 3D N64 cartridge + floor reflection rasterizer for 720p UI.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_CARTRIDGE3D_H
#define NPS64_CARTRIDGE3D_H

#include "platform.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    float sx, sy, inv_z;
    float u, v;
} Nps64ProjVtx;

static inline uint32_t nps64_c3d_shade(uint32_t rgb, float lit) {
    if (lit < 0.0f) lit = 0.0f;
    if (lit > 1.35f) lit = 1.35f;
    unsigned r = (unsigned)fminf(255.0f, ((rgb >> 16) & 255u) * lit);
    unsigned g = (unsigned)fminf(255.0f, ((rgb >> 8) & 255u) * lit);
    unsigned b = (unsigned)fminf(255.0f, (rgb & 255u) * lit);
    return 0xff000000u | (r << 16) | (g << 8) | b;
}

static inline uint32_t nps64_c3d_blend(uint32_t dst, uint32_t src, unsigned a) {
    if (a >= 255u) return 0xff000000u | (src & 0x00ffffffu);
    if (!a) return dst;
    unsigned ia = 255u - a;
    unsigned r = (((src >> 16) & 255u) * a + ((dst >> 16) & 255u) * ia) / 255u;
    unsigned g = (((src >> 8) & 255u) * a + ((dst >> 8) & 255u) * ia) / 255u;
    unsigned b = ((src & 255u) * a + (dst & 255u) * ia) / 255u;
    return 0xff000000u | (r << 16) | (g << 8) | b;
}

static inline Nps64ProjVtx nps64_c3d_proj(
    float x, float y, float z, float u, float v,
    float yaw, float pitch, float cx, float cy, float scale, float floor_y, bool mirror)
{
    float cyaw = cosf(yaw), syaw = sinf(yaw);
    float cpitch = cosf(pitch), spitch = sinf(pitch);
    /* Rotate around Y (yaw) then X (pitch) */
    float x1 = x * cyaw + z * syaw;
    float z1 = -x * syaw + z * cyaw;
    float y2 = y * cpitch - z1 * spitch;
    float z2 = y * spitch + z1 * cpitch;
    float cam_z = 3.8f + z2;
    if (cam_z < 0.8f) cam_z = 0.8f;
    float inv_z = 1.0f / cam_z;
    float px = cx + x1 * scale * inv_z * 3.8f;
    float py = cy - y2 * scale * inv_z * 3.8f;
    if (mirror) py = 2.0f * floor_y - py;
    Nps64ProjVtx out = { px, py, inv_z, u, v };
    return out;
}

static inline float nps64_c3d_edge(float ax, float ay, float bx, float by, float cx, float cy) {
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

/* Draw a textured or solid quad (v0,v1,v2,v3 in winding order) with optional floor fade. */
static void nps64_c3d_quad(
    uint32_t *fb, const Nps64ProjVtx v[4],
    uint32_t solid_color, float lit,
    const uint32_t *tex, int tw, int th,
    bool mirror, float floor_y, float max_fade_px, float base_alpha)
{
    float min_x = v[0].sx, max_x = v[0].sx, min_y = v[0].sy, max_y = v[0].sy;
    for (int i = 1; i < 4; i++) {
        if (v[i].sx < min_x) min_x = v[i].sx;
        if (v[i].sx > max_x) max_x = v[i].sx;
        if (v[i].sy < min_y) min_y = v[i].sy;
        if (v[i].sy > max_y) max_y = v[i].sy;
    }
    int x0 = (int)floorf(min_x); if (x0 < 0) x0 = 0;
    int x1 = (int)ceilf(max_x);  if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
    int y0 = (int)floorf(min_y); if (y0 < 0) y0 = 0;
    int y1 = (int)ceilf(max_y);  if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
    if (mirror && y0 < (int)floor_y) y0 = (int)floor_y;
    if (x0 > x1 || y0 > y1) return;

    uint32_t shaded = nps64_c3d_shade(solid_color, lit);
    const int tris[2][3] = { {0, 1, 2}, {0, 2, 3} };

    for (int t = 0; t < 2; t++) {
        const Nps64ProjVtx *a = &v[tris[t][0]];
        const Nps64ProjVtx *b = &v[tris[t][1]];
        const Nps64ProjVtx *c = &v[tris[t][2]];
        float area = nps64_c3d_edge(a->sx, a->sy, b->sx, b->sy, c->sx, c->sy);
        if (fabsf(area) < 0.5f) continue;
        float inv_area = 1.0f / area;

        for (int py = y0; py <= y1; py++) {
            float fade = base_alpha;
            if (mirror) {
                float d = (float)py - floor_y;
                if (d < 0.0f || d >= max_fade_px) continue;
                float k = 1.0f - d / max_fade_px;
                fade *= k * k;
            }
            unsigned alpha = (unsigned)(fade * 255.0f);
            if (!alpha) continue;

            float fy = (float)py + 0.5f;
            uint32_t *row = fb + py * SCREEN_W;
            for (int px = x0; px <= x1; px++) {
                float fx = (float)px + 0.5f;
                float w0 = nps64_c3d_edge(b->sx, b->sy, c->sx, c->sy, fx, fy) * inv_area;
                float w1 = nps64_c3d_edge(c->sx, c->sy, a->sx, a->sy, fx, fy) * inv_area;
                float w2 = nps64_c3d_edge(a->sx, a->sy, b->sx, b->sy, fx, fy) * inv_area;
                if (w0 < -0.001f || w1 < -0.001f || w2 < -0.001f) continue;

                uint32_t col = shaded;
                if (tex && tw > 0 && th > 0) {
                    float iz = w0 * a->inv_z + w1 * b->inv_z + w2 * c->inv_z;
                    if (iz > 1e-6f) {
                        float u = (w0 * a->u * a->inv_z + w1 * b->u * b->inv_z + w2 * c->u * c->inv_z) / iz;
                        float vv = (w0 * a->v * a->inv_z + w1 * b->v * b->inv_z + w2 * c->v * c->inv_z) / iz;
                        int tx = (int)(u * (float)(tw - 1));
                        int ty = (int)(vv * (float)(th - 1));
                        if (tx < 0) tx = 0; else if (tx >= tw) tx = tw - 1;
                        if (ty < 0) ty = 0; else if (ty >= th) ty = th - 1;
                        uint32_t texel = tex[ty * tw + tx];
                        col = nps64_c3d_shade(texel, lit);
                    }
                }
                row[px] = alpha >= 254u ? col : nps64_c3d_blend(row[px], col, alpha);
            }
        }
    }
}

/* Choose authentic N64 shell color based on title (Zelda gold, DK yellow, classic gray). */
static inline uint32_t nps64_c3d_shell_color(const char *name) {
    if (!name) return 0xff6b727e;
    if (strstr(name, "Zelda") || strstr(name, "Majora") || strstr(name, "Ocarina")) return 0xffc49a2c;
    if (strstr(name, "Donkey Kong")) return 0xffc9a828;
    if (strstr(name, "Spider-Man")) return 0xffa63434;
    if (strstr(name, "Rayman") || strstr(name, "Tonic")) return 0xff3b7a4c;
    return 0xff6b727e;
}

static void nps64_c3d_draw_cartridge(
    uint32_t *fb, float cx, float cy, float scale,
    float yaw, float pitch, uint32_t shell_color, uint32_t accent,
    const uint32_t *cover, int cw, int ch,
    bool mirror, float floor_y, float opacity)
{
    /* N64 cartridge normalized half-extents: W=0.68, H=0.47, D=0.11 */
    const float W = 0.68f, H = 0.47f, D = 0.11f;
    const float RW = 0.62f, RH = 0.49f; /* top ridge slightly narrower/taller */

    /* Determine which side is visible from yaw */
    float syaw = sinf(yaw);

    /* 1. Side thickness face (left if yaw > 0, right if yaw < 0) */
    if (fabsf(syaw) > 0.03f) {
        float sx = syaw > 0.0f ? -W : W;
        Nps64ProjVtx side[4] = {
            nps64_c3d_proj(sx,  H - 0.04f,  D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(sx,  H - 0.04f, -D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(sx, -H,         -D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(sx, -H,          D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        nps64_c3d_quad(fb, side, shell_color, 0.58f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
    }

    /* 2. Top beveled ridge */
    {
        Nps64ProjVtx top[4] = {
            nps64_c3d_proj(-RW, RH, -D * 0.85f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( RW, RH, -D * 0.85f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( RW, H,   D,         0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-RW, H,   D,         0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        nps64_c3d_quad(fb, top, shell_color, 1.18f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
    }

    /* 3. Main front shell */
    {
        Nps64ProjVtx front[4] = {
            nps64_c3d_proj(-W,  H, D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( W,  H, D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( W, -H, D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-W, -H, D, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        nps64_c3d_quad(fb, front, shell_color, 0.96f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
    }

    /* 4. Side grip notches (subtle darker shoulders on N64 cart) */
    {
        Nps64ProjVtx grip_l[4] = {
            nps64_c3d_proj(-W,         H * 0.65f, D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-W * 0.83f, H * 0.65f, D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-W * 0.83f, -H,        D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-W,         -H,        D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        Nps64ProjVtx grip_r[4] = {
            nps64_c3d_proj(W * 0.83f,  H * 0.65f, D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(W,          H * 0.65f, D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(W,         -H,         D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(W * 0.83f, -H,         D + 0.002f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        nps64_c3d_quad(fb, grip_l, shell_color, 0.80f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
        nps64_c3d_quad(fb, grip_r, shell_color, 0.80f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
    }

    /* 5. Bottom PCB connector lip */
    {
        Nps64ProjVtx lip[4] = {
            nps64_c3d_proj(-W * 0.72f, -H + 0.08f, D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( W * 0.72f, -H + 0.08f, D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( W * 0.72f, -H,         D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-W * 0.72f, -H,         D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        nps64_c3d_quad(fb, lip, 0xff222730, 1.0f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
    }

    /* 6. Recessed label bezel + Cover art (or stylized N64 placeholder) */
    {
        const float LW = 0.48f, LT = 0.38f, LB = -0.32f;
        Nps64ProjVtx bezel[4] = {
            nps64_c3d_proj(-LW - 0.025f, LT + 0.025f, D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( LW + 0.025f, LT + 0.025f, D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( LW + 0.025f, LB - 0.025f, D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-LW - 0.025f, LB - 0.025f, D + 0.003f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        nps64_c3d_quad(fb, bezel, 0xff161b23, 1.0f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);

        Nps64ProjVtx label[4] = {
            nps64_c3d_proj(-LW, LT, D + 0.005f, 0.0f, 0.0f, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( LW, LT, D + 0.005f, 1.0f, 0.0f, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj( LW, LB, D + 0.005f, 1.0f, 1.0f, yaw, pitch, cx, cy, scale, floor_y, mirror),
            nps64_c3d_proj(-LW, LB, D + 0.005f, 0.0f, 1.0f, yaw, pitch, cx, cy, scale, floor_y, mirror),
        };
        if (cover && cw > 0 && ch > 0) {
            nps64_c3d_quad(fb, label, 0xffffffff, 1.02f, cover, cw, ch, mirror, floor_y, 92.0f, opacity);
        } else {
            nps64_c3d_quad(fb, label, 0xff1f2836, 1.0f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
            Nps64ProjVtx strip[4] = {
                nps64_c3d_proj(-LW, LT,        D + 0.006f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
                nps64_c3d_proj( LW, LT,        D + 0.006f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
                nps64_c3d_proj( LW, LT - 0.11f, D + 0.006f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
                nps64_c3d_proj(-LW, LT - 0.11f, D + 0.006f, 0, 0, yaw, pitch, cx, cy, scale, floor_y, mirror),
            };
            nps64_c3d_quad(fb, strip, accent, 1.0f, NULL, 0, 0, mirror, floor_y, 92.0f, opacity);
        }
    }
}

#endif

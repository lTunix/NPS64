/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_UPDATE_FEED_H
#define NPS64_UPDATE_FEED_H
#include <stddef.h>
#include <stdint.h>
#define NPS64_RELEASES_URL "https://api.github.com/repos/lTunix/NPS64/releases?per_page=20"
#define NPS64_MAX_PACKAGE (512ULL * 1024 * 1024)
typedef struct {
    char tag[64], name[128], url[512];
    unsigned char digest[32];
    uint64_t size;
} Nps64Release;
/* 1: newer downloadable release, 0: current, -1: invalid/incomplete feed. */
int nps64_release_find(const char *json, size_t size, const char *current, Nps64Release *release);
#endif

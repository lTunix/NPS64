/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_SHA256_H
#define NPS64_SHA256_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint32_t state[8]; uint64_t bytes; unsigned char block[64]; size_t used; } Nps64Sha256;
void nps64_sha256_init(Nps64Sha256 *hash);
void nps64_sha256_update(Nps64Sha256 *hash,const void *bytes,size_t size);
void nps64_sha256_final(Nps64Sha256 *hash,unsigned char digest[32]);
#endif

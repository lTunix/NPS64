/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_UPDATER_H
#define NPS64_UPDATER_H
#include <stdbool.h>
#include <stddef.h>
void nps64_updater_init(void);
void nps64_updater_request(void);
void nps64_updater_status(char *text,size_t capacity,bool *busy);
void nps64_updater_shutdown(void);
#endif

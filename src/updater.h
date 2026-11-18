/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_UPDATER_H
#define NPS64_UPDATER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NPS64_UPDATER_IDLE = 0,
    NPS64_UPDATER_CHECKING,
    NPS64_UPDATER_AVAILABLE,
    NPS64_UPDATER_DOWNLOADING,
    NPS64_UPDATER_READY,
    NPS64_UPDATER_UP_TO_DATE,
    NPS64_UPDATER_ERROR
} Nps64UpdaterState;

typedef struct {
    Nps64UpdaterState state;
    bool busy;
    bool dismissed;
    char tag[64];
    char name[128];
    uint64_t size;
    uint64_t received;
    unsigned percent;
    char status[160];
} Nps64UpdaterInfo;

void nps64_updater_init(void);
void nps64_updater_set_confirm_mode(bool enabled);
void nps64_updater_request(void);
void nps64_updater_confirm(void);
void nps64_updater_dismiss(void);
void nps64_updater_cancel(void);
void nps64_updater_status(char *text,size_t capacity,bool *busy);
void nps64_updater_info(Nps64UpdaterInfo *info);
void nps64_updater_shutdown(void);
#endif

#ifndef NPS64_COVERS_H
#define NPS64_COVERS_H
#include "storage.h"
#include <stdint.h>
void covers_init(void);
void covers_request(const Game *game);
void covers_refresh(void);
void covers_set_paused(bool paused);
void covers_draw(const Game *game,int x,int y,int w,int h);
const uint32_t *covers_acquire(const Game *game,int *w,int *h,void **handle);
void covers_release(void *handle);
void covers_status(char *text,size_t capacity);
void covers_shutdown(void);
#endif

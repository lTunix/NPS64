#ifndef NPS64_COVERS_H
#define NPS64_COVERS_H
#include "storage.h"
void covers_init(void);
void covers_request(const Game *game);
void covers_refresh(void);
void covers_draw(const Game *game,int x,int y,int w,int h);
void covers_status(char *text,size_t capacity);
void covers_shutdown(void);
#endif

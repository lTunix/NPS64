#ifndef NPS64_LIBRARY_UI_H
#define NPS64_LIBRARY_UI_H
#include "storage.h"
#include <stdint.h>
void library_ui_init(void);
void library_ui_draw(const Game *games,size_t count,size_t selected,uint32_t accent,const char *message,const char *updates);
void library_ui_shutdown(void);
void library_ui_pause(const char *const *labels,unsigned count,unsigned selected,unsigned slot,uint32_t accent,const char *message,const char *updates);
#endif

#ifndef NPS64_STORAGE_H
#define NPS64_STORAGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PATH_CAP 1024
typedef struct { char path[PATH_CAP],name[256],key[40]; } Game;
bool rom_info(const char *path, Game *game);
bool rom_content_key(Game *game);
size_t library_scan(const char *dir, Game *games, size_t capacity);
bool file_write_atomic(const char *path,const void *data,size_t size);
bool file_read_exact(const char *path,void *data,size_t size);
bool state_write(const char *path,const char *key,const void *data,size_t size);
bool state_read(const char *path,const char *key,void *data,size_t size);
#endif

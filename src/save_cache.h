/* Session-local baseline of bytes known to have reached disk. */
#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef struct {unsigned char *bytes;size_t size;int valid;} Nps64SaveCache;
static inline void nps64_save_cache_reset(Nps64SaveCache *s){free(s->bytes);memset(s,0,sizeof(*s));}
static inline int nps64_save_cache_equal(const Nps64SaveCache *s,const void *data,size_t size){return data&&s->valid&&s->size==size&&!memcmp(s->bytes,data,size);}
static inline void nps64_save_cache_remember(Nps64SaveCache *s,const void *data,size_t size){
 if(!data||!size){nps64_save_cache_reset(s);return;}
 if(!s->bytes||s->size!=size){unsigned char *p=realloc(s->bytes,size);if(!p){s->valid=0;return;}s->bytes=p;s->size=size;}
 memcpy(s->bytes,data,size);s->valid=1;
}

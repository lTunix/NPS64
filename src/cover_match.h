#ifndef NPS64_COVER_MATCH_H
#define NPS64_COVER_MATCH_H
#include <stddef.h>
#include <stdbool.h>
void cover_title(const char *rom,char *text,size_t capacity);
bool cover_match(const char *catalog,const char *rom,char *filename,size_t capacity);
bool cover_url(const char *filename,char *url,size_t capacity);
void cover_key(const char *rom,char key[17]);
#endif

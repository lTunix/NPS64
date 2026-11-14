#ifndef NPS64_LOG_H
#define NPS64_LOG_H
#include <stdarg.h>
#include <stdbool.h>
bool nps64_log_init(const char *path);
void nps64_log(const char *format,...) __attribute__((format(printf,1,2)));
void nps64_vlog(const char *format,va_list args);
bool nps64_log_ok(void);
unsigned nps64_log_dropped(void);
bool nps64_log_async(void);
void nps64_log_close(void);
#endif

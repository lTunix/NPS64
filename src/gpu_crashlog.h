#pragma once
#include <stdbool.h>
/* Isolated GPU diagnostics only. Does not recover from a fatal signal. */
bool nps64_gpu_crashlog_init(const char *path);

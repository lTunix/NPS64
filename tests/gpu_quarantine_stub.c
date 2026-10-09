/* A quarantined build must never even attempt Vulkan initialization. */
#include <stdlib.h>
#include <stdbool.h>
bool __wrap_nps64_gpu_begin(void) {abort();}

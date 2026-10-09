/* Exercise GPU initialization failure after context_reset, including teardown. */
#include <stdlib.h>
int __real_nps64_parallel_ready(void);
int __wrap_nps64_parallel_ready(void) {
    return getenv("NPS64_TEST_GPU_NOT_READY")?0:__real_nps64_parallel_ready();
}

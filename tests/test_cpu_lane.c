/* Compare the actual patched renderer's reciprocal remainder with C division. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define STRICTINLINE inline __attribute__((always_inline))
#include "cpu_lane_generated.h"
int main(void) {
    uint32_t random=0x9e3779b9u;
    for(uint32_t stride=1;stride<=64;stride++) {
        uint64_t reciprocal=((uint64_t)UINT32_MAX+stride)/stride;
        for(int line=-8192;line<=8192;line++)
            assert(nps64_lane_remainder((uint32_t)line,stride,reciprocal)==(uint32_t)line%stride);
        for(unsigned i=0;i<131072;i++) {
            random^=random<<13;random^=random>>17;random^=random<<5;
            assert(nps64_lane_remainder(random,stride,reciprocal)==random%stride);
        }
    }
    puts("PASS lane remainder: 64 strides, negative/zero/positive scanlines and 8M random uint32 values");
    return 0;
}

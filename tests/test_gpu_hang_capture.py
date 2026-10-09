"""Exercise the actual capture helper against bounded mock BO mappings."""
from pathlib import Path
import subprocess

root = Path('/work')
work = root / 'build/test-gpu-hang-capture'
work.mkdir(parents=True, exist_ok=True)
s = (root / 'vendor/gpu/mesa-26.0.8/src/amd/common/ac_ps4_drm.c').read_text()
start = s.index('/* NPS64: bounded capture')
end = s.index('static unsigned ps4_walk_lines;', start)
helper = s[start:end].replace('/data/NPS64/', str(work) + '/')
harness = r'''
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
struct amdgpu_bo { void *cpu; uint64_t last_va, size; unsigned handle; };
typedef struct { unsigned num_bos; struct amdgpu_bo **bos; } ac_drm_device;
''' + helper + r'''
int main(void) {
   uint32_t data[16];
   for (unsigned i = 0; i < 16; i++) data[i] = i + 0x12340000;
   struct amdgpu_bo good = {data, 0x2000, sizeof(data), 1};
   struct amdgpu_bo unmapped = {NULL, 0x3000, 4096, 2};
   struct amdgpu_bo *bos[] = {NULL, &good, &unmapped};
   ac_drm_device dev = {3, bos};
   FILE *f = tmpfile(); assert(f);
   nps64_capture_va(&dev, f, 0x203c, false); /* Last word only. */
   nps64_capture_va(&dev, f, 0x2040, false); /* Exactly outside. */
   nps64_capture_va(&dev, f, 0x3000, false); /* Unmapped live BO. */
   nps64_capture_va(&dev, f, UINT64_MAX, false);
   nps64_capture_va(&dev, f, 0x2000, true); /* Limited to 64 bytes. */
   nps64_capture_data(&dev, f, 0x203c, UINT32_MAX); /* Limit to remaining BO bytes. */
   nps64_capture_data(&dev, f, UINT64_MAX, UINT32_MAX);
   nps64_capture_data(&dev, f, 0x3000, UINT32_MAX);
   rewind(f); char report[4096] = {0}; fread(report, 1, sizeof(report)-1, f);
   assert(strstr(report, "bytes 4\n"));
   assert(strstr(report, "+000: 1234000f"));
   assert(strstr(report, "VA 0x2040 unavailable"));
   assert(strstr(report, "VA 0x3000 unavailable"));
   assert(strstr(report, "VA 0xffffffffffffffff unavailable"));
   assert(strstr(report, "64/64 bytes")); fclose(f);
   return 0;
}
'''
(work / 'test.c').write_text(harness)
subprocess.run(['clang-14', '-fsanitize=address,undefined', '-g',
                str(work/'test.c'), '-o', str(work/'test')], check=True)
subprocess.run([str(work/'test')], check=True)
assert (work/'gpu-hang-shader-2000.bin').read_bytes() == b''.join(
    (i+0x12340000).to_bytes(4, 'little') for i in range(16))
assert (work/'gpu-hang-data-203c.bin').read_bytes() == (0x1234000f).to_bytes(4, 'little')
print('PASS: hang capture reads only live mapped BOs; end/overflow/unmapped guards; exact bounded shader bytes; ASan/UBSan')

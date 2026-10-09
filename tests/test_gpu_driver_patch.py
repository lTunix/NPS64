"""Verify reproducible PS4 driver patching and the GNM handoff ordering."""
from pathlib import Path
import shutil,subprocess
root=Path('/work');work=root/'build/test-gpu-driver-patch'
mesa=work/'src';(mesa/'amd/common').mkdir(parents=True,exist_ok=True)
(mesa/'amd/vulkan').mkdir(parents=True,exist_ok=True);(mesa/'util').mkdir(parents=True,exist_ok=True)
for name in ['amd/vulkan/radv_cmd_buffer.c','util/u_qsort.h','amd/common/ac_gpu_info.c']:
 shutil.copyfile(root/'vendor/gpu/mesa-26.0.8/src'/name,mesa/name)
source=root/'reference/DolphinPS4/port/mesa/ac_ps4_drm.c'
target=mesa/'amd/common/ac_ps4_drm.c'
files=[target,mesa/'amd/vulkan/radv_cmd_buffer.c',mesa/'util/u_qsort.h']
subprocess.run(['python3','scripts/patch_gpu_driver.py',str(source),str(target)],cwd=root,check=True)
first=[p.read_bytes() for p in files]
subprocess.run(['python3','scripts/patch_gpu_driver.py',str(source),str(target)],cwd=root,check=True)
assert first==[p.read_bytes() for p in files], 'patches must be idempotent'
s=target.read_text();handoff=s.index('int r = dev->submit(submit_ibs')
assert 'volatile("sfence" ::: "memory")' in s[handoff-150:handoff]
assert '/data/DolphinPS4' not in s and '/data/NPS64' in s
s=files[1].read_text();start=s.index('radv_emit_shader_prefetch(');end=s.index('ALWAYS_INLINE',start)
body=s[start:end];assert '#ifdef __PS4__' in body and 'return;\n#endif' in body
assert body.index('return;\n#endif')<body.index('radv_cs_cp_dma_prefetch')
assert 'RADV_PS4_TRACE' in (root/'src/gpu_vulkan.c').read_text()
s=target.read_text()
serial=s.index('while (serial_submit && ps4_completed(dev) < dev->last_seq)')
assert serial < s.index('const uint64_t seq = ++dev->last_seq;',serial)
assert 'simple_mtx_unlock(&dev->lock);' in s[serial:serial+300]
assert 'ps4_wait_seq(dev, pending, INT64_MAX);' in s[serial:serial+300]
s=files[1].read_text();start=s.index('\nradv_after_dispatch(');end=s.index('\nvoid\n',start)
body=s[start:end]
assert '#ifdef __PS4__' in body and 'radv_emit_cache_flush(cmd_buffer);' in body
for flag in ('CS_PARTIAL_FLUSH','INV_VCACHE','INV_SCACHE','WB_L2','INV_L2'):
 assert 'RADV_CMD_FLAG_'+flag in body
assert body.index('radv_cmd_buffer_after_draw') < body.index('radv_emit_cache_flush')
print('PASS: idempotent PS4 patches; WC publication; no shader prefetch; single-flight fence ordering; per-dispatch completion and cache synchronization')

assert 'info->cu_info.max_sgpr_alloc = 96;' in (mesa/'amd/common/ac_gpu_info.c').read_text()

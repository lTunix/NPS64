#!/usr/bin/env python3
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text().replace('/data/DolphinPS4','/data/NPS64')
a=s.index('   fputs(line, stderr);', s.index('ps4_log(const char'))
b=s.index('\n}',a)
s=s[:a]+'   extern void nps64_log(const char *, ...);\n   nps64_log("[radv] %s", line);'+s[b:]
# CPU writes into WC/Garlic shader buffers must be visible before GNM sees IBs.
old='   int r = dev->submit(submit_ibs, dcb, dcb_sizes, ccb, ccb_sizes);'
assert s.count(old)==1, 'Review GNM submission entry point'
s=s.replace(old, '   __asm__ volatile("sfence" ::: "memory"); /* NPS64: publish WC shader uploads. */\n'+old)
# Keep one submission in flight while investigating the GNM compute path.
old='   /* The fence IB slot for the next sequence number must be free (its previous user finished).'
assert s.count(old)==1, 'Review GNM fence-slot serialization'
s=s.replace(old, '''   /* NPS64: opt-in single-flight GNM submissions. Does not reset a hung GPU. */
   static int serial_submit = -1;
   if (serial_submit < 0)
      serial_submit = getenv("RADV_PS4_SERIAL_SUBMIT") != NULL;
   while (serial_submit && ps4_completed(dev) < dev->last_seq) {
      const uint64_t pending = dev->last_seq;
      simple_mtx_unlock(&dev->lock);
      ps4_wait_seq(dev, pending, INT64_MAX);
      simple_mtx_lock(&dev->lock);
   }
''' + old)
# Capture shader bytes and descriptor words only when a hang is already reported.
# Restrict every read to a live, CPU-mapped BO; a VA-window check is insufficient.
helper = r'''
/* NPS64: bounded capture of live shader and descriptor memory. */
static void
nps64_capture_data(ac_drm_device *dev, FILE *f, uint64_t va, uint32_t requested)
{
   if (!requested)
      return;
   for (unsigned h = 1; h < dev->num_bos; h++) {
      struct amdgpu_bo *bo = dev->bos[h];
      if (!bo || !bo->cpu || !bo->last_va || va < bo->last_va)
         continue;
      const uint64_t offset = va - bo->last_va;
      if (offset >= bo->size)
         continue;
      size_t bytes = requested;
      if (bytes > bo->size - offset)
         bytes = bo->size - offset;
      if (bytes > 16 * 1024 * 1024)
         bytes = 16 * 1024 * 1024;
      char path[128];
      snprintf(path, sizeof(path), "/data/NPS64/gpu-hang-data-%llx.bin",
               (unsigned long long)va);
      FILE *out = fopen(path, "wb");
      if (out) {
         const size_t written = fwrite((const char *)bo->cpu + offset, 1, bytes, out);
         fclose(out);
         fprintf(f, "  data file %s: %zu/%zu bytes, descriptor requested %u\n",
                 path, written, bytes, requested);
      } else {
         fprintf(f, "  data file open failed for %#llx\n", (unsigned long long)va);
      }
      return;
   }
}

static void
nps64_capture_va(ac_drm_device *dev, FILE *f, uint64_t va, bool shader)
{
   for (unsigned h = 1; h < dev->num_bos; h++) {
      struct amdgpu_bo *bo = dev->bos[h];
      if (!bo || !bo->cpu || !bo->last_va || va < bo->last_va)
         continue;
      const uint64_t offset = va - bo->last_va;
      if (offset >= bo->size)
         continue;
      const size_t available = bo->size - offset;
      const size_t bytes = available < (shader ? 16384 : 256) ?
                           available : (shader ? 16384 : 256);
      const uint32_t *data = (const uint32_t *)((const char *)bo->cpu + offset);
      fprintf(f, "  capture %s VA %#llx BO %u offset %#llx bytes %zu\n",
              shader ? "shader" : "descriptor", (unsigned long long)va,
              bo->handle, (unsigned long long)offset, bytes);
      if (shader) {
         char path[128];
         snprintf(path, sizeof(path), "/data/NPS64/gpu-hang-shader-%llx.bin",
                  (unsigned long long)va);
         FILE *out = fopen(path, "wb");
         if (out) {
            const size_t written = fwrite(data, 1, bytes, out);
            fclose(out);
            fprintf(f, "  shader file %s: %zu/%zu bytes\n", path, written, bytes);
         } else {
            fprintf(f, "  shader file open failed\n");
         }
      } else {
         for (size_t word = 0; word < bytes / 4; word++)
            fprintf(f, "    +%03zx: %08x\n", word * 4, data[word]);
         /* Raw buffer descriptors have four words. Texture descriptors can
          * occupy eight; only candidates pointing inside live BOs are read. */
         for (size_t word = 0; word + 3 < bytes / 4; word += 4) {
            const uint64_t base = data[word] | ((uint64_t)(data[word + 1] & 0xffff) << 32);
            nps64_capture_data(dev, f, base, data[word + 2]);
         }
      }
      return;
   }
   fprintf(f, "  capture VA %#llx unavailable in live mapped BOs\n",
           (unsigned long long)va);
}
'''
old='static unsigned ps4_walk_lines;'
assert s.count(old)==1
s=s.replace(old, helper+'\n'+old)
old='   struct ac_addr_info info;\n   for (unsigned i = 0; i < num_dw && ps4_walk_lines < 4000;) {'
assert s.count(old)==1
s=s.replace(old, '''   struct ac_addr_info info;
   uint32_t compute_pgm_lo = 0;
   uint32_t compute_user[16] = {0};
   for (unsigned i = 0; i < num_dw && ps4_walk_lines < 4000;) {''')
old='      if ((op == 0x3f || op == 0x33) && body >= 3) { /* INDIRECT_BUFFER(_CONST) */'
assert s.count(old)==1
s=s.replace(old, '''      if (body > num_dw - i - 1) {
         fprintf(f, "truncated PM4 packet; capture stopped\\n");
         return;
      }
      if (op == 0x76 && body >= 2) { /* SET_SH_REG */
         for (unsigned j = 1; j < body; j++) {
            const unsigned reg = (b[0] & 0xffff) + j - 1;
            if (reg == 0x20c)
               compute_pgm_lo = b[j];
            if (reg >= 0x240 && reg < 0x250)
               compute_user[reg - 0x240] = b[j];
         }
      }
      if ((op == 0x15 || op == 0x16) && compute_pgm_lo) {
         nps64_capture_va(dev, f, (uint64_t)compute_pgm_lo << 8, true);
         for (unsigned j = 2; j < 6; j++) {
            fprintf(f, "  COMPUTE_USER_DATA_%u = %08x\\n", j, compute_user[j]);
            nps64_capture_va(dev, f, dev->va32_start | compute_user[j], false);
         }
      }
''' + old)
old='      fprintf(f, "==== buffers ====\\n");'
assert s.count(old)==1
s=s.replace(old, old+'''
      for (unsigned h = 1; h < dev->num_bos; h++) {
         const struct amdgpu_bo *bo = dev->bos[h];
         if (bo)
            fprintf(f, "  live BO %u VA %#llx size %#llx heap %#x flags %#llx mapped %s\\n",
                    bo->handle, (unsigned long long)bo->last_va,
                    (unsigned long long)bo->size, bo->heap,
                    (unsigned long long)bo->flags, bo->cpu ? "yes" : "no");
      }
''')
Path(sys.argv[2]).write_text(s)

p=Path(sys.argv[2]).parents[1] / 'vulkan/radv_cmd_buffer.c'
s=p.read_text()
marker='NPS64: avoid shader self-copy prefetch on the GNM path.'
if marker not in s:
    old='radv_emit_shader_prefetch(struct radv_cmd_buffer *cmd_buffer, struct radv_shader *shader)\n{'
    assert s.count(old)==1, 'Review shader prefetch entry point'
    s=s.replace(old, old+'\n#ifdef __PS4__\n   /* '+marker+' */\n   return;\n#endif')
    p.write_text(s)

s=p.read_text()
marker='NPS64: finish each PS4 compute dispatch before changing its pipeline.'
if marker not in s:
    old='   radv_cmd_buffer_after_draw(cmd_buffer, RADV_CMD_FLAG_CS_PARTIAL_FLUSH, dgc);\n}'
    assert s.count(old)==2, 'Review compute and ray tracing after-dispatch hooks'
    # First occurrence is radv_after_dispatch; leave ray tracing unchanged.
    s=s.replace(old, '''   radv_cmd_buffer_after_draw(cmd_buffer, RADV_CMD_FLAG_CS_PARTIAL_FLUSH, dgc);
#ifdef __PS4__
   /* '''+marker+''' */
   cmd_buffer->state.flush_bits |= RADV_CMD_FLAG_CS_PARTIAL_FLUSH |
      RADV_CMD_FLAG_INV_VCACHE | RADV_CMD_FLAG_INV_SCACHE |
      RADV_CMD_FLAG_WB_L2 | RADV_CMD_FLAG_INV_L2;
   radv_emit_cache_flush(cmd_buffer);
#endif
}''',1)
    p.write_text(s)

p=Path(sys.argv[2]).parents[2] / 'util/u_qsort.h'
s=p.read_text()
if 'OpenOrbis does not expose the C11 declaration' not in s:
    s=s.replace('#ifdef __cplusplus', '#ifdef __PS4__\n#undef HAVE_QSORT_S\n#define HAVE_QSORT_S 0 /* OpenOrbis does not expose the C11 declaration. */\n#endif\n\n#ifdef __cplusplus', 1)
    p.write_text(s)

# Reserve room for the special scalar registers before ACO allocation. GFX7
# can otherwise round 104 addressable SGPRs plus VCC up to 112 in RSRC1.
# This changes allocation (and spilling), never truncates compiled metadata.
p=Path(sys.argv[2]).parents[1] / 'common/ac_gpu_info.c'
s=p.read_text()
marker='NPS64: conservative GFX7 scalar register allocation'
if marker not in s:
    old='   ac_fill_cu_info(info, &device_info);'
    assert s.count(old)==1, 'Review CU register limits initialization'
    s=s.replace(old, old+'''
#ifdef __PS4__
   /* NPS64: conservative GFX7 scalar register allocation. */
   if (info->gfx_level == GFX7)
      info->cu_info.max_sgpr_alloc = 96;
#endif
''')
    p.write_text(s)

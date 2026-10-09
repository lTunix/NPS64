"""Apply bounded, checked edits to the build copy, keeping upstream pristine."""
from pathlib import Path
import sys

root = Path(sys.argv[1])

# The supplied Mario Tennis VC dump has a stale IPL3 checksum. Repair only
# its checked in-memory header, after retaining the original database identity.
path = root / 'mupen64plus-core/src/main/rom.c'
text = path.read_text()
text = '#include "nps64_rom_compat.h"\n' + text
old = '    DebugMessage(M64MSG_VERBOSE, "Save type: %d", ROM_SETTINGS.savetype);'
assert text.count(old) == 1, 'Review ROM compatibility hook ordering'
text = text.replace(old, old + '''
    if (nps64_rom_compat((uint8_t*)mem_base_u32(g_mem_base, MM_CART_ROM), g_rom_size, digest))
        DebugMessage(M64MSG_INFO, "NPS64 compatibility: Mario Tennis VC runtime IPL3 CRC repaired; original ROM identity retained");''')
path.write_text(text)
path.with_name('nps64_rom_compat.h').write_bytes(
    (Path(__file__).resolve().parent.parent / 'src/rom_compat.h').read_bytes())

# Preserve unsigned scanline ownership without dividing on every line.
# ceil(2**32 / stride) estimates the quotient exactly or one too high.
# An unsigned remainder >= stride identifies that overestimate; adding the
# stride corrects it, including wrapped products at negative scanlines.
path = root / 'mupen64plus-video-angrylion/n64video/rdp/rasterizer.c'
text = path.read_text()
helper = '''static STRICTINLINE uint32_t nps64_lane_remainder(uint32_t line,
        uint32_t stride, uint64_t reciprocal)
{
    uint32_t quotient = (uint32_t)(((uint64_t)line * reciprocal) >> 32);
    uint32_t remainder = line - quotient * stride;
    return remainder + (stride & (0u - (uint32_t)(remainder >= stride)));
}

'''
start = text.index('static void edgewalker_for_prims(')
end = text.index('static void rasterizer_init(', start)
fast = text[start:end].replace('static void edgewalker_for_prims(', 'static void nps64_edgewalker_fast(', 1)
old = '    const int jcur = ycur >> 2, jfar = ylfar >> 2, jclose = yhclose >> 2;'
assert fast.count(old) == 1, 'Review rasterizer lane origin'
fast = fast.replace(old, old + '''
    const uint32_t lane_stride = state[wid].stride | !state[wid].stride;
    const uint64_t lane_reciprocal = ((uint64_t)UINT32_MAX + lane_stride) / lane_stride;''')
old = '(!state[wid].stride || (jj) % state[wid].stride == state[wid].offset)'
assert fast.count(old) == 1, 'Review rasterizer lane ownership'
fast = fast.replace(old, '(!state[wid].stride || nps64_lane_remainder((uint32_t)(jj), lane_stride, lane_reciprocal) == state[wid].offset)')
dispatch = '''static unsigned nps64_lane_profile;
/* Set before starting a content session, while the renderer is idle. */
void nps64_core_game_profile(unsigned enabled) { nps64_lane_profile = enabled; }
static INLINE void nps64_edgewalker_dispatch(uint32_t wid, int32_t *args)
{
    if (nps64_lane_profile) nps64_edgewalker_fast(wid, args);
    else edgewalker_for_prims(wid, args);
}

'''
text = helper + text[:end] + fast + dispatch + text[end:]
old = '    edgewalker_for_prims(wid, ewdata);'
assert text.count(old) == 11, 'Review all triangle/rectangle entry points'
text = text.replace(old, '    nps64_edgewalker_dispatch(wid, ewdata);')

# A two-cycle RGBA16 span has stable framebuffer/perspective functions for
# the whole primitive. Keep the generic path and its PGO counters intact;
# only the matching Zelda path calls inline copies instead of indirect
# functions for each pixel.
def function_body(source, name):
    import re
    match = re.search(r'static void ' + name + r'\([^;{}]*\)\s*\{', source)
    assert match, 'Review function definition: ' + name
    opening = source.index('{', match.start())
    depth = 1
    position = opening + 1
    while depth:
        depth += (source[position] == '{') - (source[position] == '}')
        position += 1
    return source[match.start():position]

helpers = []
for filename, name in [('fbuffer.c', 'fbread2_16'), ('fbuffer.c', 'fbwrite_16'),
                       ('tcoord.c', 'tcdiv_persp'), ('tcoord.c', 'tcdiv_nopersp')]:
    source = (root / 'mupen64plus-video-angrylion/n64video/rdp' / filename).read_text()
    body = function_body(source, name)
    helpers.append(body.replace('static void ' + name + '(',
                               'static STRICTINLINE void nps64_' + name + '(', 1))

specialized = []
for name in ['render_spans_2cycle_complete', 'render_spans_2cycle_notexelnext',
             'render_spans_2cycle_notexel1']:
    body = function_body(text, name)
    body = body.replace('static void ' + name + '(', 'static void nps64_fast_' + name + '(', 1)
    for field, target in [('tcdiv_ptr', 'tcdiv_persp'), ('fbread2_ptr', 'fbread2_16'),
                          ('fbwrite_ptr', 'fbwrite_16')]:
        old = 'state[wid].' + field + '('
        assert old in body, 'Review hot span call: ' + field
        body = body.replace(old, 'nps64_' + target + '(')
    specialized.append(body)
    specialized.append(body.replace('nps64_fast_' + name + '(',
                                    'nps64_flat_' + name + '(', 1)
                           .replace('nps64_tcdiv_persp(', 'nps64_tcdiv_nopersp('))
    specialized.append('''static INLINE void nps64_dispatch_%s(uint32_t wid, int start,
        int end, int tilenum, int flip)
{
    if (nps64_lane_profile && state[wid].fbread2_ptr == fbread2_16 &&
            state[wid].fbwrite_ptr == fbwrite_16)
    {
        if (state[wid].tcdiv_ptr == tcdiv_persp)
        {
            nps64_fast_%s(wid, start, end, tilenum, flip);
            return;
        }
        if (state[wid].tcdiv_ptr == tcdiv_nopersp)
        {
            nps64_flat_%s(wid, start, end, tilenum, flip);
            return;
        }
    }
    %s(wid, start, end, tilenum, flip);
}
''' % (name, name, name, name))
    old = name + '(wid, yhlimit >> 2, yllimit >> 2, tilenum, flip);'
    assert text.count(old) == 2, 'Review both edgewalker paths: ' + name
    text = text.replace(old, 'nps64_dispatch_' + old)

text = text.replace('static unsigned nps64_lane_profile;\n', '', 1)
insertion = text.index('static void edgewalker_for_prims(')
text = ('static unsigned nps64_lane_profile;\n' + text[:insertion] +
        '\n\n'.join(helpers + specialized) + '\n\n' + text[insertion:])
path.write_text(text)

# Configure actual render threads once, rather than changing the frontend's
# worker count or requesting CPU cores outside the process affinity mask.
path = root / 'mupen64plus-video-angrylion/parallel_al.c'
text = path.read_text()
old = '#define PARALLEL_LINE_PAD 64'
assert text.count(old) == 1
text = text.replace(old, '''#ifdef __ORBIS__
extern void nps64_cpu_worker_enter(unsigned) __attribute__((weak));
#endif
''' + old)
old = '    int accept_work;'
assert text.count(old) == 1
text = text.replace(old, old + '\n#ifdef __ORBIS__\n    int main_configured;\n#endif')
old = '    int seen = 0;'
assert text.count(old) == 1
text = text.replace(old, old + '''
#ifdef __ORBIS__
    if (nps64_cpu_worker_enter) nps64_cpu_worker_enter(worker_id);
#endif''')
old = 'void parallel_run(void task(uint32_t))\n{\n    struct parallel_pool *p = &pool;'
assert text.count(old) == 1
text = text.replace(old, old + '''
#ifdef __ORBIS__
    if (!p->main_configured)
    {
        if (nps64_cpu_worker_enter) nps64_cpu_worker_enter(0);
        p->main_configured = 1;
    }
#endif''')
path.write_text(text)

path = root / 'Makefile'
text = path.read_text()
start = text.index('ifeq "$(shell expr `gcc -dumpversion`')
end = text.index('\nendif', start) + len('\nendif')
old = text[start:end]
assert old.count('CPUFLAGS += -fcommon') == 1, 'Review upstream gcc-only flag probe'
path.write_text(text[:start] + '# NPS64: common symbols are required with clang too.\nCPUFLAGS += -fcommon' + text[end:])
text = path.read_text()
text = text.replace('BUILD_FLAG_STATE := ', 'BUILD_FLAG_STATE := RDP_PROFILE=$(NPS64_RDP_PROFILE_FLAGS) ', 1)
text += '\n# Profile-guided optimization applies only to the software rasterizer.\n'
text += 'mupen64plus-video-angrylion/n64video.o: CFLAGS += $(NPS64_RDP_PROFILE_FLAGS)\n'
path.write_text(text)
path = root / 'libretro/libretro.c'
text = path.read_text()
old = '#include <glsm/glsm.h>\nbool glsm_ctl(enum glsm_state_ctl state, void *data) { return false; }'
new = '/* NPS64 software-only: avoid pulling desktop GL headers into glsm stub. */\n#define glsm_ctl(state, data) false'
assert text.count(old) == 1, 'Upstream glsm stub changed: review software-only patch'
path.write_text(text.replace(old, new))

# Main-thread renderer timers and actual worker count for PS4 diagnosis.
path = root / 'mupen64plus-video-angrylion/interface.c'
text = path.read_text()
text += '''
#include "parallel_al.h"
extern double nps64_profile_time(void);
static double nps64_rdp_seconds, nps64_vi_seconds;
unsigned nps64_core_workers(void) { return config.parallel ? parallel_num_workers() : 0; }
void nps64_core_perf_take(double *rdp, double *vi) {
    *rdp = nps64_rdp_seconds; *vi = nps64_vi_seconds;
    nps64_rdp_seconds = nps64_vi_seconds = 0;
}
'''
# Declarations must precede the instrumented entry points.
decl = 'extern double nps64_profile_time(void);\nstatic double nps64_rdp_seconds, nps64_vi_seconds;\n'
text = decl + text.replace(decl, '')
for call, counter in [('rdp_emit_hle_process_dlist();', 'nps64_rdp_seconds'),
                      ('\n  n64video_process_list();', 'nps64_rdp_seconds'),
                      ('n64video_update_screen();', 'nps64_vi_seconds')]:
    assert text.count(call) == 1, 'Review Angrylion profiling entry point: ' + call
    text = text.replace(call, '{ double started = nps64_profile_time(); ' + call +
                        ' ' + counter + ' += nps64_profile_time() - started; }')
path.write_text(text)

# PS4 alternative to the tile shader that stalls after marker 5 in 00.14.
# Shader bytecode is generated from project GLSL and copied into the build tree.
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders/nps64_tile_direct.hpp'
path.write_bytes((Path(__file__).resolve().parents[1] / 'src/gpu_shaders/tile_binning_direct_spv.hpp').read_bytes())
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders/nps64_color.hpp'
path.write_bytes((Path(__file__).resolve().parents[1] / 'src/gpu_shaders/color_raster_spv.hpp').read_bytes())
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.hpp'
text = path.read_text()
old = '\t\tbool ubershader = false;'
assert text.count(old) == 1
path.write_text(text.replace(old, old + '\n\t\tbool direct_binning = false;\n\t\tbool color_only = false;'))
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.cpp'
text = path.read_text()
text = text.replace('#include "rdp_renderer.hpp"', '#include "rdp_renderer.hpp"\n#include "shaders/nps64_tile_direct.hpp"\n#include "shaders/nps64_color.hpp"', 1)
old = '\tbool allow_subgroup = true;'
assert text.count(old) == 1
text = text.replace(old, '''\tif (const char *direct = getenv("NPS64_RDP_DIRECT_BINNING"))
\t{
\t\tcaps.direct_binning = strtol(direct, nullptr, 0) > 0 && caps.ubershader;
\t\tLOGI("NPS64 direct tile binning = %d; local=64, no LDS/atomics/barriers\\n", int(caps.direct_binning));
\t}

''' + old)
old = '\t\tcmd.set_program(shader_bank->ubershader);'
assert text.count(old) == 1
text = text.replace(old, '''\t\tbool color_batch = caps.color_only && stream.static_raster_state_cache.size() > 0;
\t\tconst uint32_t texture_flags = RASTERIZATION_COPY_BIT | RASTERIZATION_USES_TEXEL0_BIT |
\t\t\tRASTERIZATION_USES_TEXEL1_BIT | RASTERIZATION_USES_PIPELINED_TEXEL1_BIT | RASTERIZATION_USES_LOD_BIT;
\t\tfor (unsigned i = 0; i < stream.static_raster_state_cache.size(); i++)
\t\t\tcolor_batch = color_batch && !(stream.static_raster_state_cache.data()[i].flags & texture_flags);
\t\tif (color_batch)
\t\t{
\t\t\tconst auto &layout = shader_bank->ubershader->get_shader(Vulkan::ShaderStage::Compute)->get_layout();
\t\t\tcmd.set_program(device->request_program(nps64_color_spirv, sizeof(nps64_color_spirv), &layout));
\t\t\tstatic unsigned logged = 0;
\t\t\tif (logged++ < 4) LOGI("NPS64 color-only raster batch\\n");
\t\t}
\t\telse
\t\t\tcmd.set_program(shader_bank->ubershader);''')
old = '\tbool allow_subgroup = true;'
text = text.replace(old, '\tif (const char *color = getenv("NPS64_RDP_COLOR_ONLY"))\n\t\tcaps.color_only = strtol(color, nullptr, 0) > 0;\n\n' + old, 1)
old = '\tif (caps.subgroup_tile_binning)\n\t{'
assert text.count(old) == 1
text = text.replace(old, '''\tif (caps.direct_binning)
\t{
\t\tconst auto &direct_layout = shader_bank->tile_binning_combined->get_shader(Vulkan::ShaderStage::Compute)->get_layout();
\t\tcmd.set_program(device->request_program(nps64_tile_direct_spirv, sizeof(nps64_tile_direct_spirv), &direct_layout));
\t\tsubgroup_size = 64;
\t}
\telse if (caps.subgroup_tile_binning)
\t{''')
old = '\tcmd.dispatch(num_primitives_32, num_meta_tiles_x, num_meta_tiles_y);'
assert text.count(old) == 1
text = text.replace(old, '\tcmd.dispatch(caps.direct_binning ? 1 : num_primitives_32, num_meta_tiles_x, num_meta_tiles_y);')
path.write_text(text)

# Expose a checked permission probe before the frontend selects Ari64.
path = root / 'libretro/libretro.c'
text = path.read_text()
text += '''
/* NPS64: no JIT selection until the existing near-ELF cache is executable. */
#include <sys/mman.h>
int nps64_core_jit_supported(void)
{
#if defined(NEW_DYNAREC) && NEW_DYNAREC == 2
    return mprotect(g_dev.r4300.extra_memory, sizeof(g_dev.r4300.extra_memory),
                    PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
#else
    return 0;
#endif
}
'''
path.write_text(text)
text = path.read_text()
old = '   mupen_main_exit();\n\n   deinit_audio_libretro();'
new = '   if (emu_initialized) mupen_main_exit();\n   CoreShutdown(); /* NPS64: release core memory/config between content sessions. */\n\n   deinit_audio_libretro();'
assert text.count(old) == 1, 'Review upstream core deinitialization'
text = text.replace(old, new)
old = 'bool retro_load_game(const struct retro_game_info *game)\n{\n   format_saved_memory();'
new = 'bool retro_load_game(const struct retro_game_info *game)\n{\n   g_real_stop = 0; /* NPS64: previous retro_deinit set this stop latch. */\n   format_saved_memory();'
assert text.count(old) == 1, 'Review upstream content lifecycle'
path.write_text(text.replace(old, new))
text = path.read_text()
old = '    mupencorestop = 1;\n    first_time = 1;\n\n    EmuThreadStep();'
new = '''    g_real_stop = 1;
    frame_break = 0;
    mupencorestop = 1;
    first_time = 1;
    /* NPS64: stop without clearing the CPU stop bit for another frame. */
    stop_device(&g_dev);
    if (emu_initialized) main_run();'''
assert text.count(old) == 1, 'Review upstream content stop path'
path.write_text(text.replace(old, new))
path = root / 'mupen64plus-core/src/main/main.c'
text = path.read_text()
old = '    if (l_setup_done) {\n'
new = '''    if (l_setup_done) {
        extern int g_real_stop;
        if (g_real_stop) {
            /* NPS64: CPU stop is latched; finish interpreter/plugin teardown. */
            frame_break = 0;
            run_device(&g_dev);
            goto nps64_frame_completed;
        }
'''
assert text.count(old) == 1, 'Review upstream emulation re-entry'
text = text.replace(old, new)
old = '    /* Per-frame yield (frame_break set by retro_return at the VI interrupt):'
assert text.count(old) == 1, 'Review upstream frame completion'
text = text.replace(old, 'nps64_frame_completed:\n' + old)
old = 'static size_t l_pak_type_idx[6];'
new = 'static size_t l_pak_type_idx[7]; /* PLUGIN_BIO_PAK == 6. */'
assert text.count(old) == 1, 'Review upstream pak index bounds'
text = text.replace(old, new)
path.write_text(text)
path = root / 'mupen64plus-core/src/device/r4300/r4300_core.c'
text = path.read_text()
old = '    *r4300_stop(r4300) = 0;'
new = '    extern int g_real_stop;\n    *r4300_stop(r4300) = g_real_stop ? 1 : 0; /* NPS64: preserve true stop for teardown. */'
assert text.count(old) == 1, 'Review upstream R4300 re-entry stop flag'
path.write_text(text.replace(old, new))
path = root / 'Makefile.common'
text = path.read_text()
old = 'ifeq ($(STATIC_LINKING),1)\nelse\n'
assert text.count(old) == 2, 'Review upstream standalone common-source selection'
path.write_text(text.replace(old, 'ifeq ($(NPS64_EXTERNAL_COMMON),1)\nelse\n'))
path = root / 'mupen64plus-core/src/device/memory/m64p_memory.c'
text = path.read_text()
old = '    if (posix_memalign(&mem_base, MB_RDRAM_DRAM_ALIGNMENT_REQUIREMENT, MB_MAX_SIZE_FULL) != 0)\n        mem_base = NULL;'
assert text.count(old) == 1, 'Review upstream memory-base allocation'
new = '''#if defined(__ORBIS__) || defined(NPS64_COMPRESSED_MEMORY)
    /* NPS64 interpreter uses compressed addressing; avoid a 512 MiB allocation. */
    mem_base = NULL;
#else
''' + old + '\n#endif'
path.write_text(text.replace(old, new))

# Frontend verifies that context_reset initialized a usable compute renderer.
path = root / 'mupen64plus-video-paraLLEl/rdp.cpp'
text = path.read_text()
path.write_text(text + '\nextern "C" int nps64_parallel_ready(void) { return RDP::device_supported; }\n')

# PS4 statically links RADV; no ELF dlopen Vulkan loader exists on the console.
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/vulkan/context.cpp'
text = path.read_text()
old = '#ifndef _WIN32\n\t\tstatic void *module;'
assert text.count(old) == 1
text = text.replace(old, '#if defined(__ORBIS__)\n\t\treturn false; /* Frontend supplies the static RADV entry point. */\n#elif !defined(_WIN32)\n\t\tstatic void *module;')
path.write_text(text)
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/volk/volk.c'
text = path.read_text()
old = '\tvoid* module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);'
assert text.count(old) == 1
text = text.replace(old, '#ifdef __ORBIS__\n\treturn VK_ERROR_INITIALIZATION_FAILED;\n#else\n' + old)
old = '\tvkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");'
pos = text.rfind(old)
assert pos >= 0
text = text[:pos] + text[pos:].replace(old, old + '\n#endif', 1)
path.write_text(text)

path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/util/logging.cpp'
text = path.read_text()
text = text.replace('#include "logging.hpp"', '#include "logging.hpp"\n#ifdef __ORBIS__\nextern "C" void nps64_vlog(const char *, va_list);\n#endif')
old = '\tif (!logging_iface)\n\t\treturn false;'
assert text.count(old) == 1
new = '#ifdef __ORBIS__\n\tif (!logging_iface) { va_list args; va_start(args,fmt); nps64_vlog(fmt,args); va_end(args); return true; }\n#else\n' + old + '\n#endif'
path.write_text(text.replace(old,new))

# Opt-in investigation: distinguish span setup completion from tile binning.
# Do not enable by default; extra barriers reduce overlap and are not a GPU fix.
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.hpp'
text = path.read_text()
old = '\t\tbool force_sync = false;'
assert text.count(old) == 1, 'Review RDP diagnostic capability storage'
path.write_text(text.replace(old, old + '\n\t\tbool serialize_setup = false;'))
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.cpp'
text = path.read_text()
old = '\tbool allow_subgroup = true;'
assert text.count(old) == 1, 'Review RDP diagnostic initialization'
text = text.replace(old, '''\tif (const char *serial = getenv("NPS64_RDP_SERIALIZE_SETUP"))
\t{
\t\tcaps.serialize_setup = strtol(serial, nullptr, 0) > 0;
\t\tLOGI("NPS64 diagnostic: serialize span/tile setup = %d\\n", int(caps.serialize_setup));
\t}

''' + old)
for upscale in ('false', 'true'):
    old = f'\tsubmit_span_setup_jobs(cmd, {upscale});'
    assert text.count(old) == 1, 'Review span setup dispatch location'
    text = text.replace(old, old + '''
\tif (caps.serialize_setup)
\t\tcmd.barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
\t\t            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
\t\t            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);''')
path.write_text(text)

#!/usr/bin/env python3
"""Bound 2x domain maintenance and retain texture-only batches across FB changes."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp'
(d/'nps64_rdp_update_window.h').write_bytes((project/'src/rdp_update_window.h').read_bytes())
p=d/'rdp_renderer.hpp';s=p.read_text()
if 'nps64_first_active_row' not in s:
 anchor='\t\tunsigned max_shaded_tiles = 0;';assert s.count(anchor)==1
 p.write_text(s.replace(anchor,anchor+'\n        unsigned nps64_first_active_row = Limits::MaxHeight;'))
p=d/'rdp_renderer.cpp';s=p.read_text()
if '[rdp-active-rows]' not in s:
 s='#include "nps64_rdp_update_window.h"\n#include <atomic>\n'+s
 anchor='namespace RDP\n{';assert s.count(anchor)==1
 s=s.replace(anchor,'''namespace {
std::atomic<unsigned long long> nps64_update_pixels{0}, nps64_update_full_pixels{0}, nps64_texture_batches_retained{0};
bool nps64_active_rows_enabled()
{
    // Reference path for local image equivalence tests. No per-frame getenv.
    static const bool enabled = getenv("NPS64_RDP_REFERENCE_BATCHES") == nullptr;
    return enabled;
}
}
extern "C" void nps64_parallel_active_rows_log(void)
{
    const auto full=nps64_update_full_pixels.exchange(0,std::memory_order_relaxed);
    const auto actual=nps64_update_pixels.exchange(0,std::memory_order_relaxed);
    const auto retained=nps64_texture_batches_retained.exchange(0,std::memory_order_relaxed);
    LOGI("[rdp-active-rows] full_pixels=%llu processed_pixels=%llu retained_texture_batches=%llu\\n",full,actual,retained);
}

'''+anchor)
 # TMEM uploads do not depend on the currently bound framebuffer. If no
 # primitive was queued, a framebuffer change need not execute those uploads.
 # Preserve their ordering and instance indices until the next real batch.
 for name,condition,reason in [
  ('set_color_framebuffer','fb.addr != addr || fb.width != width || fb.fmt != fmt',2),
  ('set_depth_framebuffer','fb.depth_addr != addr',3)]:
  old=f'\tif ({condition})\n\t\tflush_queues({reason});'
  assert s.count(old)==1,(name,old)
  new=f'''\tif ({condition})
    {{
        if (!nps64_active_rows_enabled() || !stream.triangle_setup.empty())
            flush_queues({reason});
        else if (!stream.tmem_upload_infos.empty())
            nps64_texture_batches_retained.fetch_add(1,std::memory_order_relaxed);
    }}'''
  s=s.replace(old,new)
 old='''\tsubmit_update_upscaled_domain(cmd, stage, fb.addr, fb.depth_addr,
\t                              fb.width, fb.deduced_height, pixel_size_log2);'''
 assert s.count(old)==1
 s=s.replace(old,'''    unsigned first_row = 0;
    if (nps64_active_rows_enabled() && !caps.super_sample_readback &&
        stage != ResolveStage::SSAAResolve && !stream.triangle_setup.empty())
    {
        first_row = stream.nps64_first_active_row;
    }
    const auto window=nps64_rdp_update_window(fb.addr,fb.depth_addr,fb.width,
        fb.deduced_height,first_row,pixel_size_log2);
    nps64_update_full_pixels.fetch_add(uint64_t(fb.width)*fb.deduced_height,std::memory_order_relaxed);
    nps64_update_pixels.fetch_add(uint64_t(fb.width)*window.height,std::memory_order_relaxed);
    submit_update_upscaled_domain(cmd,stage,window.color,window.depth,
        fb.width,window.height,pixel_size_log2);''')
 anchor='\tstream.max_shaded_tiles = 0;';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    stream.nps64_first_active_row = Limits::MaxHeight;')
 anchor='\t// Need to poke into next scanline validation for certain workarounds.';assert s.count(anchor)==1
 s=s.replace(anchor,'''    // Accumulate the conservative lower Y bound while span jobs are built;
    // maintenance passes then reuse it without rescanning every triangle.
    stream.nps64_first_active_row = std::min(stream.nps64_first_active_row,
        unsigned(std::max(0,min_active_line-1)));

'''+anchor)
 p.write_text(s)

vk=root/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan'
p=vk/'command_buffer.hpp';s=p.read_text()
if 'void dispatch_base(' not in s:
 anchor='\tvoid dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z);';assert s.count(anchor)==1
 p.write_text(s.replace(anchor,anchor+'\n\tvoid dispatch_base(uint32_t base_x, uint32_t base_y, uint32_t base_z, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z);'))

p=vk/'command_buffer.cpp';s=p.read_text()
if 'void CommandBuffer::dispatch_base(' not in s:
 anchor='\tif (mode == CompileMode::FailOnCompileRequired)\n\t\tinfo.flags |= VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT;'
 assert s.count(anchor)==1
 s=s.replace(anchor,'    info.flags |= VK_PIPELINE_CREATE_DISPATCH_BASE;\n'+anchor)
 anchor='void CommandBuffer::dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z)\n{'
 assert s.count(anchor)==1
 helper='''void CommandBuffer::dispatch_base(uint32_t base_x, uint32_t base_y, uint32_t base_z,
                                  uint32_t groups_x, uint32_t groups_y, uint32_t groups_z)
{
\tif (!groups_x || !groups_y || !groups_z)
\t\treturn;
\tif (!(base_x | base_y | base_z))
\t{
\t\tdispatch(groups_x, groups_y, groups_z);
\t\treturn;
\t}
\tVK_ASSERT(is_compute);
\tif (flush_compute_state(true))
\t{
\t\tset_backtrace_checkpoint();
\t\ttable.vkCmdDispatchBase(cmd, base_x, base_y, base_z, groups_x, groups_y, groups_z);
\t}
\telse
\t\tLOGE("Failed to flush render state, dispatch will be dropped.\\n");
}

'''
 p.write_text(s.replace(anchor,helper+anchor))

p=d/'rdp_renderer.hpp';s=p.read_text()
if 'nps64_first_active_col' not in s:
 anchor='        unsigned nps64_first_active_row = Limits::MaxHeight;';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n        unsigned nps64_first_active_col = Limits::MaxWidth;\n        unsigned nps64_end_active_col = 0;')
 p.write_text(s)

p=d/'rdp_renderer.cpp';s=p.read_text()
if 'nps64_active_tiles_enabled' not in s:
 anchor='std::atomic<unsigned long long> nps64_update_pixels{0}, nps64_update_full_pixels{0}, nps64_texture_batches_retained{0};'
 assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
std::atomic<unsigned long long> nps64_full_blend_tiles{0}, nps64_active_blend_tiles{0}, nps64_full_bin_groups{0}, nps64_active_bin_groups{0};
bool nps64_active_tiles_enabled()
{
    static const bool enabled = getenv("NPS64_RDP_REFERENCE_BATCHES") == nullptr &&
                                getenv("NPS64_RDP_REFERENCE_TILES") == nullptr;
    return enabled;
}''')
 old_log='''    const auto full=nps64_update_full_pixels.exchange(0,std::memory_order_relaxed);
    const auto actual=nps64_update_pixels.exchange(0,std::memory_order_relaxed);
    const auto retained=nps64_texture_batches_retained.exchange(0,std::memory_order_relaxed);
    LOGI("[rdp-active-rows] full_pixels=%llu processed_pixels=%llu retained_texture_batches=%llu\\n",full,actual,retained);'''
 assert s.count(old_log)==1
 s=s.replace(old_log,'''    const auto full=nps64_update_full_pixels.exchange(0,std::memory_order_relaxed);
    const auto actual=nps64_update_pixels.exchange(0,std::memory_order_relaxed);
    const auto retained=nps64_texture_batches_retained.exchange(0,std::memory_order_relaxed);
    const auto fblend=nps64_full_blend_tiles.exchange(0,std::memory_order_relaxed);
    const auto ablend=nps64_active_blend_tiles.exchange(0,std::memory_order_relaxed);
    const auto fbin=nps64_full_bin_groups.exchange(0,std::memory_order_relaxed);
    const auto abin=nps64_active_bin_groups.exchange(0,std::memory_order_relaxed);
    LOGI("[rdp-active-rows] full_pixels=%llu processed_pixels=%llu retained_texture_batches=%llu full_blend_tiles=%llu active_blend_tiles=%llu full_bin_groups=%llu active_bin_groups=%llu\\n",full,actual,retained,fblend,ablend,fbin,abin);''')
 anchor='\tif (stream.depth_blend_state.flags & DEPTH_BLEND_DEPTH_UPDATE_BIT)\n\t\tfb.depth_write_pending = true;'
 assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
    nps64_rdp_accumulate_x_bounds(setup.xh, setup.xm, setup.xl, setup.yh, setup.ym, setup.yl,
        setup.dxhdy, setup.dxmdy, setup.dxldy, setup.flags,
        stream.scissor_state.xlo, stream.scissor_state.ylo,
        stream.scissor_state.xhi, stream.scissor_state.yhi,
        caps.upscaling, &stream.nps64_first_active_col, &stream.nps64_end_active_col);''')
 old_bin='\tcmd.dispatch(caps.direct_binning ? 1 : num_primitives_32, num_meta_tiles_x, num_meta_tiles_y);'
 assert s.count(old_bin)==1
 s=s.replace(old_bin,'''    const uint32_t prim_groups = caps.direct_binning ? 1u : num_primitives_32;
    const auto tile_win = (nps64_active_tiles_enabled() && !caps.super_sample_readback && !caps.direct_binning) ?
        nps64_rdp_tile_window(push.width, push.height,
            stream.nps64_first_active_col, stream.nps64_end_active_col,
            stream.nps64_first_active_row, upscale ? caps.upscaling : 1u,
            meta_tiles_x, meta_tiles_y) :
        Nps64RdpTileWindow{0, num_tiles_x, 0, num_tiles_y, 0, num_meta_tiles_x, 0, num_meta_tiles_y};
    nps64_full_bin_groups.fetch_add(uint64_t(prim_groups) * num_meta_tiles_x * num_meta_tiles_y, std::memory_order_relaxed);
    nps64_active_bin_groups.fetch_add(uint64_t(prim_groups) * tile_win.count_meta_x * tile_win.count_meta_y, std::memory_order_relaxed);
    cmd.dispatch_base(0, tile_win.first_meta_x, tile_win.first_meta_y,
                      prim_groups, tile_win.count_meta_x, tile_win.count_meta_y);''')
 old_blend='\tcmd.dispatch((push.fb_width + 7) / 8, (push.fb_height + 7) / 8, 1);'
 assert s.count(old_blend)==1
 s=s.replace(old_blend,'''    const uint32_t total_tiles_x = (push.fb_width + 7) / 8;
    const uint32_t total_tiles_y = (push.fb_height + 7) / 8;
    const auto tile_win = (nps64_active_tiles_enabled() && !caps.super_sample_readback && !force_write_mask && !debug_channel) ?
        nps64_rdp_tile_window(push.fb_width, push.fb_height,
            stream.nps64_first_active_col, stream.nps64_end_active_col,
            stream.nps64_first_active_row, upscaled ? caps.upscaling : 1u, 8u, 4u) :
        Nps64RdpTileWindow{0, total_tiles_x, 0, total_tiles_y, 0, 0, 0, 0};
    nps64_full_blend_tiles.fetch_add(uint64_t(total_tiles_x) * total_tiles_y, std::memory_order_relaxed);
    nps64_active_blend_tiles.fetch_add(uint64_t(tile_win.count_tile_x) * tile_win.count_tile_y, std::memory_order_relaxed);
    cmd.dispatch_base(tile_win.first_tile_x, tile_win.first_tile_y, 0,
                      tile_win.count_tile_x, tile_win.count_tile_y, 1);''')
 anchor='    stream.nps64_first_active_row = Limits::MaxHeight;';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    stream.nps64_first_active_col = Limits::MaxWidth;\n    stream.nps64_end_active_col = 0;')
 p.write_text(s)

p=d/'rdp_device.cpp';s=p.read_text()
if 'nps64_fast_masked_memcpy' not in s:
 old_mm='''\tfor (size_t i = 0; i < size; i += 16)
\t{
\t\t__m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(data_src + i));
\t\t__m128i mask = _mm_loadu_si128(reinterpret_cast<const __m128i *>(masked_src + i));
\t\t_mm_maskmoveu_si128(data, mask, reinterpret_cast<char *>(dst + i));
\t}'''
 assert s.count(old_mm)==1
 s=s.replace(old_mm,'''    // nps64_fast_masked_memcpy: skip zero masks and use cached 128-bit stores for full masks.
\tfor (size_t i = 0; i < size; i += 16)
\t{
\t\t__m128i mask = _mm_loadu_si128(reinterpret_cast<const __m128i *>(masked_src + i));
\t\tint bits = _mm_movemask_epi8(mask);
\t\tif (!bits)
\t\t\tcontinue;
\t\t__m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(data_src + i));
\t\tif (bits == 0xffff)
\t\t\t_mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), data);
\t\telse
\t\t\t_mm_maskmoveu_si128(data, mask, reinterpret_cast<char *>(dst + i));
\t}''')
 p.write_text(s)

print('Active 16-bit framebuffer rows, 2D tile dispatches and coherency deduplication prepared in',root)

#!/usr/bin/env python3
"""Add a paused, state-preserving 1x/2x switch to a prepared Vulkan core copy."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve()
if root==Path(__file__).resolve().parents[1]/'vendor/parallel-n64':raise SystemExit('Use a build copy')
base=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp'
def replace(path,old,new,marker):
 s=path.read_text()
 if marker in s:return
 assert s.count(old)==1,(path,old)
 path.write_text(s.replace(old,new))
p=base/'rdp_renderer.hpp'
replace(p,'\tbool init_renderer(const RendererOptions &options);','\tbool init_renderer(const RendererOptions &options);\n\tbool nps64_set_live_upscaling(unsigned factor);','bool nps64_set_live_upscaling')
replace(p,'\tunsigned get_scaling_factor() const;','\tunsigned get_scaling_factor() const;\n\tunsigned nps64_scratch_scale = 1;','unsigned nps64_scratch_scale')
p=base/'rdp_renderer.cpp'
replace(p,'\tinit_buffers(options);','\tinit_buffers(options);\n\tnps64_scratch_scale = options.upscaling_factor;','nps64_scratch_scale = options.upscaling_factor')
method=r'''
/* NPS64 live resolution: caller has drained commands/timeline. Keep TMEM,
 * tiles, framebuffer state, constants and canonical RDRAM/hidden RAM intact. */
bool Renderer::nps64_set_live_upscaling(unsigned factor)
{
    if ((factor != 1 && factor != 2) || caps.super_sample_readback || caps.upscaling > 2)
        return false;
    if (factor == caps.upscaling) return true;
    const auto original_caps = caps;
    resolve_coherency_external(0, unsigned(rdram_size));
    device->wait_idle();

    // Allocate into local handles; failure leaves all active resources intact.
    Vulkan::BufferHandle scratch[9], domain[3];
    bool grow = factor > nps64_scratch_scale;
    Vulkan::BufferCreateInfo info = {};
    info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.domain = Vulkan::BufferDomain::Device;
    info.misc = Vulkan::BUFFER_MISC_ZERO_INITIALIZE_BIT;
    if (grow)
    {
        const size_t max_w = factor * Limits::MaxWidth, max_h = factor * Limits::MaxHeight;
        const size_t instances = factor * factor * Limits::MaxTileInstances;
        const size_t tiles = (max_w / ImplementationConstants::TileWidth) * (max_h / ImplementationConstants::TileHeight);
        const size_t pixels = instances * ImplementationConstants::TileWidth * ImplementationConstants::TileHeight;
        const size_t sizes[9] = {
            sizeof(uint32_t) * (Limits::MaxPrimitives / 32) * tiles,
            sizeof(uint32_t) * tiles,
            sizeof(uint32_t) * (Limits::MaxPrimitives / 32) * tiles,
            sizeof(TileRasterWork) * Limits::MaxStaticRasterizationStates * instances,
            sizeof(uint32_t) * pixels, sizeof(uint32_t) * pixels,
            sizeof(uint8_t) * pixels, sizeof(uint8_t) * pixels,
            factor * Limits::MaxSpanSetups * sizeof(SpanSetup)
        };
        for (unsigned i = 0; i < 9; i++) {
            info.size = sizes[i];scratch[i] = device->create_buffer(info);
            if (!scratch[i]) return false;
        }
    }
    bool allocate_domain = factor == 2 && !upscaling_reference_rdram;
    if (allocate_domain) {
        const size_t sizes[3] = {rdram_size, rdram_size * 4, hidden_rdram->get_create_info().size * 4};
        for (unsigned i = 0; i < 3; i++) {
            info.size = sizes[i];domain[i] = device->create_buffer(info);
            if (!domain[i]) return false;
        }
    }
    if (grow) {
        tile_binning_buffer = std::move(scratch[0]);tile_binning_buffer_coarse = std::move(scratch[1]);
        per_tile_offsets = std::move(scratch[2]);tile_work_list = std::move(scratch[3]);
        per_tile_shaded_color = std::move(scratch[4]);per_tile_shaded_depth = std::move(scratch[5]);
        per_tile_shaded_coverage = std::move(scratch[6]);per_tile_shaded_shaded_alpha = std::move(scratch[7]);
        span_setups = std::move(scratch[8]);nps64_scratch_scale = factor;
    }
    if (allocate_domain) {
        upscaling_reference_rdram = std::move(domain[0]);
        upscaling_multisampled_rdram = std::move(domain[1]);
        upscaling_multisampled_hidden_rdram = std::move(domain[2]);
    }
    if (factor == 2) {
        // Seed every sample plane from canonical memory, including FF/zero
        // colours, depth and hidden coverage. No sentinel/format assumptions.
        auto cmd = device->request_command_buffer();
        cmd->barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_WRITE_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
        cmd->copy_buffer(*upscaling_reference_rdram, 0, *rdram, rdram_offset, rdram_size);
        for (unsigned sample = 0; sample < 4; sample++) {
            cmd->copy_buffer(*upscaling_multisampled_rdram, sample * rdram_size, *rdram, rdram_offset, rdram_size);
            size_t hidden_size = hidden_rdram->get_create_info().size;
            cmd->copy_buffer(*upscaling_multisampled_hidden_rdram, sample * hidden_size, *hidden_rdram, 0, hidden_size);
        }
        cmd->barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
        Vulkan::Fence fence;device->submit(cmd, &fence);fence->wait();
    }
    caps.upscaling = factor;
    caps.max_width = factor * Limits::MaxWidth;caps.max_height = factor * Limits::MaxHeight;
    caps.max_tiles_x = factor * ImplementationConstants::MaxTilesX;
    caps.max_tiles_y = factor * ImplementationConstants::MaxTilesY;
    caps.max_num_tile_instances = factor * factor * Limits::MaxTileInstances;
    // Keep the maximum scratch allocation for later switches. Native mode
    // dispatches/rasterizes at 1x; it does not render at 2x and downsample.
    if (shader_bank) set_shader_bank(shader_bank);
    if (!original_caps.force_sync && caps.force_sync) { caps = original_caps; return false; }
    return true;
}
'''
s=p.read_text()
if '/* NPS64 live resolution: caller' in s:
 a=s.index('/* NPS64 live resolution: caller');b=s.index('void Renderer::set_device(Vulkan::Device *device_)',a)
 p.write_text(s[:a]+method.lstrip()+'\n'+s[b:])
else:
 replace(p,'void Renderer::set_device(Vulkan::Device *device_)',method+'\nvoid Renderer::set_device(Vulkan::Device *device_)','NPS64 live resolution: caller')
p=base/'rdp_device.hpp'
replace(p,'\tbool device_is_supported() const;','\tbool device_is_supported() const;\n\tbool nps64_set_live_upscaling(unsigned factor);','bool nps64_set_live_upscaling')
p=base/'rdp_device.cpp'
method=r'''
bool CommandProcessor::nps64_set_live_upscaling(unsigned factor)
{
    if (!is_supported || (factor != 1 && factor != 2)) return false;
    idle();device.wait_idle();
    if (!renderer.nps64_set_live_upscaling(factor)) return false;
    flags &= ~(COMMAND_PROCESSOR_FLAG_UPSCALING_2X_BIT | COMMAND_PROCESSOR_FLAG_UPSCALING_4X_BIT | COMMAND_PROCESSOR_FLAG_UPSCALING_8X_BIT);
    if (factor == 2) flags |= COMMAND_PROCESSOR_FLAG_UPSCALING_2X_BIT;
    return true;
}
'''
replace(p,'bool CommandProcessor::device_is_supported() const',method+'\nbool CommandProcessor::device_is_supported() const','bool CommandProcessor::nps64_set_live_upscaling')
p=root/'mupen64plus-video-paraLLEl/rdp.cpp'
method=r'''
extern "C" int nps64_parallel_resolution(unsigned factor)
{
    if (!device_supported || !frontend || !device || cmd_cur != cmd_ptr) return 0;
    if (!frontend->nps64_set_live_upscaling(factor)) return 0;
    upscaling = factor;
    log_cb(RETRO_LOG_INFO, "[resolution-live] factor=%u core_and_rdp_state_preserved=1\n", factor);
    return 1;
}
'''
replace(p,'void deinit()\n{',method+'\nvoid deinit()\n{','int nps64_parallel_resolution')
print('Live 1x/2x Vulkan resolution prepared in',root)

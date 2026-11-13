#!/usr/bin/env python3
"""Instrument an existing build copy for opt-in, CPU-side pipeline export.

NPS64_PIPELINE_CAPTURE must name an existing directory. No Vulkan calls or
emulated state are changed. Do not apply to the immutable upstream checkout.
"""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve()
project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy, not vendor')
shader=root/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan/shader.cpp'
s=shader.read_text()
marker='NPS64 CPU-side SPIR-V capture'
if marker not in s:
    needle='\tinfo.pCode = data;'
    assert s.count(needle)==1
    s='#include <cstdio>\n#include <cstdlib>\n'+s
    s=s.replace(needle,needle+'''
    // NPS64 CPU-side SPIR-V capture. Diagnostics only, before device work.
    if (const char *directory = getenv("NPS64_PIPELINE_CAPTURE"))
    {
        char path[1024];
        int length = snprintf(path, sizeof(path), "%s/shader-%016llx.spv", directory,
                              static_cast<unsigned long long>(hash));
        if (length > 0 && size_t(length) < sizeof(path) && size <= 16 * 1024 * 1024)
        {
            if (FILE *file = fopen(path, "wb"))
            {
                size_t written = fwrite(data, 1, size, file);
                fclose(file);
                if (written != size) LOGE("Incomplete SPIR-V capture.\\n");
            }
        }
    }
''')
    shader.write_text(s)
command=root/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan/command_buffer.cpp'
s=command.read_text()
marker='NPS64 CPU-side pipeline capture'
if marker not in s:
    start=s.index('CommandBuffer::build_compute_pipeline(')
    needle='\tauto start_ts = Util::get_current_time_nsecs();'
    pos=s.index(needle,start)
    insertion=r'''
    // NPS64 CPU-side pipeline capture. Serialized layouts and specialization
    // constants permit compiler-only inspection on a virtual GFX7 device.
    if (const char *directory = getenv("NPS64_PIPELINE_CAPTURE"))
    {
        char path[1024];
        int length = snprintf(path, sizeof(path), "%s/pipeline-%016llx.json", directory,
                              static_cast<unsigned long long>(compile.hash));
        if (length > 0 && size_t(length) < sizeof(path))
        {
            if (FILE *file = fopen(path, "w"))
            {
                const auto &layout = compile.program->get_pipeline_layout()->get_resource_layout();
                fprintf(file, "{\"pipeline\":\"%016llx\",\"shader\":\"%016llx\",\"flags\":%u,\"push\":[%u,%u,%u],\"bindings\":[",
                        static_cast<unsigned long long>(compile.hash),
                        static_cast<unsigned long long>(shader.get_hash()), unsigned(info.stage.flags),
                        layout.push_constant_range.offset, layout.push_constant_range.size,
                        unsigned(layout.push_constant_range.stageFlags));
                bool first = true;
                for (unsigned set = 0; set < VULKAN_NUM_DESCRIPTOR_SETS; set++)
                {
                    const auto &desc = layout.sets[set];
                    uint32_t masks[] = {desc.sampled_image_mask, desc.sampled_texel_buffer_mask,
                        desc.storage_texel_buffer_mask, desc.storage_image_mask, desc.uniform_buffer_mask,
                        desc.storage_buffer_mask, desc.input_attachment_mask, desc.separate_image_mask, desc.sampler_mask};
                    VkDescriptorType types[] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLER};
                    for (unsigned kind = 0; kind < 9; kind++)
                    for (unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
                    {
                        if (!(masks[kind] & (1u << binding))) continue;
                        if (!layout.stages_for_bindings[set][binding]) continue;
                        fprintf(file, "%s[%u,%u,%u,%u,%u]", first ? "" : ",", set, binding,
                                unsigned(types[kind]), unsigned(desc.array_size[binding]),
                                layout.stages_for_bindings[set][binding]);
                        first = false;
                    }
                }
                fprintf(file, "],\"specialization\":[");
                for (unsigned i = 0; i < spec_info.mapEntryCount; i++)
                    fprintf(file, "%s[%u,%u]", i ? "," : "", spec_entries[i].constantID, spec_constants[i]);
                fprintf(file, "]}\n");
                fclose(file);
            }
        }
    }
'''
    # The C++ strings need ordinary escaped JSON quotes, not double escapes.
    insertion=insertion.replace('\\\"','\\"').replace('\\n','\\n')
    s=s[:pos]+insertion+s[pos:]
    s='#include <cstdio>\n#include <cstdlib>\n'+s
    command.write_text(s)
print('Pipeline export instrumentation ready in '+str(root))

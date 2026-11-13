#!/usr/bin/env python3
"""Add opt-in checkpoints to a prepared paraLLEl build copy, never vendor."""
from pathlib import Path
import sys

project = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
if root == project / 'vendor/parallel-n64':
    raise SystemExit('Use a build copy, not vendor')
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/vulkan/command_buffer.cpp'
text = path.read_text()
marker = '// NPS64 opt-in command checkpoints'
if marker not in text:
    def replace(old, new):
        global text
        assert text.count(old) == 1, 'Review checkpoint anchor: ' + old
        text = text.replace(old, new)

    replace('\tVkResult vr = table.vkCreateComputePipelines(device->get_device(), compile.cache, 1, &info, nullptr, &compute_pipeline);',
            '''    if (getenv("NPS64_GPU_API_TRACE"))
        LOGI("[rdp-checkpoint] compile begin hash=%016llx shader=%016llx\\n",
             static_cast<unsigned long long>(compile.hash), static_cast<unsigned long long>(shader.get_hash()));
\tVkResult vr = table.vkCreateComputePipelines(device->get_device(), compile.cache, 1, &info, nullptr, &compute_pipeline);
    if (getenv("NPS64_GPU_API_TRACE"))
        LOGI("[rdp-checkpoint] compile end hash=%016llx result=%d\\n", static_cast<unsigned long long>(compile.hash), int(vr));''')
    for method, args, details, values in [
        ('dispatch_indirect', 'const Buffer &buffer, uint32_t offset', 'offset=%u buffer_bytes=%llu',
         'offset, static_cast<unsigned long long>(buffer.get_create_info().size)'),
        ('dispatch', 'uint32_t groups_x, uint32_t groups_y, uint32_t groups_z', 'groups=%u,%u,%u',
         'groups_x, groups_y, groups_z'),
    ]:
        old = f'void CommandBuffer::{method}({args})\n{{\n\tVK_ASSERT(is_compute);'
        replace(old, old + f'''
    if (getenv("NPS64_GPU_API_TRACE"))
        LOGI("[rdp-checkpoint] {method} flush begin command=%p {details}\\n", (void *)cmd, {values});''')
        old = ('\t\ttable.vkCmdDispatchIndirect(cmd, buffer.get_buffer(), offset);' if method == 'dispatch_indirect'
               else '\t\ttable.vkCmdDispatch(cmd, groups_x, groups_y, groups_z);')
        replace(old, f'''        if (getenv("NPS64_GPU_API_TRACE"))
            LOGI("[rdp-checkpoint] {method} flush end command=%p\\n", (void *)cmd);
''' + old + f'''
        if (getenv("NPS64_GPU_API_TRACE"))
            LOGI("[rdp-checkpoint] {method} recorded command=%p\\n", (void *)cmd);''')
    text = '#include <cstdlib>\n' + marker + '\n' + text
    path.write_text(text)
# Keep one entry/return pair per pipeline even in compact mode. This avoids
# losing the failing shader hash while removing per-dispatch/submit traffic.
updated = text.replace('if (getenv("NPS64_GPU_API_TRACE"))\n        LOGI("[rdp-checkpoint] compile begin',
                       'LOGI("[pipeline-compile] begin')
updated = updated.replace('if (getenv("NPS64_GPU_API_TRACE"))\n        LOGI("[rdp-checkpoint] compile end',
                          'LOGI("[pipeline-compile] end')
assert '[pipeline-compile] begin' in updated and '[pipeline-compile] end' in updated
if updated != text:
    path.write_text(updated)
if 'nps64_log_compile' not in updated:
    updated=updated.replace('LOGI("[pipeline-compile] begin',
        'bool nps64_log_compile = mode != CompileMode::FailOnCompileRequired || getenv("NPS64_GPU_API_TRACE");\n    if (nps64_log_compile) LOGI("[pipeline-compile] begin',1)
    updated=updated.replace('LOGI("[pipeline-compile] end hash=%016llx result=%d\\n", static_cast<unsigned long long>(compile.hash), int(vr));',
        'if (nps64_log_compile) LOGI("[pipeline-compile] end hash=%016llx result=%d elapsed_us=%.3f\\n", static_cast<unsigned long long>(compile.hash), int(vr), double(Util::get_current_time_nsecs()-start_ts)/1000.0);',1)
    assert 'elapsed_us=' in updated
    path.write_text(updated)
print('Command checkpoints ready in ' + str(root))

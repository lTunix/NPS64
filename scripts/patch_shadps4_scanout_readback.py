"""Diagnostic: synchronize the actual VI readback buffer to guest memory.

The CPU reads a host-visible Vulkan allocation after its fence. shadPS4 owns
a separate host GPU buffer, so fence completion alone cannot update that map.
No pixels are generated or replaced by this patch.
"""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
path = parser.parse_args().source / 'src/video_core/renderer_vulkan/vk_rasterizer.cpp'
text = path.read_text()
if '#include <fstream>' not in text:
    anchor = '#include "common/debug.h"'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '#include <fstream>\n' + anchor)
    path.write_text(text)
marker = '    // NPS64 VI readback: publish actual completed GPU bytes to guest host map.\n'
if marker not in text:
    anchor = '    DebugState.IncDispatch();\n'
    assert text.count(anchor) == 2
    block = '''    // NPS64 VI readback: publish actual completed GPU bytes to guest host map.
    if (cs.pgm_hash == 0xf565b20ef4baf1c7ULL) {
        scheduler.Finish();
        for (const auto& desc : cs.buffers) {
            if (desc.IsSpecial() || !desc.is_written) {
                continue;
            }
            const auto sharp = desc.GetSharp(cs);
            if (sharp.base_address && sharp.GetSize()) {
                buffer_cache.ReadMemory(sharp.base_address, sharp.GetSize(), false, true);
            }
        }
    }
'''
    text = text.replace(anchor, block + anchor, 1)
    path.write_text(text)
print('VI readback downloads the real completed GPU allocation; diagnostic shader hash only')


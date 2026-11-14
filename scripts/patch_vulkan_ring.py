#!/usr/bin/env python3
"""Keep FIFO/eventcount ordering while eliminating per-word circular copies."""
from pathlib import Path
import sys
project=Path(__file__).resolve().parents[1];root=Path(sys.argv[1]).resolve()
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy, not vendor')
p=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/command_ring.cpp';s=p.read_text()
if 'NPS64 contiguous ring copies' not in s:
 s='#include <algorithm>\n#include <cstring>\n// NPS64 contiguous ring copies; preserve publication and completion ordering.\n'+s
 old='''\tring[w++ & mask] = num_words;
\tfor (unsigned i = 0; i < num_words; i++)
\t\tring[w++ & mask] = words[i];''';assert s.count(old)==1
 s=s.replace(old,'''\tring[w++ & mask] = num_words;
    if (num_words) {
        size_t index = w & mask;
        size_t first = std::min<size_t>(num_words, size - index);
        std::memcpy(ring.data() + index, words, first * sizeof(uint32_t));
        if (first < num_words)std::memcpy(ring.data(), words + first, (num_words - first) * sizeof(uint32_t));
        w += num_words;
    }''')
 s=s.replace('std::vector<uint32_t> tmp_buffer;\n\ttmp_buffer.reserve(64);','std::vector<uint32_t> tmp_buffer(64);\n    unsigned num_words = 0;')
 old='''\t\t\tuint32_t num_words = ring[r++ & mask];
\t\t\ttmp_buffer.resize(num_words);
\t\t\tfor (uint32_t i = 0; i < num_words; i++)
\t\t\t\ttmp_buffer[i] = ring[r++ & mask];''';assert s.count(old)==1
 s=s.replace(old,'''\t\t\tnum_words = ring[r++ & mask];
            if (num_words > tmp_buffer.size())tmp_buffer.resize(num_words);
            if (num_words) {
                size_t index = r & mask;
                size_t first = std::min<size_t>(num_words, ring.size() - index);
                std::memcpy(tmp_buffer.data(), ring.data() + index, first * sizeof(uint32_t));
                if (first < num_words)std::memcpy(tmp_buffer.data() + first, ring.data(), (num_words - first) * sizeof(uint32_t));
                r += num_words;
            }''')
 s=s.replace('tmp_buffer.resize(1);','num_words = 1;').replace('if (tmp_buffer.empty())','if (!num_words)').replace('processor->enqueue_command_direct(tmp_buffer.size(), tmp_buffer.data());','processor->enqueue_command_direct(num_words, tmp_buffer.data());')
 p.write_text(s)
# The command processor must not compete below background compilation.
# The existing backend hook preserves any priority already above its target.
s=p.read_text()
if 'nps64_cpu_worker_enter' not in s:
 s='extern "C" void nps64_cpu_worker_enter(unsigned) __attribute__((weak));\n'+s
 anchor='\tUtil::register_thread_index(0);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    if (nps64_cpu_worker_enter)nps64_cpu_worker_enter(32);')
 p.write_text(s)
p=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_device.cpp';s=p.read_text()
old='this, 4 * 1024))';new='this, 64 * 1024))'
if new not in s:assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
print('Contiguous command copies, reusable payload and 256 KiB FIFO in '+str(root))

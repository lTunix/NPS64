#!/usr/bin/env python3
"""Avoid memcpy(null, 0) for empty subpass attachments in a build copy."""
from pathlib import Path
import sys

project = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
if root == project / 'vendor/parallel-n64':
    raise SystemExit('Use a build copy, not vendor')
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/vulkan/render_pass.cpp'
text = path.read_text()
marker = '// NPS64: empty attachment arrays may have null pointers.'
if marker not in text:
    for kind in ('color', 'input'):
        title = kind.capitalize()
        old = (f'\t\tmemcpy(subpass_info.{kind}_attachments, subpass.p{title}Attachments,\n'
               f'\t\t       subpass.{kind}AttachmentCount * sizeof(*subpass.p{title}Attachments));')
        assert text.count(old) == 1, 'Review subpass attachment copy: ' + kind
        text = text.replace(old, f'\t\tif (subpass.{kind}AttachmentCount)\n\t' + old.replace('\n', '\n\t'))
    text = marker + '\n' + text
    path.write_text(text)
print('Empty Vulkan attachment copies guarded in ' + str(root))

# Upscaling can produce INT_MIN sentinel coordinates. Widen before abs so
# the conservative overflow/scissor test cannot itself overflow signed int.
path = root / 'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.cpp'
text = path.read_text()
old = 'auto max_range_x = std::max(std::abs(start_x), std::abs(end_x));'
new = '''const int64_t max_range_x = std::max(
        start_x < 0 ? -int64_t(start_x) : int64_t(start_x),
        end_x < 0 ? -int64_t(end_x) : int64_t(end_x));'''
wide = 'auto max_range_x = std::max(std::abs(int64_t(start_x)), std::abs(int64_t(end_x)));'
text = text.replace(wide, old)
if new not in text:
    assert text.count(old) == 1, 'Review raster range overflow guard'
    path.write_text(text.replace(old, new))
print('Raster range abs widened before evaluating INT_MIN in ' + str(root))

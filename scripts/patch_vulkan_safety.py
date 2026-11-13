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

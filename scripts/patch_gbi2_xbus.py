#!/usr/bin/env python3
"""Route GBI 2 xbus tasks to the GBI 2 walker, including Excitebike 64."""
from pathlib import Path
import sys
root = Path(sys.argv[1]).resolve()
project = Path(__file__).resolve().parents[1]
if root == project / 'vendor/parallel-n64':
    raise SystemExit('Use a build copy')
p = root / 'mupen64plus-video-angrylion/rdp_emit_hle.c'
s = p.read_text()
start = s.rindex('static int f3d_ucode_family(')
end = s.index('\n}\n', start) + 3
body = s[start:end]
old = "if (rdram[(j + 0u) ^ 3] == 'f' && rdram[(j + 1u) ^ 3] == 'i' &&\n                    rdram[(j + 2u) ^ 3] == 'f' && rdram[(j + 3u) ^ 3] == 'o')"
new = "if ((rdram[(j + 0u) ^ 3] == 'f' && rdram[(j + 1u) ^ 3] == 'i' &&\n                     rdram[(j + 2u) ^ 3] == 'f' && rdram[(j + 3u) ^ 3] == 'o') ||\n                    (rdram[(j + 0u) ^ 3] == 'x' && rdram[(j + 1u) ^ 3] == 'b' &&\n                     rdram[(j + 2u) ^ 3] == 'u' && rdram[(j + 3u) ^ 3] == 's'))"
if new not in body:
    assert body.count(old) == 1, 'Unexpected GBI 1 family classifier'
    body = body.replace(old, new)
    body = body.replace('the "fifo" token that tags every GBI 2', 'the "fifo" or "xbus" transport token of GBI 2')
    s = s[:start] + body + s[end:]
    p.write_text(s)
print('GBI 2 xbus family classification prepared in', root)

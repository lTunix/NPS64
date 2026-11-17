#!/usr/bin/env python3
"""Skip discarded perspective divides when exact RSP projection succeeds."""
from pathlib import Path
import sys
root = Path(sys.argv[1]).resolve()
project = Path(__file__).resolve().parents[1]
if root == project / 'vendor/parallel-n64':
    raise SystemExit('Use a build copy')
p = root / 'mupen64plus-video-angrylion/rdp_emit_bridge.c'
s = p.read_text()
marker = '/* NPS64: exact RSP coordinates supersede the fallback divide. */'
if marker not in s:
    start = s.index('void bridge_compute_screen(')
    opening = s.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (s[end] == '{') - (s[end] == '}')
        end += 1
    original = s[start:end]
    a = original.index('    /* RSP-exact vertex transform:')
    b = original.index('\n}\n    *out_x', a)
    rsp = original[a:b]
    fast = '    ' + marker + '\n' + rsp + '\n    if (e->rsp_ok)\n    {\n        unsigned int pn = vp->persp_norm ? vp->persp_norm : 0xffffu;\n        *w_raw = v->cw > 0 ? ((int64_t)(int)pn << 41) / v->cw : 0;\n        *out_x = e->x; *out_y = e->y; *out_z = e->z;\n        *out_rsp_ok = e->rsp_ok; *out_rsp_invw = e->rsp_invw;\n        return;\n    }\n'
    new = original[:a] + original[b:]
    anchor = '    EmitVertex *e = &tmp;\n'
    assert new.count(anchor) == 1
    new = new.replace(anchor, anchor + fast)
    reference = original.replace('void bridge_compute_screen(', 'void nps64_reference_bridge_compute_screen(', 1)
    s = s[:start] + new + '\n#ifdef NPS64_SCREEN_ORACLE\n' + reference + '\n#endif\n' + s[end:]
    p.write_text(s)
print('Exact RSP projection fast path prepared in', root)

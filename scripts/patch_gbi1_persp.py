#!/usr/bin/env python3
"""Honor the Fast3D perspective-normalize command retained by F3DEX/LX 0.95."""
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve()
project = Path(__file__).resolve().parents[1]
if root == project / 'vendor/parallel-n64':
    raise SystemExit('Use a build copy')
path = root / 'mupen64plus-video-angrylion/rdp_emit_f3d.c'
source = path.read_text()
marker = '/* NPS64: GBI 1 0.95 retains the Fast3D RDPHALF_1 perspNorm write.'
if marker not in source:
    old = '''            if (!s_variant_d64)
                gsp_set_persp_norm(gsp, w1 & 0xffffu);'''
    new = '''            /* NPS64: GBI 1 0.95 retains the Fast3D RDPHALF_1 perspNorm write.
             * The existing legacy-format flag identifies F3DEX/F3DLX 0.95,
             * including Mario Kart. Its x2 vertex encoding selects d64,
             * but its perspective command is still the old one. Ignoring
             * it leaves the ortho/default 0xffff reciprocal scale active
             * in world geometry, losing depth precision. Texrect tails
             * are consumed inline before reaching this handler. Later
             * GBI 1 builds keep their MOVEWORD convention unchanged. */
            if (!s_variant_d64 || gsp->nps64_culldl_legacy)
                gsp_set_persp_norm(gsp, w1 & 0xffffu);'''
    assert source.count(old) == 1, 'Unexpected F3D perspective handler'
    assert 'nps64_culldl_legacy' in (path.parent / 'rdp_emit_frontend.h').read_text(), 'Apply patch_gsp_culldl.py first'
    path.write_text(source.replace(old, new))
print('GBI 1 0.95 perspective normalization prepared in', root)

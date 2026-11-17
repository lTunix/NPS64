#!/usr/bin/env python3
"""Do not read a preceding branch for a standalone page-spanning delay slot."""
from pathlib import Path
import sys
root = Path(sys.argv[1]).resolve()
project = Path(__file__).resolve().parents[1]
if root == project / 'vendor/parallel-n64':
    raise SystemExit('Use a build copy')
p = root / 'mupen64plus-core/src/device/r4300/new_dynarec/new_dynarec.c'
s = p.read_text()
marker = '/* NPS64: page-spanning delay-slot register liveness. */'
if marker not in s:
    a = s.index('    is_ds[i]=ds;\n    if(ds) {')
    b = s.index('    else { // Not delay slot', a)
    part = s[a:b]
    old = '      if(i+1<slen) {'
    assert part.count(old) == 1 and part.count('if(!ooo[i-1])') == 2
    part = part.replace(old, '''      /* NPS64: page-spanning delay-slot register liveness. */
      if(i==0) {
        /* The branch is in the preceding page/block, not entry -1.
         * Conservatively keep every guest register live except $zero. */
        current.u=1;
        current.uu=1;
      }else if(i+1<slen) {''', 1)
    part = part.replace('if(!ooo[i-1])', 'if(i==0 || !ooo[i-1])')
    p.write_text(s[:a] + part + s[b:])
print('Standalone delay-slot liveness prepared in', root)

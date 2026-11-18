#!/usr/bin/env python3
"""Keep Ari64's MI interpreter window tied to an explicit mode request."""
from pathlib import Path
import sys
root = Path(sys.argv[1])
path = root / 'mupen64plus-core/src/device/r4300/new_dynarec/new_dynarec.c'
text = path.read_text()
old = '''      && (r4300->mi->regs[MI_INIT_MODE_REG] & 0x180) && !r4300->delay_slot'''
new = '''      /* A window abandoned at a branch leaves init/EBUS bits visible to
       * compiled code. An unrelated MI write (e.g. clear DP interrupt) must
       * not revive that abandoned window: CIC-6103's final IPL3 store would
       * otherwise repeat just one byte and lose its 0x17d7 boot marker.
       * Only explicit SET_INIT / SET_EBUS requests start a new window. */
      && (state->wword & UINT32_C(0x500))
      && (r4300->mi->regs[MI_INIT_MODE_REG] & 0x180) && !r4300->delay_slot'''
if new not in text:
    if text.count(old) != 1:
        raise SystemExit('Unsupported Ari64 MI window source; refusing an unchecked patch')
    text = text.replace(old, new)
    path.write_text(text)
print('Ari64 explicit MI mode window prepared in', root)

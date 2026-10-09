"""Reject incomplete PS4 packages before invoking GP4/PKG tools."""
import sys
from pathlib import Path

REQUIRED = (
    'eboot.bin', 'font.bin', 'LICENSE.txt', 'CREDITS.txt', 'FONT-LICENSE.txt',
    'sce_module/libc.prx', 'sce_module/libSceFios2.prx',
    'sce_sys/about/right.sprx',
)

def check(root):
    missing = [name for name in REQUIRED
               if not (root / name).is_file() or (root / name).stat().st_size == 0]
    if missing:
        raise SystemExit('Incomplete PS4 package: ' + ', '.join(missing))
    print('PASS: application payload and public OpenOrbis modules present')

if __name__ == '__main__':
    check(Path(sys.argv[1]))

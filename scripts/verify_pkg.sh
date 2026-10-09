#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk="${OO_PS4_TOOLCHAIN:-/work/toolchain/OpenOrbis/PS4Toolchain}"
pkg=out/IV0000-NPSX00064_00-NPS64PS400000000.pkg
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
"$sdk/bin/linux/PkgTool.Core" pkg_validate --verbose "$pkg" > build/pkg-validation.log
"$sdk/bin/linux/PkgTool.Core" pkg_listentries "$pkg" > build/pkg-entries.log
mkdir -p build/pkg-verify
"$sdk/bin/linux/PkgTool.Core" pkg_extract "$pkg" build/pkg-verify > build/pkg-extraction.log
python3 - <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, '/work/scripts')
from check_stage import REQUIRED, check
root=Path('/work')
pkg=root/'out/IV0000-NPSX00064_00-NPS64PS400000000.pkg'
with pkg.open('rb') as f:
    assert f.read(4)==b'\x7fCNT', 'Invalid PKG magic'
assert pkg.stat().st_size % 0x8000==0
log=(root/'build/pkg-validation.log').read_text()
checks=[line for line in log.splitlines() if line.startswith('[')]
assert len(checks)>20 and all(line.startswith('[OK]') for line in checks), log
check(root/'build/pkg-verify/uroot')
for name in REQUIRED:
    assert (root/'build/pkg-verify/uroot'/name).read_bytes()==(root/'build/stage'/name).read_bytes(), name
entries=(root/'build/pkg-entries.log').read_text()
assert 'PARAM_SFO' in entries and 'ICON0_PNG' in entries, entries
print(f'PASS: PKG {pkg.stat().st_size} bytes, {len(checks)} checks OK, extracted payload matches stage')
PY

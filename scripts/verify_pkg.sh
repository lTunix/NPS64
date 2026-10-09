#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk="${OO_PS4_TOOLCHAIN:-/work/toolchain/OpenOrbis/PS4Toolchain}"
pkg="${NPS64_PACKAGE_PATH:-out/IV0000-NPSX00064_00-NPS64PS400000000.pkg}"
verify="${NPS64_VERIFY_DIR:-/work/build/pkg-verify}"
export NPS64_PACKAGE_PATH="$pkg" NPS64_VERIFY_DIR="$verify"
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
"$sdk/bin/linux/PkgTool.Core" pkg_validate --verbose "$pkg" > build/pkg-validation.log
"$sdk/bin/linux/PkgTool.Core" pkg_listentries "$pkg" > build/pkg-entries.log
mkdir -p "$verify"
"$sdk/bin/linux/PkgTool.Core" pkg_extract "$pkg" "$verify" > build/pkg-extraction.log
python3 - <<'PY'
from pathlib import Path
import os
import sys
sys.path.insert(0, '/work/scripts')
from check_stage import REQUIRED, check
root=Path('/work')
pkg=root/os.environ['NPS64_PACKAGE_PATH']
verify=Path(os.environ['NPS64_VERIFY_DIR'])
stage=Path(os.environ.get('NPS64_STAGE_DIR','/work/build/stage'))
with pkg.open('rb') as f:
    assert f.read(4)==b'\x7fCNT', 'Invalid PKG magic'
assert pkg.stat().st_size % 0x8000==0
log=(root/'build/pkg-validation.log').read_text()
checks=[line for line in log.splitlines() if line.startswith('[')]
assert len(checks)>20 and all(line.startswith('[OK]') for line in checks), log
check(verify/'uroot')
for name in REQUIRED:
    assert (verify/'uroot'/name).read_bytes()==(stage/name).read_bytes(), name
entries=(root/'build/pkg-entries.log').read_text()
assert 'PARAM_SFO' in entries and 'ICON0_PNG' in entries, entries
print(f'PASS: PKG {pkg.stat().st_size} bytes, {len(checks)} checks OK, extracted payload matches stage')
PY

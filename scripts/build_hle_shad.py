"""Package an isolated emulator variant using the existing GNM HLE adapters."""
from pathlib import Path
import subprocess

root = Path('/work')
adapter = root / 'build/shad-adapters'
for name in ('libamd_common.a', 'libvulkan_radeon.a'):
    assert (adapter / name).is_file(), name
subprocess.run(['scripts/ps4-gpu-cc', '-std=c11', '-O2', '-c',
                'src/ps4_wide_abi.c', '-o', 'build/hle-game/wide.o'], check=True)
source = (root / 'scripts/build_hle_game.sh').read_text()
link = next(line for line in source.splitlines() if line.startswith('ld.lld-21 '))
link = link.replace('--start-group', 'build/hle-game/wide.o --wrap=wmemchr --start-group', 1)
link = link.replace('-lSceSysmodule', '-lSceSysmodule -lSceGnmDriver')
libraries = source[source.index('libs=('):source.index('ld.lld-21 ')]
libraries = libraries.replace('archives+=("$mesa/$lib")',
    'case "$lib" in src/amd/common/libamd_common.a) archives+=(build/shad-adapters/libamd_common.a);; '
    'src/amd/vulkan/libvulkan_radeon.a) archives+=(build/shad-adapters/libvulkan_radeon.a);; '
    '*) archives+=("$mesa/$lib");; esac')
fself = next(line for line in source.splitlines() if line.startswith('"$sdk/bin/linux/create-fself"'))
script = '\n'.join([
    'set -euo pipefail', 'cd /work',
    'sdk=/work/toolchain/OpenOrbis/PS4Toolchain', 'sysroot=/work/build/gpu/sysroot',
    'dir=build/hle-game', 'mesa=build/gpu/mesa-build', libraries, link, fself,
    'python3 scripts/check_elf.py', 'python3 scripts/check_gpu_boot.py',
    'NPS64_VERSION=00.65 bash scripts/package.sh', 'bash scripts/verify_pkg.sh',
    'cp out/IV0000-NPSX00064_00-NPS64PS400000000.pkg out/NPS64-00.65-hle-shadps4.pkg',
    'sha256sum out/NPS64-00.65-hle-shadps4.pkg > out/NPS64-00.65-hle-shadps4-SHA256.txt',
])
subprocess.run(['bash', '-c', script], check=True)

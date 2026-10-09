"""Check that the explicit experimental release matches its corresponding source."""
import hashlib
import tarfile
from pathlib import Path

root = Path('/work')
expected = {
    name: (root / name).read_bytes()
    for name in (
        'src/gpu_vulkan.c', 'src/frontend.c', 'src/gpu_boot.c',
        'scripts/patch_core.py', 'scripts/patch_gpu_driver.py',
        'src/gpu_shaders/color_raster_spv.hpp', 'scripts/compile_gpu_shaders.py',
        'tests/replay_gpu_capture.py', 'tests/test_gpu_hang_capture.py', 'scripts/analyze_gpu_capture.py',
        'scripts/build_gpu.sh', 'scripts/build_vulkan_test.ps1',
        'scripts/package.sh', 'README.md', 'TODO.md',
        'docs/VULKAN-00.19-TEST.md',
        'src/gpu_shaders/tile_binning_direct.comp',
        'src/gpu_shaders/tile_binning_direct_spv.hpp',
        'vendor/gpu/mesa-26.0.8/src/amd/common/ac_ps4_drm.c',
        'vendor/gpu/mesa-26.0.8/src/amd/common/ac_gpu_info.c',
        'vendor/gpu/mesa-26.0.8/src/amd/vulkan/radv_cmd_buffer.c',
    )
}
source = root / 'out/NPS64-00.19-source.tar.gz'
with tarfile.open(source, 'r|gz') as archive:
    for member in archive:
        assert Path(member.name).suffix.lower() not in {'.z64', '.n64', '.v64'}, member.name
        if member.name in expected:
            assert archive.extractfile(member).read() == expected.pop(member.name), member.name
assert not expected, list(expected)
pkg = root / 'out/NPS64-00.19-vulkan-test.pkg'
assert pkg.read_bytes() == (root / 'out/IV0000-NPSX00064_00-NPS64PS400000000.pkg').read_bytes()
assert b'00.19\x00' in (root / 'build/stage/sce_sys/param.sfo').read_bytes()
elf = (root / 'build/nps64.elf').read_bytes()
assert b'NPS64 00.19 EXPERIMENTAL' in elf
assert b'NPS64 direct tile binning = %d; local=64, no LDS/atomics/barriers' in elf
assert b'PS4 Vulkan disabled: unresolved' not in elf
assert b'PS4 Vulkan quarantined after system freeze' not in elf
assert b'gpu-hang-shader-%llx.bin' in elf
assert b'gpu-hang-data-%llx.bin' in elf
assert b'RADV_PS4_SERIAL_SUBMIT' in elf and b'NPS64_RDP_SERIALIZE_SETUP' in elf
assert b'NPS64 color-only raster batch' in elf
lines = []
for path in (pkg, source):
    with path.open('rb') as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    lines.append(f'{digest.hexdigest()}  {path.name}\n')
(root / 'out/NPS64-00.19-SHA256SUMS.txt').write_text(''.join(lines), encoding='ascii')
print('PASS: experimental package/version/ELF, exact current source overlays, no ROMs and SHA256 manifest')

"""Verify the standalone graphics probe and corresponding release sources."""
import hashlib, tarfile
from pathlib import Path
root=Path('/work')
names=['src/hle_display_list.c','src/hle_display_list.h','tests/hle_display_list_test.c',
       'src/gpu_hle_probe.c','src/hle_probe_frontend.c','src/gpu_shaders/hle_probe.vert',
       'src/gpu_shaders/hle_probe.frag','src/gpu_shaders/hle_probe_spv.h','src/gpu_shaders/hle_probe_dispatch.h',
       'scripts/compile_hle_probe.py','scripts/build_hle_probe.sh','scripts/check_gpu_boot.py',
       'scripts/test_hle_probe.sh','tests/hle_probe_host.c','docs/HLE-00.23-PROBE.md']
expected={name:(root/name).read_bytes() for name in names}
source=root/'out/NPS64-00.23-source.tar.gz'
with tarfile.open(source,'r|gz') as archive:
    for member in archive:
        assert Path(member.name).suffix.lower() not in {'.z64','.n64','.v64'}
        if member.name in expected:
            assert archive.extractfile(member).read()==expected.pop(member.name), member.name
assert not expected,expected
pkg=root/'out/NPS64-00.23-hle-graphics-probe.pkg'
assert pkg.read_bytes()==(root/'out/IV0000-NPSX00064_00-NPS64PS400000000.pkg').read_bytes()
assert b'00.23\0' in (root/'build/stage/sce_sys/param.sfo').read_bytes()
elf=(root/'build/nps64.elf').read_bytes()
assert b'NPS64 00.23 HLE GRAPHICS PROBE' in elf
assert b'EmuThread: M64CMD_EXECUTE' not in elf
manifest=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in [pkg,source])
(root/'out/NPS64-00.23-SHA256SUMS.txt').write_text(manifest)
print('PASS probe package, SFO, ELF, exact source overlays, no ROM and hashes')

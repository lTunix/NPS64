"""Verify the game geometry snapshot package and its corresponding sources."""
import hashlib, tarfile
from pathlib import Path
root=Path('/work')
names=['tests/hle_game_dialog_test.c','scripts/test_hle_game_dialog.sh','src/frontend.c','src/gpu_hle_probe.c','src/hle_game_geometry.c','src/hle_game_geometry.h',
       'tests/hle_renderer_live_test.c','scripts/test_hle_renderer_live.sh',
       'src/hle_game_material.h','src/hle_game_material.c','scripts/test_hle_game_texture.sh',
       'scripts/test_hle_game_combiner.sh','tests/hle_game_combiner_test.c',
       'tests/hle_game_texture_test.c','tests/hle_game_material_test.c',
       'tests/hle_game_sequence_test.c','scripts/test_hle_game_sequence.sh',
       'src/hle_display_list.c','src/hle_display_list.h','src/gpu_boot.c',
       'src/gpu_shaders/hle_probe.vert','src/gpu_shaders/hle_probe.frag',
       'src/gpu_shaders/hle_probe_spv.h','src/gpu_shaders/hle_probe_dispatch.h',
       'scripts/build_hle_game.sh','scripts/patch_hle_geometry.py','scripts/test_hle_game.sh',
       'scripts/check_hle_game_artifacts.py','tests/platform_bench.c',
       'scripts/test_hle_game_depth.sh','tests/hle_game_depth_test.c',
       'docs/HLE-00.37-VI-TEXTURES.md','docs/HLE-00.38-DEFERRED-VI.md','docs/HLE-SCREEN-SPACE-PERFORMANCE.md',
       'tests/hle_scene_replay.c','scripts/test_hle_scene_replay.sh','scripts/test_hle_deferred_vi.sh',
       'scripts/profile_hle_frames.py','scripts/profile_hle_frames.sh','scripts/test_hle_compare.sh','scripts/compare_hle_frames.py','TODO.md']
expected={name:(root/name).read_bytes() for name in names}
source=root/'out/NPS64-00.38-source.tar.gz'
with tarfile.open(source,'r|gz') as archive:
    for member in archive:
        assert Path(member.name).suffix.lower() not in {'.z64','.n64','.v64'}
        if member.name in expected:
            assert archive.extractfile(member).read()==expected.pop(member.name),member.name
assert not expected,expected
pkg=root/'out/NPS64-00.38-hle-game-sequence.pkg'
assert pkg.read_bytes()==(root/'out/IV0000-NPSX00064_00-NPS64PS400000000.pkg').read_bytes()
assert b'00.38\0' in (root/'build/stage/sce_sys/param.sfo').read_bytes()
elf=(root/'build/nps64.elf').read_bytes()
assert b'NPS64 00.38 Ari64 / Angrylion + Vulkan game geometry sequence' in elf
assert b'%s Vulkan sequence frames=' in elf
assert b'scene_frames=%u uniform_frames=%u' in elf
assert b'persistent renderer ready' in elf
assert b'[hle-live] mode=' in elf
assert b'renderer retained until process exit' in elf
assert hashlib.sha256((root/'out/NPS64-00.13.pkg').read_bytes()).hexdigest()=='83412904ab8e66d13fea71aadc7ae224c860e3272c23a6e3cfeeb5b3ecf39c22'
manifest=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in [pkg,source])
(root/'out/NPS64-00.38-SHA256SUMS.txt').write_text(manifest)
print('PASS game snapshot package, version, source overlays, no ROMs, stable package and hashes')

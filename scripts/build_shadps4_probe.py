"""Build an isolated PS4 eboot importing shadPS4's GNM HLE entry points.

Run in nps64-gpu-builder after building the normal ParaLLEl game staging.
Does not replace Mesa archives, production staging, or console packages.
"""
import json
from pathlib import Path
import shlex
import shutil
import subprocess

root = Path('/work')
out = root / 'build/shadps4/probe'
out.mkdir(parents=True, exist_ok=True)
mesa = root / 'build/gpu/mesa-build'
source = (root / 'vendor/gpu/mesa-26.0.8/src/amd/common/ac_ps4_drm.c').read_text()
start = source.index('static bool\nps4_load_gnm(')
end = source.index('\n/* ---', start)
source = source[:start] + '''
/* Local emulator probe only: resolve genuine HLE imports, not firmware dlopen. */
extern int sceGnmSubmitCommandBuffers(uint32_t, void *[], uint32_t *, void *[], uint32_t *);
extern int sceGnmSubmitDone(void);
extern int sceGnmAddEqEvent(void *, uint64_t, void *);
static bool
ps4_load_gnm(ac_drm_device *dev)
{
   dev->submit = sceGnmSubmitCommandBuffers;
   dev->submit_done = sceGnmSubmitDone;
   dev->add_eq_event = sceGnmAddEqEvent;
   dev->submit_and_flip = NULL;
   dev->gnm_module = -1;
   ps4_log("shadPS4 probe: direct GNM HLE imports; no firmware module\\n");
   return true;
}
''' + source[end:]
# Firmware fault discovery requires a real module; no fake handle is supplied.
assert source.count('if (!ps4_fault_dev)\n      ps4_install_fault_capture') == 1
source = source.replace('if (!ps4_fault_dev)\n      ps4_install_fault_capture',
                        'if (!ps4_fault_dev && dev->gnm_module >= 0)\n      ps4_install_fault_capture')
patched = out / 'ac_ps4_drm.c'
patched.write_text(source)
entry = next(e for e in json.loads((mesa / 'compile_commands.json').read_text())
             if e['file'].endswith('ac_ps4_drm.c'))
command = shlex.split(entry['command'])
for flag in ('-o', '-MF', '-MQ'):
    idx = command.index(flag)
    command[idx + 1] = str(out / ('ac_ps4_drm.c.o' if flag != '-MF' else 'ac_ps4_drm.c.o.d'))
command[command.index('-c') + 1] = str(patched)
subprocess.run(command, cwd=mesa, check=True)
archive = out / 'libamd_common.a'
original = mesa / 'src/amd/common/libamd_common.a'
members = subprocess.check_output(['llvm-ar-21', 't', str(original)], text=True).splitlines()
assert sum(Path(m).name == 'ac_ps4_drm.c.o' for m in members) == 1
objects = [str(out / 'ac_ps4_drm.c.o') if Path(m).name == 'ac_ps4_drm.c.o'
           else str(original.parent / m) for m in members]
if archive.exists():
    archive.unlink()
subprocess.run(['llvm-ar-21', 'rcs', str(archive), *objects], check=True)

# shadPS4 requires an OrbShdr trailer to identify shader length and cache key.
# Preserve every instruction and derive both values from Mesa's compiled binary.
shader_source = (root / 'vendor/gpu/mesa-26.0.8/src/amd/vulkan/radv_shader.c').read_text()
helper = '''
static void
nps64_shad_shader_info(void *dest, const struct radv_shader *shader)
{
   const unsigned offset = (shader->code_size + 7u) & ~7u;
   uint8_t *info = (uint8_t *)dest + offset;
   const uint32_t length = shader->code_size << 8;
   memset((uint8_t *)dest + shader->code_size, 0, offset - shader->code_size + 32);
   memcpy(info, "OrbShdr", 7);
   info[7] = 1;
   memcpy(info + 8, &length, sizeof(length));
   memcpy(info + 16, shader->hash, 8);
}

'''
anchor = 'static bool\nradv_shader_upload('
assert shader_source.count(anchor) == 1
shader_source = shader_source.replace(anchor, helper + anchor)
anchor = 'radv_alloc_shader_memory(device, shader->code_size, replayable, shader)'
assert shader_source.count(anchor) == 1
shader_source = shader_source.replace(anchor, 'radv_alloc_shader_memory(device, ((shader->code_size + 7u) & ~7u) + 32u, replayable, shader)')
anchor = '      if (!radv_shader_binary_upload(device, binary, shader, dest_ptr))\n         return false;'
assert shader_source.count(anchor) == 1
shader_source = shader_source.replace(anchor, anchor + '\n      nps64_shad_shader_info(dest_ptr, shader);')
anchor = '      memcpy(dest_ptr, shader->code, shader->code_size);'
assert shader_source.count(anchor) == 1
shader_source = shader_source.replace(anchor, anchor + '\n      nps64_shad_shader_info(dest_ptr, shader);')
# This probe requires visible shader allocations, as provided by the PS4 winsys.
anchor = 'radv_shader_upload(struct radv_device *device, struct radv_shader *shader, const struct radv_shader_binary *binary)\n{'
assert shader_source.count(anchor) == 1
shader_source = shader_source.replace(anchor, anchor + '\n   if (device->shader_use_invisible_vram || shader->code_size >= 0x10000 || shader->has_replay_alloc)\n      return false; /* Unsupported trailer placement: fail rather than guess. */')
shader_path = out / 'radv_shader.c'
shader_path.write_text(shader_source)
entry = next(e for e in json.loads((mesa / 'compile_commands.json').read_text())
             if e['file'].endswith('/radv_shader.c'))
shader_object = out / Path(entry['output']).name
command = shlex.split(entry['command'])
for flag in ('-o', '-MF', '-MQ'):
    command[command.index(flag) + 1] = str(shader_object) + ('.d' if flag == '-MF' else '')
command[command.index('-c') + 1] = str(shader_path)
subprocess.run(command, cwd=mesa, check=True)
original = mesa / 'src/amd/vulkan/libvulkan_radeon.a'
shader_archive = out / 'libvulkan_radeon.a'
members = subprocess.check_output(['llvm-ar-21', 't', str(original)], text=True).splitlines()
assert sum(Path(m).name == shader_object.name for m in members) == 1
objects = [str(shader_object) if Path(m).name == shader_object.name
           else str(original.parent / m) for m in members]
if shader_archive.exists():
    shader_archive.unlink()
subprocess.run(['llvm-ar-21', 'rcs', str(shader_archive), *objects], check=True)
shutil.copytree(root / 'build/parallel-game/stage', out / 'stage', dirs_exist_ok=True)
build = (root / 'scripts/build_parallel_game.sh').read_text()
link = next(line for line in build.splitlines() if line.startswith('ld.lld-21 '))
link = link.replace('"$game"/objects/', '/work/build/parallel-game/objects/')
link = link.replace('-lSceSysmodule', '-lSceSysmodule -lSceGnmDriver')
libs = build[build.index('libs=('):build.index('ld.lld-21 ')]
libs = libs.replace('archives+=("$mesa/$lib")',
                    'if [ "$lib" = src/amd/common/libamd_common.a ]; then archives+=("$game/libamd_common.a"); elif [ "$lib" = src/amd/vulkan/libvulkan_radeon.a ]; then archives+=("$game/libvulkan_radeon.a"); else archives+=("$mesa/$lib"); fi')
fself = next(line for line in build.splitlines() if line.startswith('"$sdk/bin/linux/create-fself"'))
script = '\n'.join(['set -euo pipefail', 'cd /work',
                    'sdk=/work/toolchain/OpenOrbis/PS4Toolchain',
                    'sysroot=/work/build/gpu/sysroot', f'game={out}',
                    'mesa=/work/build/gpu/mesa-build',
                    'NPS64_STAGE_DIR="$game/stage"', libs, link, fself,
                    'export NPS64_ELF_DIR="$game"', 'python3 scripts/check_elf.py'])
(out / 'build.sh').write_text(script)
subprocess.run(['bash', str(out / 'build.sh')], check=True)
print('Built isolated shadPS4 probe:', out / 'stage/eboot.bin')

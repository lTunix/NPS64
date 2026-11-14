#!/usr/bin/env python3
"""Initialize the N64 machine without executing guest graphics before Vulkan."""
from pathlib import Path
import sys

def patch_boot(root):
    main=root/'mupen64plus-core/src/main/main.c'
    libretro=root/'libretro/libretro.c'
    text=main.read_text()
    if 'int nps64_initialize_only;' in text:
        assert 'nps64_initialize_only = gfx_plugin == GFX_PARALLEL;' in libretro.read_text()
        return
    old='m64p_error main_run(void)\n{'
    assert text.count(old)==1
    text=text.replace(old,'/* The Vulkan context needs initialized device memory, not a running CPU. */\nint nps64_initialize_only;\n\n'+old)
    old='    pif_bootrom_hle_execute(&g_dev.r4300);\n\n    run_device(&g_dev);'
    assert text.count(old)==1
    text=text.replace(old,'''    pif_bootrom_hle_execute(&g_dev.r4300);

    /* CoreDoCommand(EXECUTE) used to run one whole VI during context_reset,
     * before parallel_init. Tennis submits its bootstrap FULLSYNC in that
     * interval; the uninitialized renderer drops it and the game waits forever.
     * Device setup remains complete; the first retro_run starts the CPU. */
    if (nps64_initialize_only)
        return M64ERR_SUCCESS;

    run_device(&g_dev);''')
    lib=libretro.read_text()
    old='   CoreDoCommand(M64CMD_EXECUTE, 0, NULL);'
    assert lib.count(old)==1
    lib=lib.replace(old,'''   {
      extern int nps64_initialize_only;
      nps64_initialize_only = gfx_plugin == GFX_PARALLEL;
      CoreDoCommand(M64CMD_EXECUTE, 0, NULL);
      nps64_initialize_only = 0;
   }''')
    main.write_text(text);libretro.write_text(lib)

if __name__=='__main__':patch_boot(Path(sys.argv[1]))

#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk="${OO_PS4_TOOLCHAIN:-/work/toolchain/OpenOrbis/PS4Toolchain}"
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
tools="$sdk/bin/linux"
stage=/work/build/stage
content=IV0000-NPSX00064_00-NPS64PS400000000
sfo="$stage/sce_sys/param.sfo"
rm -f "$sfo"
"$tools/PkgTool.Core" sfo_new "$sfo"
entry() { "$tools/PkgTool.Core" sfo_setentry "$sfo" "$1" --type "$2" --maxsize "$3" --value "$4"; }
entry APP_TYPE Integer 4 1
entry APP_VER Utf8 8 "${NPS64_VERSION:-00.13}"
entry ATTRIBUTE Integer 4 0
entry CATEGORY Utf8 4 gd
entry CONTENT_ID Utf8 48 "$content"
entry DOWNLOAD_DATA_SIZE Integer 4 0
entry SYSTEM_VER Integer 4 0
entry TITLE Utf8 128 'NPS64 - Experimental'
entry TITLE_ID Utf8 12 NPSX00064
entry VERSION Utf8 8 "${NPS64_VERSION:-00.13}"
cp -a assets/font.bin "$stage/"
cp LICENSE "$stage/LICENSE.txt"
cp docs/CREDITS.md "$stage/CREDITS.txt"
cp /usr/share/doc/fonts-dejavu-core/copyright "$stage/FONT-LICENSE.txt"
mkdir -p "$stage/sce_module" "$stage/sce_sys/about"
# Public OpenOrbis runtime placeholders, also packaged by DolphinPS4.
cp "$sdk/samples/piglet/sce_module/libc.prx" "$stage/sce_module/"
cp "$sdk/samples/piglet/sce_module/libSceFios2.prx" "$stage/sce_module/"
cp "$sdk/samples/piglet/sce_sys/about/right.sprx" "$stage/sce_sys/about/"
python3 /work/scripts/check_stage.py "$stage"
cd "$stage"
rm -f pkg.gp4
"$tools/create-gp4" -out pkg.gp4 --content-id="$content" --files 'eboot.bin font.bin LICENSE.txt CREDITS.txt FONT-LICENSE.txt sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx sce_sys/about/right.sprx'
"$tools/PkgTool.Core" pkg_build pkg.gp4 /work/out
rm -f pkg.gp4

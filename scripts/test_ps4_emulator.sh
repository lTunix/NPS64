#!/usr/bin/env bash
set -euo pipefail
cd /work
# Run inside the pinned linuxserver/shadps4 container. This is an integration probe.
app=/work/build/shadps4/squashfs-root/AppRun
binary=/work/build/stage/eboot.bin
if ! test -x "$app"; then
    chmod +x build/shadps4/Shadps4-sdl.AppImage
    (cd build/shadps4 && ./Shadps4-sdl.AppImage --appimage-extract >/dev/null)
fi
test -x "$app"
test -f "$binary"
mkdir -p /config/.local/share/shadPS4/data/NPS64 /work/build/shadps4/runtime
guest_log=/config/.local/share/shadPS4/data/NPS64/nps64.log
: > "$guest_log"
export XDG_RUNTIME_DIR=/work/build/shadps4/runtime
chmod 700 "$XDG_RUNTIME_DIR"
Xvfb :99 -screen 0 1280x720x24 >build/shadps4/xvfb.log 2>&1 &
xpid=$!
trap 'kill "$xpid" 2>/dev/null || true' EXIT
export DISPLAY=:99
set +e
timeout 30 "$app" -g "$binary" >build/shadps4/integration.log 2>&1
status=$?
set -e
printf 'Emulator process exit=%s (124 means timeout)
' "$status"
tail -15 build/shadps4/integration.log
# A process exit of zero does not mean the game reached main or rendered.
if ! test -s "$guest_log"; then
    echo 'NOT VALIDATED: guest log missing; inspect integration.log for guest initialization and unsupported calls.'
    exit 2
fi
cp "$guest_log" build/shadps4/guest.log
if ! grep -q 'state=library' "$guest_log"; then
    echo 'NOT VALIDATED: guest did not reach library loop.'; exit 2
fi
echo 'PASS STARTUP ONLY: PS4 binary reached library loop. ROM/Vulkan gameplay not tested.'


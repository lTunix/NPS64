#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p downloads toolchain vendor reference
if [ ! -f toolchain/OpenOrbis/PS4Toolchain/lib/libc.a ]; then
  curl -fL --retry 3 https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/download/v0.5.4/toolchain-llvm-18.tar.gz -o downloads/openorbis.tar.gz
  echo '3c7cd5bb593ca74fa1c13fd59f3938dc0fc07985167f7275063019e63abe4526  downloads/openorbis.tar.gz' | sha256sum -c -
  tar -xzf downloads/openorbis.tar.gz -C toolchain
fi
python3 scripts/fetch_sources.py
